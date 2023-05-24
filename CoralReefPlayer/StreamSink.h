#pragma once

#include <functional>
#include "UsageEnvironment.hh"
#include "MediaSink.hh"
#include "MediaSession.hh"
extern "C"
{
#include "libavcodec/avcodec.h"
}

class StreamSink : public MediaSink {
public:
	using callback_t = std::function<void(StreamSink*, AVPacket*)>;

	static StreamSink* createNew(UsageEnvironment& env, MediaSubsession& subsession, callback_t callback);

private:
	StreamSink(UsageEnvironment& env, MediaSubsession& subsession, callback_t callback);
	virtual ~StreamSink();
	virtual Boolean continuePlaying();
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
		struct timeval presentationTime, unsigned durationInMicroseconds);
	static void afterGettingFrame(void* clientData,
		unsigned frameSize, unsigned numTruncatedBytes,
		struct timeval presentationTime, unsigned durationInMicroseconds);

public:
	Boolean fHasFirstKeyframe;

private:
	u_int8_t* fReceiveBuffer;
	MediaSubsession& fSubsession;
	callback_t fCallback;
};
