#include "StreamSink.h"
#include <cstdio>

#define SINK_RECEIVE_BUFFER_SIZE 1000000

static uint8_t startCode4[4] = { 0x00, 0x00, 0x00, 0x01 };

//FILE* pH264;

StreamSink* StreamSink::createNew(UsageEnvironment& env, MediaSubsession& subsession, callback_t callback) {
    return new StreamSink(env, subsession, callback);
}

StreamSink::StreamSink(UsageEnvironment& env, MediaSubsession& subsession, callback_t callback)
    : MediaSink(env), fHasFirstKeyframe(False), fSubsession(subsession), fCallback(callback) {
    fReceiveBuffer = new u_int8_t[SINK_RECEIVE_BUFFER_SIZE];
    //pH264 = fopen("file.h264", "w");
}

StreamSink::~StreamSink() {
    delete[] fReceiveBuffer;
    //fclose(pH264);
}

void StreamSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
    struct timeval presentationTime, unsigned durationInMicroseconds) {
    StreamSink* sink = (StreamSink*) clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void StreamSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
    struct timeval presentationTime, unsigned durationInMicroseconds) {

#ifdef _DEBUG
    envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
    if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
    char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
    sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
    envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
    if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
        envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
    }
    envir() << "\tNormal play time: " << fSubsession.getNormalPlayTime(presentationTime) << "\n";
#endif
    //printf("%#02x %#02x %#02x %#02x %#02x %#02x %#02x %#02x\n",
    //    fReceiveBuffer[0], fReceiveBuffer[1], fReceiveBuffer[2], fReceiveBuffer[3], fReceiveBuffer[4], fReceiveBuffer[5], fReceiveBuffer[6], fReceiveBuffer[7]);
    
    //fwrite(startCode4, 4, 1, pH264);
    //fwrite(fReceiveBuffer, frameSize, 1, pH264);

    AVPacket packet{};
    packet.data = fReceiveBuffer;
    packet.size = frameSize;
    fCallback(this, &packet);

    if (!continuePlaying()) {
        onSourceClosure(this);
    }
}

Boolean StreamSink::continuePlaying() {
    if (fSource == NULL)
        return False;

    fSource->getNextFrame(fReceiveBuffer, SINK_RECEIVE_BUFFER_SIZE, afterGettingFrame, this, onSourceClosure, this);
    return True;
}
