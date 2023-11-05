#pragma once

#include <string>
#include <thread>
#include <functional>
#include "GroupsockHelper.hh"
#include "BasicUsageEnvironment.hh"
#include "RTSPClient.hh"
#include "coralreefplayer.h"
#include "AsyncCallback.hpp"
#include "VideoDecoder.h"

class StreamPuller
{
public:
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

private:
    void run();
    void shutdownStream(RTSPClient* rtspClient);
    void continueAfterDESCRIBE0(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterSETUP0(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterPLAY0(RTSPClient* rtspClient, int resultCode, char* resultString);
    void setupNextSubsession(RTSPClient* rtspClient);
    void subsessionAfterPlaying(MediaSubsession* subsession);
    void subsessionByeHandler(MediaSubsession* subsession, char const* reason);

private:
    std::string url;
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
    VideoDecoder* videoDecoder;

    thread_local static StreamPuller* instance;
};
