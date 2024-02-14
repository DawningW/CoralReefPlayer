#pragma once

#include <string>
#include <vector>
#include <functional>
#include "UsageEnvironment.hh"
#include "MediaSink.hh"
#include "MediaSession.hh"
#include "VideoDecoder.h" // for AVPacket and start codes
#include "Matcher.hpp"

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
    Boolean fH264OrH265;
};

class HTTPSink
{
public:
    using Callback = StreamSink::Callback;

    HTTPSink(Callback callback);
    ~HTTPSink();
    void setBoundary(const std::string& boundary);
    bool writeData(const uint8_t* data, size_t size);

private:
    Callback onFrame;
    std::vector<uint8_t> buffer;
    Matcher<uint8_t> boundaryMatcher;
    Matcher<uint8_t> startMatcher;
    Matcher<uint8_t> endMatcher;
};
