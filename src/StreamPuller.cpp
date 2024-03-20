#include "StreamPuller.h"
#include <cstdio>
#include <cstring>
#include <regex>
#include "H264VideoRTPSource.hh" // for parseSPropParameterSets
#include "MPEG4LATMAudioRTPSource.hh" // for parseGeneralConfigStr
#include "StreamSink.h"

#ifdef _DEBUG
#define LOG_LEVEL 1
#else
#define LOG_LEVEL 0
#endif
#define DEFAULT_TIMEOUT_MS 1000

static const std::string USER_AGENT = std::string("CoralReefPlayer/") + crp_version_str();

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

StreamPuller::StreamPuller() : exit(1), authenticator(NULL) {}

StreamPuller::~StreamPuller()
{
    stop();
    delete authenticator;
}

void StreamPuller::authenticate(const char* username, const char* password, bool useMD5)
{
    delete authenticator;
    authenticator = new Authenticator(username, password, useMD5);
}

bool StreamPuller::start(const char* url, const Option* option, Callback callback, void* userData)
{
    if (!exit)
        return false;

    this->url = url;
    protocol = parseUrl(this->url);
    if (protocol == CRP_UNKNOWN)
        return false;

    if (option != nullptr)
        this->option = *option;
    if (option->timeout == 0)
        this->option.timeout = DEFAULT_TIMEOUT_MS;
    this->callback = callback;
    this->userData = userData;

    start();
    return true;
}

bool StreamPuller::restart()
{
    if (exit || protocol == CRP_UNKNOWN)
        return false;

    stop();
    start();
    return true;
}

void StreamPuller::stop()
{
    if (exit)
        return;

    exit = 1;
    thread.join();
    callback.wait();
    callback.invokeSync(CRP_EV_STOP, nullptr, userData);
    delete videoDecoder;
    delete audioDecoder;
}

void StreamPuller::start()
{
    videoDecoder = nullptr;
    audioDecoder = nullptr;
    exit = 0;
    if (protocol == CRP_RTSP)
        thread = std::thread(&StreamPuller::runRTSP, this);
    else if (protocol == CRP_HTTP)
        thread = std::thread(&StreamPuller::runHTTP, this);
}

void StreamPuller::runRTSP()
{
    scheduler = BasicTaskScheduler::createNew();
    environment = BasicUsageEnvironment::createNew(*scheduler);
    rtspClient = OurRTSPClient::createNew(*environment, url.c_str(), LOG_LEVEL, USER_AGENT.c_str());
    session = NULL;
    livenessCheckTask = NULL;
    
    callback.invokeSync(CRP_EV_START, nullptr, userData);
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
}

void StreamPuller::runHTTP()
{
    static const std::regex urlRegex(R"(([a-z]+:\/\/[^/]*)(\/?.*))");

    std::string host, path;
    try
    {
        std::smatch m;
        if (std::regex_match(url, m, urlRegex))
        {
            host = m[1].str();
            path = m[2].str();
            if (path.empty()) path = "/";
        }
        httpClient = new httplib::Client(host);
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "Failed to create HTTP client: %s\n", e.what());
        callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
        return;
    }

    callback.invokeSync(CRP_EV_START, nullptr, userData);
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httpClient->enable_server_certificate_verification(false);
#endif
    httpClient->set_follow_location(true);
    httpClient->set_default_headers({
        {"User-Agent", USER_AGENT}
    });
    if (authenticator != NULL)
    {
        httpClient->set_basic_auth(authenticator->username(), authenticator->password());
    }
    std::string* body = nullptr;
    HTTPSink sink([this](AVPacket* packet)
        {
            if (videoDecoder->processPacket(packet))
            {
                callback(CRP_EV_NEW_FRAME, videoDecoder->getFrame(), userData);
            }
        });
#ifdef _DEBUG
    printf("> GET %s\n", url.c_str());
#endif
    httplib::Result res = httpClient->Get(path,
        [&](const httplib::Response& rep)
        {
            /*
             * 在标准实现中，header 条目为 Content-Type: multipart/x-mixed-replace; boundary="boundaryValue"
             * 但在某些实现中，会有以下情况：
             * 1. 双引号消失：boundary=boundaryValue
             * 2. 多出分隔符：boundary="--boundaryValue"
             * 3. 其他
             * 此处提取的是最纯粹的 boundaryValue 部分
             *
             * JPEG 图片间标准实现的分隔符为 --boundaryValue
             * 为了防止某些实现中，--消失，本实现仅匹配 boundaryValue
             */
            static const std::regex boundaryRegex(R"===(boundary="?(?:--)?(\S+)"?\n?)===");

#ifdef _DEBUG
            printf("< %d %s\n", rep.status, rep.reason.c_str());
            for (auto& header : rep.headers)
            {
                printf("< %s: %s\n", header.first.c_str(), header.second.c_str());
            }
#endif
            body = const_cast<std::string*>(&rep.body);
            std::string boundary;
            if (rep.has_header("Content-Type"))
            {
                std::string contentType = rep.get_header_value("Content-Type");
                std::cmatch m;
                if (std::regex_search(contentType.c_str(), m, boundaryRegex))
                {
                    boundary = m[1].str();
                }
            }
            if (!boundary.empty())
            {
                sink.setBoundary(boundary);
                videoDecoder = VideoDecoder::createNew("JPEG",
                    (Format) option.video.format, option.video.width, option.video.height, option.video.hw_device);
                callback.invokeSync(CRP_EV_PLAYING, nullptr, userData);
            }
            return true;
        },
        [&](const char* data, size_t length)
        {
            if (videoDecoder == nullptr)
            {
                body->append(data, length);
                return true;
            }
            sink.writeData((const uint8_t*) data, length);
            return !exit;
        });
    if (res.error() == httplib::Error::Success)
    {
        if (videoDecoder != nullptr)
        {
            callback.invokeSync(CRP_EV_END, nullptr, userData);
        }
        else
        {
            fprintf(stderr, "Invalid response with status %d %s:\n", res.value().status, res.value().reason.c_str());
            fputs(res.value().body.c_str(), stderr);
            fputc('\n', stderr);
            callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
        }
    }
    else if (res.error() != httplib::Error::Canceled)
    {
        fprintf(stderr, "Request failed: %s\n", httplib::to_string(res.error()).c_str());
        callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
    }

    delete httpClient;
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

    env << "Closing the stream\n";
    Medium::close(rtspClient);
    this->rtspClient = NULL;
}

void StreamPuller::continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString)
{
    UsageEnvironment& env = rtspClient->envir();

    if (resultCode != 0)
    {
        env << "Failed to get a SDP description: " << resultString << "\n";
        callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
        return;
    }
    const char* sdpDescription = resultString;
    env << "Got a SDP description:\n" << sdpDescription << "\n";

    session = MediaSession::createNew(env, sdpDescription);
    if (session == NULL)
    {
        env << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
        callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
        goto end;
    }
    else if (!session->hasSubsessions())
    {
        env << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
        callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
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
        callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
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
        videoDecoder = VideoDecoder::createNew(codecName,
            (Format) option.video.format, option.video.width, option.video.height, option.video.hw_device);
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
                env << "Get H264 SPropRecords\n";
            }
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
                    env << "Get H265 SPropRecords\n";
                }
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
                env << "Get H265 SPropRecords\n";
            }
        }

        subsession->sink = StreamSink::createNew(env, *subsession, [this](AVPacket* packet)
            {
                noteLiveness();
                if (videoDecoder->processPacket(packet))
                {
                    callback(CRP_EV_NEW_FRAME, videoDecoder->getFrame(), userData);
                }
            });
    }
    else if (strcmp(mediumName, "audio") == 0)
    {
        if (!option.enable_audio)
        {
            env << "Audio decode is not enabled\n";
            goto end;
        }

        audioDecoder = AudioDecoder::createNew(codecName,
            (Format) option.audio.format, option.audio.sample_rate, option.audio.channels);
        if (audioDecoder == nullptr)
        {
            env << "Not support audio codec: " << codecName << "\n";
            goto end;
        }

        if (strcmp(codecName, "MPEG4-GENERIC") == 0 || strcmp(codecName, "OPUS") == 0)
        {
            if (strcmp(subsession->fmtp_config(), "") != 0)
            {
                unsigned configSize = 0;
                unsigned char* config = parseGeneralConfigStr(subsession->fmtp_config(), configSize);
                audioDecoder->addExtraData(config, configSize);
                env << "Get AAC or OPUS config\n";
            }
        }
        else if (strcmp(codecName, "PCMA") == 0 || strcmp(codecName, "PCMU") == 0)
        {
            audioDecoder->initParameters(subsession->rtpTimestampFrequency(), subsession->numChannels());
        }
        else if (strcmp(codecName, "G726") == 0)
        {
            audioDecoder->initParameters(8000, 1);
        }

        subsession->sink = StreamSink::createNew(env, *subsession, [this](AVPacket* packet)
            {
                noteLiveness();
                if (audioDecoder->processPacket(packet))
                {
                    callback(CRP_EV_NEW_AUDIO, audioDecoder->getFrame(), userData);
                }
            });
    }
    else
    {
        env << "Not support medium: " << mediumName << "\n";
        goto end;
    }

    env << "Created a " << mediumName << " data sink for the subsession\n";
    subsession->miscPtr = rtspClient;
    subsession->sink->startPlaying(*(subsession->readSource()), [](void* clientData)
        {
            MediaSubsession* subsession = (MediaSubsession*) clientData;
            ((OurRTSPClient*) subsession->miscPtr)->parent->subsessionAfterPlaying(subsession);
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
        callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
        goto end;
    }

    env << "Started playing session...\n";
    callback.invokeSync(CRP_EV_PLAYING, nullptr, userData);
    noteLiveness();
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
            callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
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
                }, False, option.transport == CRP_TCP);
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
    callback.invokeSync(CRP_EV_END, nullptr, userData);
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

void StreamPuller::timeoutHandler()
{
    UsageEnvironment& env = rtspClient->envir();

    env << "Receive stream timeout after " << (int) option.timeout << " ms\n";
    callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
}

void StreamPuller::noteLiveness()
{
    if (option.timeout > 0)
    {
        rtspClient->envir().taskScheduler().rescheduleDelayedTask(livenessCheckTask, option.timeout * 1000,
            [](void* clientData)
            {
                ((StreamPuller*) clientData)->timeoutHandler();
            }, this);
    }
}

StreamPuller::Protocol StreamPuller::parseUrl(const std::string& url)
{
    if (url.starts_with("rtsp"))
        return CRP_RTSP;
    else if (url.starts_with("http"))
        return CRP_HTTP;
    return CRP_UNKNOWN;
}
