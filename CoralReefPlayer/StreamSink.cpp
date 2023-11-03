#include "StreamSink.h"
#include <cstdio>
#include "VideoDecoder.h"

#define SINK_RECEIVE_BUFFER_SIZE 1000000

StreamSink* StreamSink::createNew(UsageEnvironment& env, MediaSubsession& subsession, Callback callback)
{
    return new StreamSink(env, subsession, callback);
}

StreamSink::StreamSink(UsageEnvironment& env, MediaSubsession& subsession, Callback callback)
    : MediaSink(env), fSubsession(subsession), fCallback(callback)
{
    fReceiveBuffer = new u_int8_t[SINK_RECEIVE_BUFFER_SIZE];
    memcpy(fReceiveBuffer, startCode4, sizeof(startCode4));
    fH264OrH265 = strcmp(subsession.codecName(), "H264") == 0 ||
        strcmp(subsession.codecName(), "H265") == 0;
}

StreamSink::~StreamSink()
{
    delete[] fReceiveBuffer;
}

void StreamSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
    struct timeval presentationTime, unsigned durationInMicroseconds)
{
    StreamSink* sink = (StreamSink*) clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void StreamSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
    struct timeval presentationTime, unsigned durationInMicroseconds)
{
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
    if (fH264OrH265)
    {
        if ((memcmp(buffer, startCode4, sizeof(startCode4)) != 0 &&
            memcmp(buffer, startCode3, sizeof(startCode3)) != 0))
        {
            buffer -= 4;
            frameSize += 4;
        }
    }
    AVPacket packet{};
    packet.data = buffer;
    packet.size = frameSize;
    packet.pts = presentationTime.tv_sec * 1000 + presentationTime.tv_usec / 1000;
    packet.dts = AV_NOPTS_VALUE;
    fCallback(&packet);

    if (!continuePlaying())
        onSourceClosure(this);
}

Boolean StreamSink::continuePlaying()
{
    if (fSource == NULL)
        return False;

    fSource->getNextFrame(fReceiveBuffer + 4, SINK_RECEIVE_BUFFER_SIZE - 4, afterGettingFrame, this, onSourceClosure, this);
    return True;
}
