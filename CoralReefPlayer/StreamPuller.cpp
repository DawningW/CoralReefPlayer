#include "StreamPuller.h"
#include "H264VideoRTPSource.hh" // for parseSPropParameterSets
#include "StreamSink.h"

thread_local StreamPuller* StreamPuller::instance;

StreamPuller::StreamPuller() : exit(1), authenticator(nullptr) {}

StreamPuller::~StreamPuller()
{
    stop();
}

void StreamPuller::authenticate(const char* username, const char* password, bool useMD5)
{
    if (authenticator != nullptr)
        delete authenticator;
    authenticator = new Authenticator(username, password, useMD5);
}

bool StreamPuller::start(const char* url, Transport transport, int width, int height, Format format, Callback callback)
{
    if (!exit)
        return false;

    this->url = url;
    this->transport = transport;
    this->width = width;
    this->height = height;
    this->format = format;
    this->callback = callback;

    exit = 0;
    thread = std::thread(&StreamPuller::run, this);

    return true;
}

void StreamPuller::stop()
{
    if (exit)
        return;

    exit = 1;
    thread.join();
    callback = Callback();
}

void StreamPuller::run()
{
    instance = this;

    scheduler = BasicTaskScheduler::createNew();
    environment = BasicUsageEnvironment::createNew(*scheduler);

    rtspClient = RTSPClient::createNew(*environment, url.c_str(), 1, "CoralReefPlayer");
    rtspClient->sendDescribeCommand(continueAfterDESCRIBE, authenticator);
    session = NULL;
    videoDecoder = nullptr;

    environment->taskScheduler().doEventLoop(&exit);

    if (rtspClient != NULL)
        shutdownStream(rtspClient);
    delete scheduler;
    if (videoDecoder != nullptr)
        delete videoDecoder;
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
    this->rtspClient = NULL;
}

void StreamPuller::continueAfterDESCRIBE0(RTSPClient* rtspClient, int resultCode, char* resultString)
{
    UsageEnvironment& env = rtspClient->envir();

    if (resultCode != 0)
    {
        env << "Failed to get a SDP description: " << resultString << "\n";
        return;
    }
    const char* sdpDescription = resultString;
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
    const char* mediumName;
    const char* codecName;

    if (resultCode != 0)
    {
        env << "Failed to set up the subsession: " << resultString << "\n";
        goto end;
    }

    env << "Set up the subsession (" << subsession->clientPortNum() << ")\n";

    mediumName = subsession->mediumName();
    codecName = subsession->codecName();
    if (strcmp(mediumName, "video") == 0)
    {
        videoDecoder = VideoDecoder::createNew(codecName, format, width, height);
        if (videoDecoder == nullptr)
        {
            env << "Not support video codec: " << codecName << "\n";
            goto end;
        }

        if (strcmp(codecName, "H264") == 0)
        {
            const char* sprops = subsession->fmtp_spropparametersets();
            unsigned spropCount;
            SPropRecord* spropRecords = parseSPropParameterSets(sprops, spropCount);
            if (spropCount == 2)
            {
                for (int i = 0; i < spropCount; i++)
                {
                    SPropRecord& record = spropRecords[i];
                    videoDecoder->addExtraData(startCode4, sizeof(startCode4));
                    videoDecoder->addExtraData(record.sPropBytes, record.sPropLength);
                }
                env << "Get SPropRecords.\n";
            }
            if (spropRecords)
                delete[] spropRecords;
        }
        else if (strcmp(codecName, "H265") == 0)
        {
            if (strcmp(subsession->fmtp_spropparametersets(), "") != 0)
            {
                unsigned spropCount;
                SPropRecord* spropRecords = parseSPropParameterSets(subsession->fmtp_spropparametersets(), spropCount);
                if (spropCount == 3)
                {
                    for (int i = 0; i < spropCount; i++)
                    {
                        SPropRecord& record = spropRecords[i];
                        videoDecoder->addExtraData(startCode4, sizeof(startCode4));
                        videoDecoder->addExtraData(record.sPropBytes, record.sPropLength);
                    }
                    env << "Get SPropRecords.\n";
                }
                if (spropRecords)
                    delete[] spropRecords;
            }
            else if (strcmp(subsession->fmtp_spropvps(), "") != 0)
            {
                unsigned spropCounts[3];
                SPropRecord* spropRecords[3];
                spropRecords[0] = parseSPropParameterSets(subsession->fmtp_spropvps(), spropCounts[0]);
                spropRecords[1] = parseSPropParameterSets(subsession->fmtp_spropsps(), spropCounts[1]);
                spropRecords[2] = parseSPropParameterSets(subsession->fmtp_sproppps(), spropCounts[2]);
                for (int i = 0; i < 3; i++)
                {
                    if (spropRecords[i])
                    {
                        SPropRecord& record = spropRecords[i][0];
                        videoDecoder->addExtraData(startCode4, sizeof(startCode4));
                        videoDecoder->addExtraData(record.sPropBytes, record.sPropLength);
                        delete[] spropRecords[i];
                    }
                }
                env << "Get SPropRecords.\n";
            }
        }
    }
    else if (strcmp(mediumName, "audio") == 0)
    {
        env << "Not support audio codec: " << codecName << "\n";
        goto end;
    }

    subsession->sink = StreamSink::createNew(env, *subsession, [this, &env](AVPacket* packet)
    {
        if (videoDecoder->processPacket(packet))
        {
            callback(CRP_EV_NEW_FRAME, videoDecoder->getFrame());
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
