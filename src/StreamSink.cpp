#include "StreamSink.h"
#include <cstdio>
#include <cstring>
#include "VideoDecoder.h"

#define SINK_RECEIVE_BUFFER_SIZE 1000000
#define HTTP_RECEIVE_BUFFER_SIZE 1500000

StreamSink* StreamSink::createNew(UsageEnvironment& env, const char *mediumName, const char *codecName, Callback callback)
{
    return new StreamSink(env, mediumName, codecName, callback);
}

StreamSink::StreamSink(UsageEnvironment& env, const char *mediumName, const char *codecName, Callback callback)
    : MediaSink(env), fMediumName(mediumName), fCodecName(codecName), fCallback(callback)
{
    fReceiveBuffer = new u_int8_t[SINK_RECEIVE_BUFFER_SIZE];
    memcpy(fReceiveBuffer, startCode4, sizeof(startCode4));
    fH264OrH265 = strcmp(codecName, "H264") == 0 || strcmp(codecName, "H265") == 0;
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
    printDebugInfo(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
    envir() << "\n";
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

void StreamSink::printDebugInfo(unsigned frameSize, unsigned numTruncatedBytes,
    struct timeval presentationTime, unsigned durationInMicroseconds)
{
    envir() << fMediumName << "/" << fCodecName << ":\tReceived " << frameSize << " bytes";
    if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
    char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
    sprintf(uSecsStr, "%06u", (unsigned) presentationTime.tv_usec);
    envir() << ".\tPresentation time: " << (int) presentationTime.tv_sec << "." << uSecsStr;
}

Boolean StreamSink::continuePlaying()
{
    if (fSource == NULL)
        return False;

    fSource->getNextFrame(fReceiveBuffer + 4, SINK_RECEIVE_BUFFER_SIZE - 4, afterGettingFrame, this, onSourceClosure, this);
    return True;
}

SessionStreamSink *SessionStreamSink::createNew(UsageEnvironment &env, MediaSubsession &subsession, Callback callback)
{
    return new SessionStreamSink(env, subsession, callback);
}

SessionStreamSink::SessionStreamSink(UsageEnvironment &env, MediaSubsession &subsession, Callback callback)
    : StreamSink(env, subsession.mediumName(), subsession.codecName(), callback), fSubsession(subsession) {}

void SessionStreamSink::printDebugInfo(unsigned frameSize, unsigned numTruncatedBytes, timeval presentationTime, unsigned durationInMicroseconds)
{
    StreamSink::printDebugInfo(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
    if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP())
    {
        envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
    }
    envir() << "\tNormal play time: " << fSubsession.getNormalPlayTime(presentationTime);
}

HTTPSink::HTTPSink(Callback onFrame) : onFrame(onFrame),
    startMatcher{ std::vector(jpegStartCode, jpegStartCode + sizeof(jpegStartCode)) },
    endMatcher{ std::vector(jpegEndCode, jpegEndCode + sizeof(jpegEndCode)) } {}

HTTPSink::~HTTPSink() {}

void HTTPSink::setBoundary(const std::string& boundary)
{
#ifdef _DEBUG
    printf("MJPEG stream boundary: %s\n", boundary.c_str());
#endif
    boundaryMatcher = Matcher<uint8_t>(std::vector<uint8_t>(boundary.data(), boundary.data() + boundary.size()));
}

bool HTTPSink::writeData(const uint8_t* data, size_t size)
{
#ifdef _DEBUG
    printf("video/mjpeg:\tReceived %u bytes.\n", (uint32_t) size);
#endif

    if (size == 0)
        return false;

    if (buffer.size() + size > HTTP_RECEIVE_BUFFER_SIZE)
    {
        fprintf(stderr, "Received data had overflowed by %u bytes\n",
            (uint32_t) (buffer.size() + size - HTTP_RECEIVE_BUFFER_SIZE));
        buffer.clear();
        return false;
    }

    buffer.insert(buffer.end(), data, data + size);

    boundaryMatcher.reset();
    startMatcher.reset();
    endMatcher.reset();

    uint8_t* start = nullptr;
    uint8_t* end = nullptr;
    uint8_t* p = buffer.data();

    while (p < buffer.data() + buffer.size())
    {
        if (!start && startMatcher.match(*p))
        {
            start = p - startMatcher.size() + 1;
        }
        if (start && endMatcher.match(*p))
        {
            end = p + 1;
            break;
        }
        if (boundaryMatcher.match(*p))
        {
            buffer.erase(buffer.begin(), buffer.begin() + (p - buffer.data() + 1));
            startMatcher.reset();
            endMatcher.reset();
            boundaryMatcher.reset();
        }
        p++;
    }
    if (!start)
    {
        buffer.clear();
        return false;
    }
    if (!end)
    {
        buffer.erase(buffer.begin(), buffer.begin() + (start - buffer.data()));
        return false;
    }

    AVPacket packet{};
    packet.data = start;
    packet.size = end - start;
    packet.pts = AV_NOPTS_VALUE;
    packet.dts = AV_NOPTS_VALUE;
    onFrame(&packet);

    buffer.erase(buffer.begin(), buffer.begin() + (end - buffer.data()));

    return true;
}
