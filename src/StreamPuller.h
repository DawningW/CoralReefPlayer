#pragma once

#include <string>
#include <thread>
#include <functional>
#include "GroupsockHelper.hh"
#include "BasicUsageEnvironment.hh"
#include "RTSPClient.hh"
#include "MPEG2TransportStreamDemux.hh"
#define CPPHTTPLIB_RECV_BUFSIZ size_t(32768u)
#include "httplib.h"
#include "coralreefplayer.h"
#include "AsyncCallback.hpp"
#include "VideoDecoder.h"
#include "AudioDecoder.h"
#include "StreamSink.h"

class StreamPuller
{
public:
    enum Protocol
    {
        CRP_UNKNOWN,
        CRP_RTSP,
        CRP_RTP,
        CRP_HTTP,
    };
    using Callback = std::function<void(int, void*, void*)>;

    StreamPuller();
    ~StreamPuller();
    void authenticate(const char* username, const char* password, bool useMD5 = false);
    bool start(const char* url, const Option* option, Callback callback, void* userData);
    bool restart();
    void stop();

private:
    void start();
    void runRTSP();
    void runRTP();
    void runHTTP();
    void shutdownStream(RTSPClient* rtspClient);
    void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
    void setupNextSubsession(RTSPClient* rtspClient);
    void subsessionAfterPlaying(MediaSubsession* subsession);
    void subsessionByeHandler(MediaSubsession* subsession, char const* reason);
    void createTransportStream(PIDState_STREAM* pidState, StreamType& streamType);
    void timeoutHandler();
    void noteLiveness();
    static Protocol parseUrl(const std::string& url);

private:
    std::string url;
    Protocol protocol;
    Option option;
    AsyncCallback<int, void*, void*> callback;
    void *userData;

    volatile char exit;
    std::thread thread;
    Authenticator* authenticator;
    TaskScheduler* scheduler;
    UsageEnvironment* environment;
    RTSPClient* rtspClient;
    MediaSession* session;
    MediaSubsession* subsession;
    MediaSubsessionIterator* iter;
    MPEG2TransportStreamDemux* demuxer;
    volatile char available; // for reading RTP socket
    TaskToken livenessCheckTask;
    httplib::Client* httpClient;
    VideoDecoder* videoDecoder;
    StreamSink::Callback videoCallback;
    AudioDecoder* audioDecoder;
    StreamSink::Callback audioCallback;
};
