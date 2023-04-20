#include "StreamPuller.h"
#include <stdexcept>
#include "H264VideoRTPSource.hh" // for parseSPropParameterSets
extern "C"
{
#include "libavutil\opt.h"
}
#include "StreamSink.h"

static uint8_t startCode4[4] = { 0x00, 0x00, 0x00, 0x01 };

static bool is_keyframe_h264(AVPacket* pkt) {
    //printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
    //    pkt->data[0], pkt->data[1], pkt->data[2], pkt->data[3], pkt->data[4],
    //    pkt->data[5], pkt->data[6], pkt->data[7], pkt->data[8], pkt->data[9]);
    int length = 0;
    unsigned char* p_buffer = pkt->data;
    int i_buffer = pkt->size;
    if ((pkt->data[0] == 0 && pkt->data[1] == 0 && pkt->data[2] == 0 && pkt->data[3] == 1) ||
        (pkt->data[0] == 0 && pkt->data[1] == 0 && pkt->data[2] == 1)) {
        while (i_buffer > 3) {
            if (p_buffer[0] == 0 && p_buffer[1] == 0 && p_buffer[2] == 0 && p_buffer[3] == 1) {
                p_buffer += 4;
                i_buffer -= 4;
                int nal_type = p_buffer[0] & 0x1F;
                if (nal_type == 5) return true;
            }
            if (p_buffer[0] == 0 && p_buffer[1] == 0 && p_buffer[2] == 1) {
                p_buffer += 3;
                i_buffer -= 3;
                int nal_type = p_buffer[0] & 0x1F;
                if (nal_type == 5) return true;
            }
            p_buffer++;
            i_buffer--;
        }
    }
    else {
        while (i_buffer > 4) {
            unsigned char* p = (unsigned char*)&length;
            p[3] = p_buffer[0];
            p[2] = p_buffer[1];
            p[1] = p_buffer[2];
            p[0] = p_buffer[3];
            p_buffer += 4;
            i_buffer -= 4;
            int nal_type = p_buffer[0] & 0x1F;
            if (nal_type == 5) return true;
            p_buffer += length;
            i_buffer -= length;
        }
    }
    return false;
}

StreamPuller* StreamPuller::instance;

StreamPuller::StreamPuller()
{
    instance = this;

    pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (pCodec == NULL)
    {
        throw std::runtime_error("Codec not found.");
    }

    pCodecCtx = avcodec_alloc_context3(pCodec);
    pCodecCtx->flags = AV_CODEC_FLAG_LOW_DELAY;
    pCodecCtx->flags2 = AV_CODEC_FLAG2_CHUNKS;
    //av_opt_set(pCodecCtx->priv_data, "preset", "ultrafast", 0);
    //av_opt_set(pCodecCtx->priv_data, "tune", "zerolatency", 0);

    AVDictionary* opts = NULL;
    av_dict_set(&opts, "preset", "ultrafast", 0);
    av_dict_set(&opts, "tune", "zerolatency", 0);
    if (avcodec_open2(pCodecCtx, pCodec, &opts) < 0)
    {
        throw std::runtime_error("Open codec failed.");
    }
    av_dict_free(&opts);

    pFrame = av_frame_alloc();
}

StreamPuller::~StreamPuller()
{
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);

    instance = nullptr;
}

bool StreamPuller::start(const char* url, Transport transport, callback_t callback)
{
    pUrl = url;
    mTransport = transport;
    pCallback = callback;

    mExit = 0;
    pThread = std::thread(&StreamPuller::run, this);

    return true;
}

void StreamPuller::stop()
{
    mExit = 1;
    pThread.join();
}

void StreamPuller::run()
{
    scheduler = BasicTaskScheduler::createNew();
    environment = BasicUsageEnvironment::createNew(*scheduler);

    rtspClient = RTSPClient::createNew(*environment, pUrl, 1, "CoralReefCam");
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE);

    environment->taskScheduler().doEventLoop(&mExit);

    if (rtspClient != NULL)
    {
        shutdownStream(rtspClient);
    }
    delete scheduler;
}

void StreamPuller::shutdownStream(RTSPClient* rtspClient) {
    UsageEnvironment& env = rtspClient->envir();

    if (session != NULL)
    {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter(*session);
        MediaSubsession* subsession;

        while ((subsession = iter.next()) != NULL)
        {
            if (subsession->sink != NULL)
            {
                Medium::close(subsession->sink);
                subsession->sink = NULL;
                if (subsession->rtcpInstance() != NULL)
                {
                    subsession->rtcpInstance()->setByeHandler(NULL, NULL);
                }
                someSubsessionsWereActive = True;
            }
        }

        if (someSubsessionsWereActive)
        {
            rtspClient->sendTeardownCommand(*session, NULL);
        }
    }

    env << "Closing the stream.\n";
    Medium::close(rtspClient);
    rtspClient = NULL;
}

void StreamPuller::continueAfterDESCRIBE0(RTSPClient* rtspClient, int resultCode, char* resultString) {
    UsageEnvironment& env = rtspClient->envir();

    if (resultCode != 0)
    {
        env << "Failed to get a SDP description: " << resultString << "\n";
        return;
    }
    char* const sdpDescription = resultString;
    env << "Got a SDP description:\n" << sdpDescription << "\n";

    session = MediaSession::createNew(env, sdpDescription);
    if (session == NULL)
    {
        env << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
        goto end;
    }
    else if (!session->hasSubsessions())
    {
        env << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
        goto end;
    }

    iter = new MediaSubsessionIterator(*session);
    setupNextSubsession(rtspClient);
    return;

end:
    shutdownStream(rtspClient);
}

void StreamPuller::continueAfterSETUP0(RTSPClient* rtspClient, int resultCode, char* resultString) {
    UsageEnvironment& env = rtspClient->envir();
    const char *codecName, *sprops;

    if (resultCode != 0)
    {
        env << "Failed to set up the subsession: " << resultString << "\n";
        goto end;
    }

    env << "Set up the subsession (" << subsession->clientPortNum() << ")\n";

    codecName = subsession->codecName();
    if (strcmp(codecName, "H264") != 0)
    {
        env << "Not support codec: " << codecName << "\n";
        goto end;
    }

    sprops = subsession->fmtp_spropparametersets();
    if (strcmp(sprops, "") != 0)
    {
        unsigned spropCount;
        SPropRecord* spropRecords = parseSPropParameterSets(sprops, spropCount);
        if (spropCount >= 2)
        {
            SPropRecord& sps = spropRecords[0];
            SPropRecord& pps = spropRecords[1];

            int size = 8 + sps.sPropLength + pps.sPropLength;
            uint8_t* buffer = new uint8_t[size];
            memcpy(buffer, startCode4, 4);
            memcpy(buffer + 4, sps.sPropBytes, sps.sPropLength);
            memcpy(buffer + 4 + sps.sPropLength, startCode4, 4);
            memcpy(buffer + 4 + sps.sPropLength + 4, pps.sPropBytes, pps.sPropLength);
            pCodecCtx->extradata = buffer;
            pCodecCtx->extradata_size = size;

            delete[] spropRecords;
            env << "Get SPropRecords.\n";
        }
    }

    subsession->sink = StreamSink::createNew(env, *subsession, [this, &env](StreamSink* sink, AVPacket* packet)
    {
        bool isKeyframe = is_keyframe_h264(packet);
        if (!sink->fHasFirstKeyframe)
        {
            if (!isKeyframe)
            {
                env << "Waiting for the first keyframe.\n";
                return;
            }
            sink->fHasFirstKeyframe = True;
        }

        if (isKeyframe)
        {
            //avcodec_flush_buffers(pCodecCtx);
            //env << "Flush deocder buffer.\n";
        }

        if (avcodec_send_packet(pCodecCtx, packet) < 0)
        {
            env << "Decode Error.\n";
            return;
        }

        while (avcodec_receive_frame(pCodecCtx, pFrame) >= 0)
        {
            AVFrame* frameYUV = av_frame_alloc();
            frameYUV->format = pFrame->format;
            frameYUV->width = pFrame->width;
            frameYUV->height = pFrame->height;
            frameYUV->channels = pFrame->channels;
            frameYUV->channel_layout = pFrame->channel_layout;
            frameYUV->nb_samples = pFrame->nb_samples;
            av_frame_get_buffer(frameYUV, 32);
            av_frame_copy(frameYUV, pFrame);
            av_frame_copy_props(frameYUV, pFrame);

            //frameYUV->format = AV_PIX_FMT_YUVJ420P;
            //frameYUV->width = pCodecCtx->width;
            //frameYUV->height = pCodecCtx->height;
            //av_frame_get_buffer(frameYUV, 32);

            //struct SwsContext* swsCxt = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
            //	pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUVJ420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
            //sws_scale(swsCxt, pFrame->data, pFrame->linesize, 0, pCodecCtx->height, frameYUV->data, frameYUV->linesize);
            //sws_freeContext(swsCxt);

            pCallback(frameYUV);
        }
    });
    if (subsession->sink == NULL)
    {
        env << "Failed to create a data sink for the subsession: " << env.getResultMsg() << "\n";
        goto end;
    }

    env << "Created a data sink for the subsession\n";
    subsession->miscPtr = rtspClient;
    subsession->sink->startPlaying(*(subsession->readSource()), subsessionAfterPlaying, subsession);
    if (subsession->rtcpInstance() != NULL)
    {
        subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, subsession);
    }
    
end:
    setupNextSubsession(rtspClient);
}

void StreamPuller::continueAfterPLAY0(RTSPClient* rtspClient, int resultCode, char* resultString) {
    UsageEnvironment& env = rtspClient->envir();

    if (resultCode != 0)
    {
        env << "Failed to start playing session: " << resultString << "\n";
        shutdownStream(rtspClient);
        return;
    }
    env << "Started playing session...\n";
}

void StreamPuller::setupNextSubsession(RTSPClient* rtspClient) {
    UsageEnvironment& env = rtspClient->envir();

    subsession = iter->next();
    if (subsession != NULL)
    {
        if (!subsession->initiate())
        {
            env << "Failed to initiate the subsession: " << env.getResultMsg() << "\n";
            setupNextSubsession(rtspClient);
        }
        else
        {
            env << "Initiated the subsession (" << subsession->clientPortNum() << ")\n";
            rtspClient->sendSetupCommand(*subsession, continueAfterSETUP, False, mTransport == Transport::TCP);
        }
        return;
    }

    rtspClient->sendPlayCommand(*session, continueAfterPLAY);
}

void StreamPuller::subsessionAfterPlaying(MediaSubsession* subsession) {
    Medium::close(subsession->sink);
    subsession->sink = NULL;

    MediaSession& session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != NULL)
    {
        if (subsession->sink != NULL)
            return;
    }

    shutdownStream(rtspClient);
}

void StreamPuller::subsessionByeHandler(MediaSubsession* subsession, char const* reason) {
    UsageEnvironment& env = rtspClient->envir();

    env << "Received RTCP \"BYE\"";
    if (reason != NULL)
    {
        env << " (reason:\"" << reason << "\")";
    }
    env << " on subsession\n";

    subsessionAfterPlaying(subsession);
}
