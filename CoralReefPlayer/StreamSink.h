#pragma once

#include <functional>
#include "UsageEnvironment.hh"
#include "MediaSink.hh"
#include "MediaSession.hh"
extern "C"
{
#include "libavcodec/avcodec.h"
}

class StreamSink : public MediaSink
{
public:
    using Callback = std::function<void(AVPacket*)>;

    static StreamSink* createNew(UsageEnvironment& env, MediaSubsession& subsession, Callback callback);

private:
    StreamSink(UsageEnvironment& env, MediaSubsession& subsession, Callback callback);
    virtual ~StreamSink();
    virtual Boolean continuePlaying();
    void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
        struct timeval presentationTime, unsigned durationInMicroseconds);
    static void afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
        struct timeval presentationTime, unsigned durationInMicroseconds);

private:
    u_int8_t* fReceiveBuffer;
    MediaSubsession& fSubsession;
    Callback fCallback;
};
