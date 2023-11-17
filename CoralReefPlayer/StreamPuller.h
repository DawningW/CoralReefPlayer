#pragma once

#include <string>
#include <thread>
#include <functional>
#include "GroupsockHelper.hh"
#include "BasicUsageEnvironment.hh"
#include "RTSPClient.hh"
#include "curl/curl.h"
#include "coralreefplayer.h"
#include "AsyncCallback.hpp"
#include "VideoDecoder.h"

class StreamPuller
{
public:
    enum Protocol
    {
        CRP_UNKNOWN,
        CRP_RTSP,
        CRP_HTTP,
    };
    using Callback = std::function<void(int, void*)>;

    StreamPuller();
    ~StreamPuller();
    void authenticate(const char* username, const char* password, bool useMD5 = false);
    bool start(const char* url, Transport transport, int width, int height, Format format, Callback callback);
    void stop();

private:
    void runRTSP();
    void runHTTP();
    void shutdownStream(RTSPClient* rtspClient);
    void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
    void setupNextSubsession(RTSPClient* rtspClient);
    void subsessionAfterPlaying(MediaSubsession* subsession);
    void subsessionByeHandler(MediaSubsession* subsession, char const* reason);
    void timeoutHandler();
    void noteLiveness();
    size_t curlProgressCallback(curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
    static Protocol parseUrl(const std::string& url);

private:
    std::string url;
    Protocol protocol;
    Transport transport;
    int width;
    int height;
    Format format;
    AsyncCallback<int, void*> callback;
    int64_t timeout;

    volatile char exit;
    std::thread thread;
    Authenticator* authenticator;
    TaskScheduler* scheduler;
    UsageEnvironment* environment;
    RTSPClient* rtspClient;
    MediaSession* session;
    MediaSubsession* subsession;
    MediaSubsessionIterator* iter;
    TaskToken livenessCheckTask;
    CURL* curl;
    bool downloading;
    VideoDecoder* videoDecoder;
};
