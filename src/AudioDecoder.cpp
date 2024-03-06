#include "AudioDecoder.h"
#include <cstdio>
extern "C"
{
#include "libswresample/swresample.h"
}

#define EXTRADATA_MAX_SIZE (AV_INPUT_BUFFER_PADDING_SIZE)

static AVSampleFormat to_av_format(Format format)
{
    switch (format)
    {
        case CRP_U8: return AV_SAMPLE_FMT_U8;
        case CRP_S16: return AV_SAMPLE_FMT_S16;
        case CRP_S32: return AV_SAMPLE_FMT_S32;
        case CRP_F32: return AV_SAMPLE_FMT_FLT;
    }
    return AV_SAMPLE_FMT_NONE;
}

AudioDecoder* AudioDecoder::createNew(const std::string codecName, Format format, int sampleRate, int channels)
{
    const AVCodec* codec = nullptr;
    if (codecName == "MPEG4-GENERIC") // AAC
    {
        codec = avcodec_find_decoder(AV_CODEC_ID_AAC);
    }
    else if (codecName == "OPUS")
    {
        codec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
    }
    else if (codecName == "PCMA") // G711A
    {
        codec = avcodec_find_decoder(AV_CODEC_ID_PCM_ALAW);
    }
    else if (codecName == "PCMU") // G711U
    {
        codec = avcodec_find_decoder(AV_CODEC_ID_PCM_MULAW);
    }
    else if (codecName.starts_with("G726"))
    {
        codec = avcodec_find_decoder(AV_CODEC_ID_ADPCM_G726);
    }
    if (codec == nullptr)
    {
        return nullptr;
    }
    return new AudioDecoder(codec, format, sampleRate, channels);
}

AudioDecoder::AudioDecoder(const AVCodec* codec, Format format, int sampleRate, int channels)
    : codecCtx(nullptr), frame(nullptr), outFrame{}
{
    codecCtx = avcodec_alloc_context3(codec);
    codecCtx->extradata = (uint8_t*) av_malloc(EXTRADATA_MAX_SIZE);
    codecCtx->extradata_size = 0;
    frame = av_frame_alloc();

    outFrame.sample_rate = sampleRate;
    outFrame.channels = channels;
    outFrame.format = format;
}

AudioDecoder::~AudioDecoder()
{
    if (outFrame.data[0] != nullptr)
        av_freep(&outFrame.data[0]);

    av_frame_free(&frame);
    avcodec_free_context(&codecCtx);
}

bool AudioDecoder::processPacket(AVPacket* packet)
{
    if (state == 0)
    {
        if (avcodec_open2(codecCtx, NULL, NULL) < 0)
        {
            return false;
        }
        state = 1;
    }

    if (avcodec_send_packet(codecCtx, packet) < 0)
    {
        fprintf(stderr, "Decode Error.\n");
        return false;
    }

    bool hasFrame = false;
    while (avcodec_receive_frame(codecCtx, frame) >= 0)
    {
        if (outFrame.data[0] == nullptr)
        {
            av_samples_alloc(outFrame.data, outFrame.linesize, outFrame.channels, 1024, to_av_format((Format) outFrame.format), 0);
        }

        SwrContext* swrCtx = swr_alloc_set_opts(nullptr,
            av_get_default_channel_layout(outFrame.channels), to_av_format((Format) outFrame.format), outFrame.sample_rate,
            av_get_default_channel_layout(frame->channels), (AVSampleFormat) frame->format, frame->sample_rate,
            0, nullptr);
        swr_init(swrCtx);
        int outSamples = swr_convert(swrCtx, outFrame.data, frame->nb_samples, (const uint8_t**) frame->data, frame->nb_samples);
        swr_free(&swrCtx);
        outFrame.linesize[0] = outSamples * av_get_bytes_per_sample(to_av_format((Format) outFrame.format)) * outFrame.channels;
        outFrame.pts = frame->pts;
        hasFrame = true;
    }
    return hasFrame;
}

void AudioDecoder::initParameters(int sampleRate, int channels)
{
    codecCtx->sample_rate = sampleRate;
    codecCtx->channels = channels;
    //codecCtx->bits_per_coded_sample = 16;
}

void AudioDecoder::addExtraData(const uint8_t* data, int size)
{
    if (codecCtx->extradata_size + size > EXTRADATA_MAX_SIZE)
    {
        fprintf(stderr, "Decoder extradata overflowed.\n");
        return;
    }
    memcpy(codecCtx->extradata + codecCtx->extradata_size, data, size);
    codecCtx->extradata_size += size;
}

Frame* AudioDecoder::getFrame()
{
    return &outFrame;
}
