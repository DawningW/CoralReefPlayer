#include "StreamPuller.h"
#include <stdexcept>
#include <atomic>
#include "H264VideoRTPSource.hh" // for parseSPropParameterSets
extern "C"
{
#include "libavutil/opt.h"
}
#include "StreamSink.h"

static uint8_t startCode4[4] = { 0x00, 0x00, 0x00, 0x01 };

static AVPixelFormat to_av_format(Format format) {
    switch (format) {
        case CRP_YUV420P: return AV_PIX_FMT_YUV420P;
        case CRP_RGB24: return AV_PIX_FMT_RGB24;
        case CRP_BGR24: return AV_PIX_FMT_BGR24;
        case CRP_ARGB32: return AV_PIX_FMT_ARGB;
        case CRP_RGBA32: return AV_PIX_FMT_RGBA;
        case CRP_ABGR32: return AV_PIX_FMT_ABGR;
        case CRP_BGRA32: return AV_PIX_FMT_BGRA;
    }
    return AV_PIX_FMT_NONE;
}

static int find_sps_pps_before_I_frame(AVPacket* pkt) {
    //printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
    //    pkt->data[0], pkt->data[1], pkt->data[2], pkt->data[3], pkt->data[4],
    //    pkt->data[5], pkt->data[6], pkt->data[7], pkt->data[8], pkt->data[9]);
    int length = 0;
    unsigned char* buffer = pkt->data;
    int size = pkt->size;
    if ((pkt->data[0] == 0 && pkt->data[1] == 0 && pkt->data[2] == 0 && pkt->data[3] == 1) ||
        (pkt->data[0] == 0 && pkt->data[1] == 0 && pkt->data[2] == 1)) {
        while (size > 3) {
            if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 1) {
                int nal_type = buffer[4] & 0x1F;
                if (nal_type == 5) {
                    return pkt->size - size;
                }
                buffer += 4;
                size -= 4;
            }
            if (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 1) {
                int nal_type = buffer[3] & 0x1F;
                if (nal_type == 5) {
                    return pkt->size - size;
                }
                buffer += 3;
                size -= 3;
            }
            buffer++;
            size--;
        }
    } else {
        while (size > 4) {
            unsigned char* p = (unsigned char*) &length;
            p[3] = buffer[0];
            p[2] = buffer[1];
            p[1] = buffer[2];
            p[0] = buffer[3];
            int nal_type = buffer[4] & 0x1F;
            if (nal_type == 5) {
                return pkt->size - size;
            }
            buffer += 4 + length;
            size -= 4 + length;
        }
    }
    return 0;
}

thread_local StreamPuller* StreamPuller::instance;

StreamPuller::StreamPuller() : authenticator(nullptr), outFrame{}, exit(1)
{
    codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (codec == NULL)
    {
        throw std::runtime_error("Codec not found.");
    }

    codecCtx = avcodec_alloc_context3(codec);
    codecCtx->flags = AV_CODEC_FLAG_LOW_DELAY;
    codecCtx->flags2 = AV_CODEC_FLAG2_CHUNKS;
    codecCtx->extradata = (uint8_t*) av_malloc(AV_INPUT_BUFFER_PADDING_SIZE);
    codecCtx->extradata_size = 0;

    // 由于需要sps和pps, 解码器初始化已移至void initCodec()函数

    frame = av_frame_alloc();
}

StreamPuller::~StreamPuller()
{
    if (outFrame.data[0] != nullptr)
    {
        av_freep(&outFrame.data[0]);
    }
    av_frame_free(&frame);
    avcodec_close(codecCtx);
    av_free(codecCtx->extradata);
}

void StreamPuller::authenticate(const char* username, const char* password, bool useMD5)
{
    if (authenticator != nullptr)
        delete authenticator;
    authenticator = new Authenticator(username, password, useMD5);
}

bool StreamPuller::start(const char* url, Transport transport, int width, int height, Format format, Callback callback)
{
    if (!exit) return false;

    this->url = std::make_unique<char[]>(strlen(url) + 1);
    strcpy(this->url.get(), url);
    this->transport = transport;
    outFrame.width = width;
    outFrame.height = height;
    outFrame.format = format;
    this->callback = callback;

    exit = 0;
    thread = std::thread(&StreamPuller::run, this);
    callbackThread = std::thread(&StreamPuller::runCallback, this);

    return true;
}

void StreamPuller::stop()
{
    exit = 1;
    signal.test_and_set();
    signal.notify_all();
    thread.join();
    callbackThread.join();
}

void StreamPuller::initCodec()
{
    AVDictionary* opts = NULL;
    av_dict_set(&opts, "preset", "ultrafast", 0);
    av_dict_set(&opts, "tune", "zerolatency", 0);
    //av_dict_set(&opts, "strict", "1", 0);
    if (avcodec_open2(codecCtx, codec, &opts) < 0)
    {
        throw std::runtime_error("Open codec failed.");
    }
    av_dict_free(&opts);
}

void StreamPuller::initBuffer()
{
    av_image_alloc(outFrame.data, outFrame.linesize, outFrame.width, outFrame.height,
        to_av_format(outFrame.format), 1);
}

void StreamPuller::run()
{
    instance = this;

    scheduler = BasicTaskScheduler::createNew();
    environment = BasicUsageEnvironment::createNew(*scheduler);

    rtspClient = RTSPClient::createNew(*environment, url.get(), 1, "CoralReefPlayer");
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE, authenticator);
    session = NULL;

    environment->taskScheduler().doEventLoop(&exit);

    if (rtspClient != NULL)
    {
        shutdownStream(rtspClient);
    }
    delete scheduler;
}

void StreamPuller::runCallback()
{
    while (!exit)
    {
        signal.wait(false);
        if (!exit)
        {
            callback(CRP_EV_NEW_FRAME, &outFrame);
        }
        signal.clear();
    }
}

void StreamPuller::shutdownStream(RTSPClient* rtspClient)
{
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

void StreamPuller::continueAfterDESCRIBE0(RTSPClient* rtspClient, int resultCode, char* resultString)
{
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

void StreamPuller::continueAfterSETUP0(RTSPClient* rtspClient, int resultCode, char* resultString)
{
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
            uint8_t* buffer = codecCtx->extradata;
            memcpy(buffer, startCode4, 4);
            memcpy(buffer + 4, sps.sPropBytes, sps.sPropLength);
            memcpy(buffer + 4 + sps.sPropLength, startCode4, 4);
            memcpy(buffer + 4 + sps.sPropLength + 4, pps.sPropBytes, pps.sPropLength);
            codecCtx->extradata_size = size;

            delete[] spropRecords;
            env << "Get SPropRecords.\n";
        }
    }

    subsession->sink = StreamSink::createNew(env, *subsession, [this, &env](StreamSink* sink, AVPacket* packet)
    {
        // FIXME 有些监控单独发SPS和PPS, 并不在I帧前面
        int sps_pps_len = find_sps_pps_before_I_frame(packet);
        if (!sink->fHasFirstKeyframe)
        {
            if (codecCtx->extradata_size <= 0)
            {
                if (sps_pps_len <= 0)
                {
                    env << "Waiting for the first keyframe.\n";
                    return;
                }
                memcpy(codecCtx->extradata, packet->data, sps_pps_len);
                codecCtx->extradata_size = sps_pps_len;
            }
            initCodec();
            if (outFrame.width != CRP_WIDTH_AUTO && outFrame.height != CRP_HEIGHT_AUTO)
                initBuffer();
            sink->fHasFirstKeyframe = True;
        }

        if (sps_pps_len > 0)
        {
            packet->data += sps_pps_len;
            packet->size -= sps_pps_len;

            //avcodec_flush_buffers(codecCtx);
            //env << "Flush deocder buffer.\n";
        }

        codecCtx->has_b_frames = 0; // 丢帧使reordering buffer增大导致延迟变高
        if (avcodec_send_packet(codecCtx, packet) < 0)
        {
            env << "Decode Error.\n";
            return;
        }

        while (avcodec_receive_frame(codecCtx, frame) >= 0)
        {
            if (outFrame.width == CRP_WIDTH_AUTO && outFrame.height == CRP_HEIGHT_AUTO)
            {
                outFrame.width = codecCtx->width;
                outFrame.height = codecCtx->height;
                initBuffer();
            }

            //AVFrame* frameYUV = av_frame_alloc();
            //frameYUV->format = frame->format;
            //frameYUV->width = frame->width;
            //frameYUV->height = frame->height;
            //frameYUV->channels = frame->channels;
            //frameYUV->channel_layout = frame->channel_layout;
            //frameYUV->nb_samples = frame->nb_samples;
            //av_frame_get_buffer(frameYUV, 32);
            //av_frame_copy(frameYUV, frame);
            //av_frame_copy_props(frameYUV, frame);
            //av_frame_free(frameYUV);

            AVPixelFormat pixFmt = codecCtx->pix_fmt == AV_PIX_FMT_YUVJ420P ? AV_PIX_FMT_YUV420P : codecCtx->pix_fmt;
            struct SwsContext* swsCxt = sws_getContext(codecCtx->width, codecCtx->height, pixFmt,
            	outFrame.width, outFrame.height, to_av_format(outFrame.format), SWS_FAST_BILINEAR, NULL, NULL, NULL);
            sws_scale(swsCxt, frame->data, frame->linesize, 0, codecCtx->height, outFrame.data, outFrame.linesize);
            sws_freeContext(swsCxt);
            outFrame.pts = frame->pts;

            signal.test_and_set();
            signal.notify_all();
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

void StreamPuller::continueAfterPLAY0(RTSPClient* rtspClient, int resultCode, char* resultString)
{
    UsageEnvironment& env = rtspClient->envir();

    if (resultCode != 0)
    {
        env << "Failed to start playing session: " << resultString << "\n";
        shutdownStream(rtspClient);
        return;
    }
    env << "Started playing session...\n";
}

void StreamPuller::setupNextSubsession(RTSPClient* rtspClient)
{
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
            rtspClient->sendSetupCommand(*subsession, continueAfterSETUP, False, transport == CRP_TCP);
        }
        return;
    }

    rtspClient->sendPlayCommand(*session, continueAfterPLAY);
}

void StreamPuller::subsessionAfterPlaying(MediaSubsession* subsession)
{
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

void StreamPuller::subsessionByeHandler(MediaSubsession* subsession, char const* reason)
{
    UsageEnvironment& env = rtspClient->envir();

    env << "Received RTCP \"BYE\"";
    if (reason != NULL)
    {
        env << " (reason: \"" << reason << "\")";
    }
    env << " on subsession\n";

    subsessionAfterPlaying(subsession);
}
