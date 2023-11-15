#include "StreamPuller.h"
#include <cstdio>
#include <cstring>
#include "H264VideoRTPSource.hh" // for parseSPropParameterSets
#include "StreamSink.h"

class OurRTSPClient : public RTSPClient
{
public:
    static OurRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL, int verbosityLevel = 0,
        char const* applicationName = NULL, portNumBits tunnelOverHTTPPortNum = 0, int socketNumToServer = -1)
    {
        return new OurRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, socketNumToServer);
    }

protected:
    using RTSPClient::RTSPClient;

public:
    StreamPuller* parent;
};

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
    protocol = parseUrl(this->url);
    if (protocol == CRP_UNKNOWN)
        return false;

    this->transport = transport;
    this->width = width;
    this->height = height;
    this->format = format;
    this->callback = callback;

    videoDecoder = nullptr;
    exit = 0;
    if (protocol == CRP_RTSP)
        thread = std::thread(&StreamPuller::runRTSP, this);
    else if (protocol == CRP_HTTP)
        thread = std::thread(&StreamPuller::runHTTP, this);

    return true;
}

void StreamPuller::stop()
{
    if (exit)
        return;

    exit = 1;
    thread.join();
    callback = Callback();
    if (videoDecoder != nullptr)
        delete videoDecoder;
}

void StreamPuller::runRTSP()
{
    scheduler = BasicTaskScheduler::createNew();
    environment = BasicUsageEnvironment::createNew(*scheduler);
    rtspClient = OurRTSPClient::createNew(*environment, url.c_str(), 1, "CoralReefPlayer");
    session = NULL;
    
    callback.invokeSync(CRP_EV_START, nullptr);
    ((OurRTSPClient*) rtspClient)->parent = this;
    rtspClient->sendDescribeCommand([](RTSPClient * rtspClient, int resultCode, char* resultString)
        {
            ((OurRTSPClient*) rtspClient)->parent->continueAfterDESCRIBE(rtspClient, resultCode, resultString);
            delete[] resultString;
        }, authenticator);
    environment->taskScheduler().doEventLoop(&exit);

    if (rtspClient != NULL)
        shutdownStream(rtspClient);
    delete scheduler;
    callback.invokeSync(CRP_EV_STOP, nullptr);
}

void StreamPuller::runHTTP()
{
    HTTPSink sink([this](AVPacket* packet)
        {
            if (videoDecoder->processPacket(packet))
            {
                callback(CRP_EV_NEW_FRAME, videoDecoder->getFrame());
            }
        });
    curl = curl_easy_init();
    downloading = false;
    videoDecoder = VideoDecoder::createNew("JPEG", format, width, height);
    if (curl == NULL)
    {
        callback.invokeSync(CRP_EV_ERROR, (void*) 0);
        return;
    }

    callback.invokeSync(CRP_EV_START, nullptr);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CoralReefPlayer");
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION,
        +[](void* userdata, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
        {
            return ((StreamPuller*) userdata)->curlProgressCallback(dltotal, dlnow, ultotal, ulnow);
        });
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &sink);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION,
        +[](void* contents, size_t size, size_t nmemb, void* userdata)
        {
            size_t realsize = size * nmemb;
            ((HTTPSink*) userdata)->writeHeader((const uint8_t*) contents, realsize);
            return realsize;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
        +[](void* contents, size_t size, size_t nmemb, void* userdata)
        {
            size_t realsize = size * nmemb;
            ((HTTPSink*) userdata)->writeData((const uint8_t*) contents, realsize);
            return realsize;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &sink);
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK)
    {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 0 || response_code == 200)
        {
            callback.invokeSync(CRP_EV_END, nullptr);
        }
        else
        {
            fprintf(stderr, "Invalid response with status code %ld\n", response_code);
            callback.invokeSync(CRP_EV_ERROR, (void*) 0);
        }
    }
    else if (res != CURLE_ABORTED_BY_CALLBACK)
    {
        fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
        callback.invokeSync(CRP_EV_ERROR, (void*) 0);
    }

    curl_easy_cleanup(curl);
    callback.invokeSync(CRP_EV_STOP, nullptr);
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

void StreamPuller::continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString)
{
    UsageEnvironment& env = rtspClient->envir();

    if (resultCode != 0)
    {
        env << "Failed to get a SDP description: " << resultString << "\n";
        callback.invokeSync(CRP_EV_ERROR, (void*) 0);
        return;
    }
    const char* sdpDescription = resultString;
    env << "Got a SDP description:\n" << sdpDescription << "\n";

    session = MediaSession::createNew(env, sdpDescription);
    if (session == NULL)
    {
        env << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
        callback.invokeSync(CRP_EV_ERROR, (void*) 0);
        goto end;
    }
    else if (!session->hasSubsessions())
    {
        env << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
        callback.invokeSync(CRP_EV_ERROR, (void*) 0);
        goto end;
    }

    iter = new MediaSubsessionIterator(*session);
    setupNextSubsession(rtspClient);
    return;

end:
    shutdownStream(rtspClient);
}

void StreamPuller::continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString)
{
    UsageEnvironment& env = rtspClient->envir();
    const char* mediumName;
    const char* codecName;

    if (resultCode != 0)
    {
        env << "Failed to set up the subsession: " << resultString << "\n";
        callback.invokeSync(CRP_EV_ERROR, (void*) 0);
        goto end;
    }

    env << "Set up the subsession (";
    if (subsession->rtcpIsMuxed())
        env << "client port " << subsession->clientPortNum();
    else
        env << "client ports " << subsession->clientPortNum() << "-" << subsession->clientPortNum() + 1;
    env << ")\n";
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
                env << "Get H264 SPropRecords.\n";
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
                    env << "Get H265 SPropRecords.\n";
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
                env << "Get H265 SPropRecords.\n";
            }
        }
    }
    else if (strcmp(mediumName, "audio") == 0)
    {
        env << "Not support audio codec: " << codecName << "\n";
        goto end;
    }

    env << "Created a data sink for the subsession\n";
    subsession->sink = StreamSink::createNew(env, *subsession, [this](AVPacket* packet)
        {
            if (videoDecoder->processPacket(packet))
            {
                callback(CRP_EV_NEW_FRAME, videoDecoder->getFrame());
            }
        });
    subsession->miscPtr = rtspClient;
    subsession->sink->startPlaying(*(subsession->readSource()), [](void* clientData)
        {
            MediaSubsession* subsession = (MediaSubsession*) clientData;
            ((OurRTSPClient*)subsession->miscPtr)->parent->subsessionAfterPlaying(subsession);
        }, subsession);
    if (subsession->rtcpInstance() != NULL)
    {
        subsession->rtcpInstance()->setByeWithReasonHandler([](void* clientData, char const* reason)
            {
                MediaSubsession* subsession = (MediaSubsession*) clientData;
                ((OurRTSPClient*) subsession->miscPtr)->parent->subsessionByeHandler(subsession, reason);
                delete[] reason;
            }, subsession);
    }
    
end:
    setupNextSubsession(rtspClient);
}

void StreamPuller::continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString)
{
    UsageEnvironment& env = rtspClient->envir();

    if (resultCode != 0)
    {
        env << "Failed to start playing session: " << resultString << "\n";
        callback.invokeSync(CRP_EV_ERROR, (void*) 0);
        goto end;
    }

    env << "Started playing session...\n";
    callback.invokeSync(CRP_EV_PLAYING, nullptr);
    return;

end:
    shutdownStream(rtspClient);
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
            callback.invokeSync(CRP_EV_ERROR, (void*) 0);
            setupNextSubsession(rtspClient);
        }
        else
        {
            env << "Initiated the subsession (";
            if (subsession->rtcpIsMuxed())
                env << "client port " << subsession->clientPortNum();
            else
                env << "client ports " << subsession->clientPortNum() << "-" << subsession->clientPortNum() + 1;
            env << ")\n";
            rtspClient->sendSetupCommand(*subsession, [](RTSPClient * rtspClient, int resultCode, char* resultString)
                {
                    ((OurRTSPClient*) rtspClient)->parent->continueAfterSETUP(rtspClient, resultCode, resultString);
                    delete[] resultString;
                }, False, transport == CRP_TCP);
        }
        return;
    }

    rtspClient->sendPlayCommand(*session, [](RTSPClient * rtspClient, int resultCode, char* resultString)
        {
            ((OurRTSPClient*) rtspClient)->parent->continueAfterPLAY(rtspClient, resultCode, resultString);
            delete[] resultString;
        });
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
    callback.invokeSync(CRP_EV_END, nullptr);
}

void StreamPuller::subsessionByeHandler(MediaSubsession* subsession, char const* reason)
{
    UsageEnvironment& env = rtspClient->envir();

    env << "Received RTCP \"BYE\"";
    if (reason != NULL)
        env << " (reason: \"" << reason << "\")";
    env << " on subsession\n";

    subsessionAfterPlaying(subsession);
}

size_t StreamPuller::curlProgressCallback(curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    if (!downloading && dlnow > 0)
    {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code == 0 || response_code == 200)
        {
            downloading = true;
            callback.invokeSync(CRP_EV_PLAYING, nullptr);
        }
    }

#ifdef _DEBUG
    if (!exit)
        return CURL_PROGRESSFUNC_CONTINUE;
#endif
    return exit;
}

StreamPuller::Protocol StreamPuller::parseUrl(const std::string& url)
{
    if (url.starts_with("rtsp"))
        return CRP_RTSP;
    else if (url.starts_with("http") || url.starts_with("file"))
        return CRP_HTTP;
    return CRP_UNKNOWN;
}
