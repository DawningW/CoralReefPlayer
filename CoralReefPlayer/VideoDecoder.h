#pragma once

#include <string>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/hwcontext.h"
}
#include "coralreefplayer.h"

extern const uint8_t startCode3[3];
extern const uint8_t startCode4[4];

class VideoDecoder
{
public:
    VideoDecoder(const AVCodec* codec, Format format, int width, int height);
    virtual ~VideoDecoder();
    virtual bool processPacket(AVPacket* packet);
    void addExtraData(const uint8_t* data, int size);
    Frame* getFrame();
    static VideoDecoder* createNew(const std::string codecName, Format format, int width, int height);

protected:
    AVCodecContext* codecCtx;
    AVFrame* frame;

    Frame outFrame;
};

class H264VideoDecoder : public VideoDecoder
{
public:
    using VideoDecoder::VideoDecoder;
    virtual bool processPacket(AVPacket* packet);

private:
    int state = 0;
};

class H265VideoDecoder : public VideoDecoder
{
public:
    using VideoDecoder::VideoDecoder;
    virtual bool processPacket(AVPacket* packet);

private:
    int state = 0;
};

class MJPEGVideoDecoder : public VideoDecoder
{
public:
    MJPEGVideoDecoder(const AVCodec* codec, Format format, int width, int height);
};
