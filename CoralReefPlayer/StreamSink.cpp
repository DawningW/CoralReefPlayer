#include "StreamSink.h"
#include <cstdio>

#define SINK_RECEIVE_BUFFER_SIZE 1000000

static const uint8_t startCode4[4] = { 0x00, 0x00, 0x00, 0x01 };

//FILE* pH264;

StreamSink* StreamSink::createNew(UsageEnvironment& env, MediaSubsession& subsession, Callback callback) {
    return new StreamSink(env, subsession, callback);
}

StreamSink::StreamSink(UsageEnvironment& env, MediaSubsession& subsession, Callback callback)
    : MediaSink(env), fHasFirstKeyframe(False), fSubsession(subsession), fCallback(callback) {
    fReceiveBuffer = new u_int8_t[SINK_RECEIVE_BUFFER_SIZE];
    memcpy(fReceiveBuffer, startCode4, sizeof(startCode4));
    //pH264 = fopen("stream.h264", "w");
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

    uint8_t* buffer = fReceiveBuffer + 4;
    if (!(buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 1) &&
        !(buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 1)) {
        buffer -= 4;
        frameSize += 4;
    }
    //printf("%#02x %#02x %#02x %#02x %#02x %#02x %#02x %#02x\n",
    //    buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
    //fwrite(buffer, frameSize, 1, pH264);

    AVPacket packet{};
    packet.data = buffer;
    packet.size = frameSize;
    packet.pts = presentationTime.tv_sec * 1000 + presentationTime.tv_usec / 1000;
    fCallback(this, &packet);

    if (!continuePlaying()) {
        onSourceClosure(this);
    }
}

Boolean StreamSink::continuePlaying() {
    if (fSource == NULL)
        return False;

    fSource->getNextFrame(fReceiveBuffer + 4, SINK_RECEIVE_BUFFER_SIZE - 4, afterGettingFrame, this, onSourceClosure, this);
    return True;
}
