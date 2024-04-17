#pragma once

#include <string>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/hwcontext.h"
#include "libswscale/swscale.h"
}
#include "coralreefplayer.h"

extern const uint8_t startCode3[3];
extern const uint8_t startCode4[4];
extern const uint8_t jpegStartCode[2];
extern const uint8_t jpegEndCode[2];

class VideoDecoder
{
public:
    VideoDecoder(const AVCodec* codec, AVHWDeviceType deviceType, const std::string& device,
                    Format format, int width, int height);
    virtual ~VideoDecoder();
    virtual bool processPacket(AVPacket* packet);
    void addExtraData(const uint8_t* data, int size);
    AVPixelFormat getHwPixFormat() { return hwPixFmt; }
    Frame* getFrame();
    static VideoDecoder* createNew(const std::string& codecName, Format format, int width, int height,
                                    const std::string& deviceName = "");

protected:
    AVCodecContext* codecCtx;
    AVBufferRef* hwDeviceCtx;
    AVPixelFormat hwPixFmt;
    AVFrame* hwFrame;
    AVFrame* frame;
    SwsContext* swsCtx;

    Frame outFrame;
};

class H264VideoDecoder : public VideoDecoder
{
public:
    using VideoDecoder::VideoDecoder;
    virtual bool processPacket(AVPacket* packet) override;

private:
    int state = 0;
};

class H265VideoDecoder : public VideoDecoder
{
public:
    using VideoDecoder::VideoDecoder;
    virtual bool processPacket(AVPacket* packet) override;

private:
    int state = 0;
};

class MJPEGVideoDecoder : public VideoDecoder
{
public:
    MJPEGVideoDecoder(const AVCodec* codec, AVHWDeviceType deviceType, const std::string& device,
                        Format format, int width, int height);
};
