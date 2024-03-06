#pragma once

#include <string>
extern "C"
{
#include "libavcodec/avcodec.h"
}
#include "coralreefplayer.h"

class AudioDecoder
{
public:
    AudioDecoder(const AVCodec* codec, Format format, int sampleRate, int channels);
    virtual ~AudioDecoder();
    virtual bool processPacket(AVPacket* packet);
    void initParameters(int sampleRate, int channels);
    void addExtraData(const uint8_t* data, int size);
    Frame* getFrame();
    static AudioDecoder* createNew(const std::string codecName, Format format, int sampleRate, int channels);

private:
    AVCodecContext* codecCtx;
    AVFrame* frame;
    int state = 0;

    Frame outFrame;
};
