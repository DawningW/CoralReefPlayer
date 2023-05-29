#pragma once

#include <memory>
#include <thread>
#include <functional>
extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}
#include "GroupsockHelper.hh"
#include "BasicUsageEnvironment.hh"
#include "RTSPClient.hh"
#include "coralreefplayer.h"

class StreamPuller
{
public:
    using Callback = std::function<void(int, void*)>;

    StreamPuller();
    ~StreamPuller();
    bool start(const char* url, Transport transport, int width, int height, Format format, Callback callback);
    void stop();
    static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
        instance->continueAfterDESCRIBE0(rtspClient, resultCode, resultString);
        delete[] resultString;
    }
    static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
        instance->continueAfterSETUP0(rtspClient, resultCode, resultString);
        delete[] resultString;
    }
    static void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
        instance->continueAfterPLAY0(rtspClient, resultCode, resultString);
        delete[] resultString;
    }
    static void subsessionAfterPlaying(void* clientData) {
        instance->subsessionAfterPlaying((MediaSubsession*) clientData);
    }
    static void subsessionByeHandler(void* clientData, char const* reason) {
        instance->subsessionByeHandler((MediaSubsession*) clientData, reason);
        delete[] reason;
    }

private:
    void initCodec();
    void initBuffer();
    void run();
    void runCallback();
    void shutdownStream(RTSPClient* rtspClient);
    void continueAfterDESCRIBE0(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterSETUP0(RTSPClient* rtspClient, int resultCode, char* resultString);
    void continueAfterPLAY0(RTSPClient* rtspClient, int resultCode, char* resultString);
    void setupNextSubsession(RTSPClient* rtspClient);
    void subsessionAfterPlaying(MediaSubsession* subsession);
    void subsessionByeHandler(MediaSubsession* subsession, char const* reason);

private:
    AVCodec* codec;
    AVCodecContext* codecCtx;
    AVFrame* frame;

    TaskScheduler* scheduler;
    UsageEnvironment* environment;
    RTSPClient* rtspClient;
    MediaSession* session;
    MediaSubsession* subsession;
    MediaSubsessionIterator* iter;
    MediaSink* sink;

    std::unique_ptr<char[]> url;
    Transport transport;
    Frame outFrame;
    Callback callback;

    volatile char exit;
    std::thread thread;
    std::thread callbackThread;
    std::atomic_flag signal;

    thread_local static StreamPuller* instance;
};
