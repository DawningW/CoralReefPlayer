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
    struct StreamMemory
    {
        uint8_t* memory;
        size_t size;
        size_t capacity;
    };
    using Callback = std::function<void(int, void*)>;

    StreamPuller();
    ~StreamPuller();
    void authenticate(const char* username, const char* password, bool useMD5 = false);
    bool start(const char* url, Transport transport, int width, int height, Format format, Callback callback);
    void stop();
    static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString)
    {
        instance->continueAfterDESCRIBE0(rtspClient, resultCode, resultString);
        delete[] resultString;
    }
    static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString)
    {
        instance->continueAfterSETUP0(rtspClient, resultCode, resultString);
        delete[] resultString;
    }
    static void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString)
    {
        instance->continueAfterPLAY0(rtspClient, resultCode, resultString);
        delete[] resultString;
    }
    static void subsessionAfterPlaying(void* clientData)
    {
        instance->subsessionAfterPlaying((MediaSubsession*) clientData);
    }
    static void subsessionByeHandler(void* clientData, char const* reason)
    {
        instance->subsessionByeHandler((MediaSubsession*) clientData, reason);
        delete[] reason;
    }
    static size_t curlWriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userdata)
    {
        return instance->curlWriteMemoryCallback0(contents, size, nmemb, (StreamMemory*) userdata);
    }

private:
    void runRTSP();
    void runHTTP();
    void shutdownStream(RTSPClient* rtspClient);
    void continueAfterDESCRIBE0(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterSETUP0(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterPLAY0(RTSPClient* rtspClient, int resultCode, char* resultString);
    void setupNextSubsession(RTSPClient* rtspClient);
    void subsessionAfterPlaying(MediaSubsession* subsession);
    void subsessionByeHandler(MediaSubsession* subsession, char const* reason);
    size_t curlWriteMemoryCallback0(void* contents, size_t size, size_t nmemb, StreamMemory* mem);
    static Protocol parseUrl(const std::string& url);

private:
    std::string url;
    Protocol protocol;
    Transport transport;
    int width;
    int height;
    Format format;
    AsyncCallback<int, void*> callback;

    volatile char exit;
    std::thread thread;
    Authenticator* authenticator;
    TaskScheduler* scheduler;
    UsageEnvironment* environment;
    RTSPClient* rtspClient;
    MediaSession* session;
    MediaSubsession* subsession;
    MediaSubsessionIterator* iter;
    CURL* curl;
    VideoDecoder* videoDecoder;

    thread_local static StreamPuller* instance;
};
