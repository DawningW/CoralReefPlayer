#include "StreamPuller.h"
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <regex>
#ifdef __EMSCRIPTEN__
#include <strings.h>
#include <emscripten.h>
#include <emscripten/fetch.h>
#endif
#include "SimpleRTPSource.hh"
#include "H264VideoRTPSource.hh" // for parseSPropParameterSets
#include "MPEG4LATMAudioRTPSource.hh" // for parseGeneralConfigStr
#include "MPEG2IndexFromTransportStream.hh" // for TRANSPORT_PACKET_SIZE
#include "MPEG2TransportStreamParser.hh" // internal header for PIDState_STREAM and StreamType
#include "H264VideoStreamFramer.hh"
#include "H265VideoStreamFramer.hh"
#include "Base64.hh"
#ifndef __EMSCRIPTEN__
#define CPPHTTPLIB_RECV_BUFSIZ size_t(32768u)
#include "httplib.h"
#endif
#include "StreamSink.h"

#ifdef _DEBUG
#define LOG_LEVEL 1
#else
#define LOG_LEVEL 0
#endif
#define DEFAULT_TIMEOUT_MS 1000

static const std::string USER_AGENT = std::string("CoralReefPlayer/") + crp_version_str();
static const unsigned MAX_TS_FRAME_SIZE = 7 * TRANSPORT_PACKET_SIZE;

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

class MPEG2TransportStreamSource : public SimpleRTPSource
{
public:
    static MPEG2TransportStreamSource* createNew(UsageEnvironment& env, Groupsock* RTPgs,
        unsigned char rtpPayloadFormat, unsigned rtpTimestampFrequency = 90000)
    {
        return new MPEG2TransportStreamSource(env, RTPgs, rtpPayloadFormat, rtpTimestampFrequency, "video/MP2T", 0, False);
    }

protected:
    using SimpleRTPSource::SimpleRTPSource;

public:
    virtual unsigned maxFrameSize() const
    {
        // Must specify maxFrameSize, or fCurParserIndex + numBytesNeeded > BANK_SIZE never becomes true
        // causing StreamParser can't swap bank and run out of memory eventually.
        // http://lists.live555.com/pipermail/live-devel/2021-March/021907.html
        return MAX_TS_FRAME_SIZE;
    }
};

StreamPuller::StreamPuller() : exit(1), authenticator(NULL)
{
    videoCallback = [this](AVPacket* packet)
        {
            noteLiveness();
            Stream stream;
            stream.is_audio = false;
            stream.data = packet->data;
            stream.size = packet->size;
            stream.pts = packet->pts;
            callback.invokeSync(CRP_EV_RAW_STREAM, &stream, userData);

            bool inited = videoDecoder->getFrame()->data[0] != nullptr;
            if (videoDecoder->processPacket(packet))
            {
                if (!inited)
                {
                    int size = 0;
                    const uint8_t* extraData = videoDecoder->getExtraData(size);
                    if (extraData && size > 0)
                    {
                        EventData eventData;
                        eventData.extra_data.data = extraData;
                        eventData.extra_data.size = size;
                        callback.invokeSync(CRP_EV_VIDEO_EXTRADATA, &eventData, userData);
                    }
                }
                callback(CRP_EV_NEW_FRAME, videoDecoder->getFrame(), userData);
            }
        };
    audioCallback = [this](AVPacket* packet)
        {
            noteLiveness();
            Stream stream;
            stream.is_audio = true;
            stream.data = packet->data;
            stream.size = packet->size;
            stream.pts = packet->pts;
            callback.invokeSync(CRP_EV_RAW_STREAM, &stream, userData);

            bool inited = audioDecoder->getFrame()->data[0] != nullptr;
            if (audioDecoder->processPacket(packet))
            {
                if (!inited)
                {
                    int size = 0;
                    const uint8_t* extraData = audioDecoder->getExtraData(size);
                    if (extraData && size > 0)
                    {
                        EventData eventData;
                        eventData.extra_data.data = extraData;
                        eventData.extra_data.size = size;
                        callback.invokeSync(CRP_EV_AUDIO_EXTRADATA, &eventData, userData);
                    }
                }
                callback(CRP_EV_NEW_AUDIO, audioDecoder->getFrame(), userData);
            }
        };
}

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
    if (this->option.timeout == 0) // option may be null pointer
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

    available = 1;
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
        thread = CRPThread(&StreamPuller::runRTSP, this);
    else if (protocol == CRP_SDP)
        thread = CRPThread(&StreamPuller::runSDP, this);
    else if (protocol == CRP_RTP)
        thread = CRPThread(&StreamPuller::runRTP, this);
    else if (protocol == CRP_HTTP)
        thread = CRPThread(&StreamPuller::runHTTP, this);
}

void StreamPuller::runRTSP()
{
    scheduler = BasicTaskScheduler::createNew();
    environment = BasicUsageEnvironment::createNew(*scheduler);
    rtspClient = OurRTSPClient::createNew(*environment, url.c_str(), LOG_LEVEL, USER_AGENT.c_str());
    session = NULL;
    demuxer = NULL;
    livenessCheckTask = NULL;

    callback.invokeSync(CRP_EV_START, nullptr, userData);
    ((OurRTSPClient*) rtspClient)->parent = this;
    rtspClient->sendDescribeCommand([](RTSPClient* rtspClient, int resultCode, char* resultString)
        {
            ((OurRTSPClient*) rtspClient)->parent->continueAfterDESCRIBE(rtspClient, resultCode, resultString);
            delete[] resultString;
        }, authenticator);
    environment->taskScheduler().doEventLoop(&exit);

    if (rtspClient != NULL)
        shutdownStream(rtspClient);
    delete scheduler;
}

static bool parseSDP(const std::string& url, std::string& sdp)
{
    static const std::regex regex(R"(([a-z\d\+]+):\/\/(.+))");
    std::smatch match;
    if (std::regex_match(url, match, regex))
    {
        std::string schema = match[1].str();
        sdp = match[2].str();
        if (schema == "sdp+base64")
        {
            unsigned dataSize = 0;
            unsigned char* data = base64Decode(sdp.c_str(), sdp.length(), dataSize, true);
            sdp = std::string((char*) data, dataSize);
        }
        else if (schema != "sdp")
        {
            sdp = "";
            return false;
        }
        return !sdp.empty();
    }
    return false;
}

void StreamPuller::runSDP()
{
    std::string sdp;
    if (!parseSDP(url, sdp))
    {
        fprintf(stderr, "Invalid SDP description: %s\n", url.c_str());
        callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
        return;
    }

    scheduler = BasicTaskScheduler::createNew();
    environment = BasicUsageEnvironment::createNew(*scheduler);
    rtspClient = NULL;
    session = NULL;
    demuxer = NULL;
    livenessCheckTask = NULL;

    callback.invokeSync(CRP_EV_START, nullptr, userData);
    continueAfterDESCRIBE(NULL, 0, sdp.c_str());
    environment->taskScheduler().doEventLoop(&exit);

    shutdownStream(NULL);
    delete scheduler;
}

static bool parseRTPUrl(const std::string& url, std::string& host, int& port)
{
    static const std::regex regex(R"(rtp://([^:]+):(\d+))");
    std::smatch match;
    if (std::regex_match(url, match, regex))
    {
        host = match[1].str();
        port = std::stoi(match[2].str());
        return true;
    }
    return false;
}

void StreamPuller::runRTP()
{
    std::string host;
    int port;
    if (!parseRTPUrl(url, host, port)) {
        fprintf(stderr, "Invalid RTP multicast url: %s\n", url.c_str());
        callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
        return;
    }

    scheduler = BasicTaskScheduler::createNew();
    environment = BasicUsageEnvironment::createNew(*scheduler);
    livenessCheckTask = NULL;

    callback.invokeSync(CRP_EV_START, nullptr, userData);
#if ANDROID || __OHOS__
    // Android and HarmonyOS can't receive multicast from all interfaces at the same time
    // So we have to specify one, usually the "wlan0" interface
    ReceivingInterfaceAddr = ourIPv4Address(*environment);
    memcpy(ReceivingInterfaceAddr6.s6_addr, ourIPv6Address(*environment), sizeof(ipv6AddressBits));
#endif
    NetAddressList sessionAddresses(host.c_str());
    struct sockaddr_storage sessionAddress;
    copyAddress(sessionAddress, sessionAddresses.firstAddress());
    Port rtpPort(port);
    Groupsock rtpSocket(*environment, sessionAddress, rtpPort, 1);

    unsigned char payloadType = 0;
    FramedSource* source = NULL;
    demuxer = NULL;
    MediaSink *sink = NULL;
    {
        unsigned char packet[1500];
        unsigned packetSize;
        struct sockaddr_storage fromAddress;
        *environment << "Waiting for first RTP packet to determine payload type...\n";

        available = 0;
        environment->taskScheduler().setBackgroundHandling(rtpSocket.socketNum(), SOCKET_READABLE,
            [](void* clientData, int mask)
            {
                ((StreamPuller*) clientData)->available = 1;
            }, this);
        environment->taskScheduler().doEventLoop(&available);
        environment->taskScheduler().disableBackgroundHandling(rtpSocket.socketNum());
        if (exit) goto end;

        if (!rtpSocket.handleRead(packet, sizeof(packet), packetSize, fromAddress)) {
            *environment << "Failed to read RTP packet\n";
            callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
            goto end;
        }

        payloadType = packet[1] & 0x7F;
    }

    if (payloadType == 33) {
        source = MPEG2TransportStreamSource::createNew(*environment, &rtpSocket, payloadType);
        demuxer = MPEG2TransportStreamDemux::createNew(*environment, source,
            [](PIDState_STREAM* pidState, StreamType& streamType, void* clientData)
            {
                ((StreamPuller*) clientData)->createTransportStream(pidState, streamType);
            }, this, NULL, NULL);
    } else if (payloadType >= 96 && payloadType <= 99) {
        source = H264VideoRTPSource::createNew(*environment, &rtpSocket, payloadType);

        const char* codecName = "H264"; // only support H.264 now
        videoDecoder = VideoDecoder::createNew(codecName,
            (Format) option.video.format, option.video.width, option.video.height, option.video.hw_device);
        sink = StreamSink::createNew(*environment, "video", codecName, videoCallback);

        *environment << "Created a video data sink for the RTP stream\n";
        sink->startPlaying(*source, NULL, NULL);
    } else {
        *environment << "Unknown or unsupported RTP payload type: " << (int) payloadType << "\n";
        goto end;
    }
    *environment << "Detected RTP payload type: " << source->MIMEtype() << " (" << (int) payloadType << ")\n";

    callback.invokeSync(CRP_EV_PLAYING, nullptr, userData);
    environment->taskScheduler().doEventLoop(&exit);

    Medium::close(sink);
    Medium::close(source);
    Medium::close(demuxer);
end:
    delete scheduler;
}

static std::string parseBoundary(const char* contentType)
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
    static const std::regex regex(R"===(boundary="?(?:--)?(\S+)"?\n?)===");
    std::cmatch match;
    if (std::regex_search(contentType, match, regex))
    {
        return match[1].str();
    }
    return "";
}

void StreamPuller::runHTTP()
{
    scheduler = NULL;
    environment = NULL;

#ifndef __EMSCRIPTEN__
    static const std::regex urlRegex(R"(([a-z]+:\/\/[^/]*)(\/?.*))");
    std::string host, path;
    httplib::Client* httpClient = nullptr;
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
    HTTPSink sink(videoCallback);
#ifdef _DEBUG
    printf("> GET %s\n", url.c_str());
#endif
    httplib::Result res = httpClient->Get(path,
        [&](const httplib::Response& rep)
        {
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
                boundary = parseBoundary(contentType.c_str());
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
#else
    callback.invokeSync(CRP_EV_START, nullptr, userData);
    struct FetchData {
        bool cancelled;
        StreamPuller* puller;
        std::string body;
        HTTPSink sink;
        emscripten_fetch_t* fetch;
    } data = { false, this, std::string(), HTTPSink(videoCallback), NULL };
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.userData = &data;
    attr.onsuccess = [](emscripten_fetch_t* fetch)
        {
            FetchData* data = (FetchData*) fetch->userData;
            if (data->puller->videoDecoder != nullptr)
            {
                data->puller->callback.invokeSync(CRP_EV_END, nullptr, data->puller->userData);
            }
            else
            {
                fprintf(stderr, "Invalid response with status %d %s:\n", fetch->status, fetch->statusText);
                fputs(data->body.c_str(), stderr);
                fputc('\n', stderr);
                data->puller->callback.invokeSync(CRP_EV_ERROR, (void*) 0, data->puller->userData);
            }
            data->cancelled = true;
        };
    attr.onerror = [](emscripten_fetch_t* fetch)
        {
            FetchData* data = (FetchData*) fetch->userData;
            if (!data->cancelled && !data->puller->exit)
            {
                printf("Fetch failed: %d %s\n", fetch->status, fetch->statusText);
                data->puller->callback.invokeSync(CRP_EV_ERROR, (void*) 0, data->puller->userData);
                data->cancelled = true;
            }
        };
    attr.onprogress = [](emscripten_fetch_t* fetch)
        {
            FetchData* data = (FetchData*) fetch->userData;
            if (data->cancelled || data->puller->exit)
                return;
            if (data->puller->videoDecoder == nullptr)
            {
                data->body.append(fetch->data, fetch->numBytes);
                return;
            }
            data->sink.writeData((const uint8_t*) fetch->data, fetch->numBytes);
        };
    attr.onreadystatechange = [](emscripten_fetch_t* fetch)
        {
            FetchData* data = (FetchData*) fetch->userData;
#ifdef _DEBUG
            printf("< Ready State: %d\n", fetch->readyState);
#endif
            if (fetch->readyState == 2) // HEADERS_RECEIVED
            {
#ifdef _DEBUG
                printf("< %d %s\n", fetch->status, fetch->statusText);
#endif
                std::string boundary;
                size_t headers_length = emscripten_fetch_get_response_headers_length(fetch);
                char* headers_str = new char[headers_length + 1];
                emscripten_fetch_get_response_headers(fetch, headers_str, headers_length + 1);
                char** headers = emscripten_fetch_unpack_response_headers(headers_str);
                for (char** header = headers; *header != NULL && *(header + 1) != NULL; header += 2)
                {
                    char* key = *header;
                    char* value = *(header + 1);
#ifdef _DEBUG
                    printf("< %s: %s\n", key, value);
#endif
                    if (strcasecmp(key, "Content-Type") == 0)
                    {
                        boundary = parseBoundary(value);
                    }
                }
                emscripten_fetch_free_unpacked_response_headers(headers);
                delete[] headers_str;
                if (!boundary.empty())
                {
                    Option& option = data->puller->option;
                    data->sink.setBoundary(boundary);
                    data->puller->videoDecoder = VideoDecoder::createNew("JPEG",
                        (Format) option.video.format, option.video.width, option.video.height, option.video.hw_device);
                    data->puller->callback.invokeSync(CRP_EV_PLAYING, nullptr, data->puller->userData);
                }
            }
        };
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_STREAM_DATA;
    if (authenticator != NULL)
    {
        attr.userName = authenticator->username();
        attr.password = authenticator->password();
    }
#ifdef _DEBUG
    printf("> %s %s\n", attr.requestMethod, url.c_str());
#endif
    data.fetch = emscripten_fetch(&attr, url.c_str());
#ifdef __EMSCRIPTEN_PTHREADS__
    emscripten_set_main_loop_arg([](void* arg)
    {
        FetchData* data = (FetchData*) arg;
        if (!data->cancelled && !data->puller->exit)
            return;
        emscripten_fetch_close(data->fetch);
        emscripten_cancel_main_loop();
    }, &data, 0, true);
#else
    while (!data.cancelled && !exit)
    {
        emfiber_pthread_yield();
    }
    emscripten_fetch_close(data.fetch);
#endif
#endif
}

void StreamPuller::shutdownStream(RTSPClient* rtspClient)
{
    UsageEnvironment& env = *environment;

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

        if (rtspClient != NULL && someSubsessionsWereActive)
        {
            rtspClient->sendTeardownCommand(*session, NULL);
        }
    }

    env << "Closing the stream\n";
    Medium::close(rtspClient);
    this->rtspClient = NULL;
}

void StreamPuller::continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, const char* resultString)
{
    UsageEnvironment& env = *environment;

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
    UsageEnvironment& env = *environment;
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

    if (strcmp(codecName, "MP2T") == 0)
    {
        subsession->readSource()->setMaxFrameSize(MAX_TS_FRAME_SIZE); // See note in MPEG2TransportStreamSource::maxFrameSize()
        demuxer = MPEG2TransportStreamDemux::createNew(env, subsession->readSource(),
            [](PIDState_STREAM* pidState, StreamType& streamType, void* clientData)
            {
                ((StreamPuller*) clientData)->createTransportStream(pidState, streamType);
            }, this, NULL, NULL);
    }
    else if (strcmp(mediumName, "video") == 0)
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

        subsession->sink = SessionStreamSink::createNew(env, *subsession, videoCallback);
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

        subsession->sink = SessionStreamSink::createNew(env, *subsession, audioCallback);
    }
    else
    {
        env << "Not support medium: " << mediumName << "\n";
        goto end;
    }

    subsession->miscPtr = this;
    if (subsession->sink != NULL)
    {
        env << "Created a " << mediumName << " data sink for the subsession\n";
        subsession->sink->startPlaying(*(subsession->readSource()), [](void* clientData)
            {
                MediaSubsession* subsession = (MediaSubsession*) clientData;
                ((StreamPuller*) subsession->miscPtr)->subsessionAfterPlaying(subsession);
            }, subsession);
    }
    if (subsession->rtcpInstance() != NULL)
    {
        subsession->rtcpInstance()->setByeWithReasonHandler([](void* clientData, char const* reason)
            {
                MediaSubsession* subsession = (MediaSubsession*) clientData;
                ((StreamPuller*) subsession->miscPtr)->subsessionByeHandler(subsession, reason);
                delete[] reason;
            }, subsession);
    }

end:
    setupNextSubsession(rtspClient);
}

void StreamPuller::continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString)
{
    UsageEnvironment& env = *environment;

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
    UsageEnvironment& env = *environment;

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
            if (rtspClient != NULL)
            {
                rtspClient->sendSetupCommand(*subsession, [](RTSPClient* rtspClient, int resultCode, char* resultString)
                    {
                        ((OurRTSPClient*) rtspClient)->parent->continueAfterSETUP(rtspClient, resultCode, resultString);
                        delete[] resultString;
                    }, False, option.transport == CRP_TCP);
            }
            else
            {
                continueAfterSETUP(NULL, 0, NULL);
            }
        }
        return;
    }

    if (rtspClient != NULL)
    {
        rtspClient->sendPlayCommand(*session, [](RTSPClient* rtspClient, int resultCode, char* resultString)
            {
                ((OurRTSPClient*) rtspClient)->parent->continueAfterPLAY(rtspClient, resultCode, resultString);
                delete[] resultString;
            });
    }
    else
    {
        continueAfterPLAY(NULL, 0, NULL);
    }
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
    UsageEnvironment& env = *environment;

    env << "Received RTCP \"BYE\"";
    if (reason != NULL)
        env << " (reason: \"" << reason << "\")";
    env << " on subsession\n";

    subsessionAfterPlaying(subsession);
}

void StreamPuller::createTransportStream(PIDState_STREAM *pidState, StreamType &streamType)
{
    static const std::unordered_map<uint8_t, const char*> STREAM_TYPE_CODEC_MAP =
    {
        {0x0F, "MPEG4-GENERIC"},
        {0x1B, "H264"},
        {0x24, "H265"},
    };

    UsageEnvironment& env = *environment;

    env << "Setup PES stream type " << (int) pidState->stream_type << " with PID " << (int) pidState->PID << "\n";
    const char* mediumName = streamType.dataType == StreamType::AUDIO ? "audio" :
                             streamType.dataType == StreamType::VIDEO ? "video" :
                             streamType.dataType == StreamType::DATA ? "data" :
                             streamType.dataType == StreamType::TEXT ? "text" :
                             "unknown";
    const char* codecName = STREAM_TYPE_CODEC_MAP.contains(pidState->stream_type) ?
                            STREAM_TYPE_CODEC_MAP.at(pidState->stream_type) : "";

    FramedSource *inputSource = pidState->streamSource;
    if (streamType.dataType == StreamType::VIDEO)
    {
        videoDecoder = VideoDecoder::createNew(codecName,
            (Format) option.video.format, option.video.width, option.video.height, option.video.hw_device);
        if (videoDecoder == nullptr)
        {
            env << "Not support video codec: " << streamType.description << "\n";
            return;
        }

        if (strcmp(codecName, "H264") == 0)
        {
            inputSource = H264VideoStreamFramer::createNew(env, inputSource);
        }
        else if (strcmp(codecName, "H265") == 0)
        {
            inputSource = H265VideoStreamFramer::createNew(env, inputSource);
        }

        pidState->streamSink = StreamSink::createNew(env, "ts/video", codecName, videoCallback);
    }
    else
    {
        env << "Not support medium: " << mediumName << "\n";
        return;
    }

    env << "Created a " << mediumName << " data sink for the PES stream\n";
    pidState->streamSink->startPlaying(*inputSource, NULL, NULL);
}

void StreamPuller::timeoutHandler()
{
    UsageEnvironment& env = *environment;

    env << "Receive stream timeout after " << (int) option.timeout << " ms\n";
    callback.invokeSync(CRP_EV_ERROR, (void*) 0, userData);
}

void StreamPuller::noteLiveness()
{
    if (environment == NULL) return;
    if (option.timeout > 0)
    {
        environment->taskScheduler().rescheduleDelayedTask(livenessCheckTask, option.timeout * 1000,
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
    else if (url.starts_with("sdp"))
        return CRP_SDP;
    else if (url.starts_with("rtp"))
        return CRP_RTP;
    else if (url.starts_with("http"))
        return CRP_HTTP;
    return CRP_UNKNOWN;
}
