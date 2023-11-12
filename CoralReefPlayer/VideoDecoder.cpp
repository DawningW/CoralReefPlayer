#include "VideoDecoder.h"
#include <cstdio>
extern "C"
{
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
}

#define EXTRADATA_MAX_SIZE (AV_INPUT_BUFFER_PADDING_SIZE * 2)

const uint8_t startCode3[3] = { 0x00, 0x00, 0x01 };
const uint8_t startCode4[4] = { 0x00, 0x00, 0x00, 0x01 };

static AVPixelFormat to_av_format(Format format)
{
    switch (format)
    {
        case CRP_YUV420P: return AV_PIX_FMT_YUV420P;
        case CRP_RGB24: return AV_PIX_FMT_RGB24;
        case CRP_BGR24: return AV_PIX_FMT_BGR24;
        case CRP_ARGB32: return AV_PIX_FMT_ARGB;
        case CRP_RGBA32: return AV_PIX_FMT_RGBA;
        case CRP_ABGR32: return AV_PIX_FMT_ABGR;
        case CRP_BGRA32: return AV_PIX_FMT_BGRA;
    }
    return AV_PIX_FMT_NONE;
}

static bool extractH264Or5Frame(AVPacket* pkt, int& offset, int& size, int& markerSize)
{
    offset += size;
    if (offset >= pkt->size)
        return false;
    int tempOffset = offset;
    // 两种起始码有可能混用, 所以需要同时判断
    if (memcmp(pkt->data + offset, startCode4, sizeof(startCode4)) == 0 ||
        memcmp(pkt->data + offset, startCode3, sizeof(startCode3)) == 0)
    {
        markerSize = memcmp(pkt->data + offset, startCode4, sizeof(startCode4)) == 0 ?
            sizeof(startCode4) : sizeof(startCode3);
        tempOffset += markerSize;
        while (pkt->size - tempOffset >= sizeof(startCode4))
        {
            if (memcmp(pkt->data + tempOffset, startCode4, sizeof(startCode4)) == 0 ||
                memcmp(pkt->data + tempOffset, startCode3, sizeof(startCode3)) == 0)
            {
                size = tempOffset - offset;
                return true;
            }
            tempOffset++;
        }
        size = pkt->size - offset;
        return true;
    }
    else
    {
        markerSize = 4;
        while (pkt->size - tempOffset >= 4)
        {
            unsigned char* p = (unsigned char*) &size;
            p[3] = pkt->data[offset];
            p[2] = pkt->data[offset + 1];
            p[1] = pkt->data[offset + 2];
            p[0] = pkt->data[offset + 3];
            return true;
        }
    }
    return false;
}

VideoDecoder* VideoDecoder::createNew(const std::string codecName, Format format, int width, int height)
{
    const AVCodec* codec = nullptr;
    if (codecName == "H264")
    {
        codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        // codec = avcodec_find_decoder_by_name("h264");
        return new H264VideoDecoder(codec, format, width, height);
    }
    else if (codecName == "H265")
    {
        codec = avcodec_find_decoder(AV_CODEC_ID_H265);
        return new H265VideoDecoder(codec, format, width, height);
    }
    else if (codecName == "JPEG")
    {
        codec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
        return new MJPEGVideoDecoder(codec, format, width, height);
    }
    return nullptr;
}

VideoDecoder::VideoDecoder(const AVCodec* codec, Format format, int width, int height)
    : codecCtx(nullptr), frame(nullptr), outFrame{}
{
    codecCtx = avcodec_alloc_context3(codec);
    codecCtx->flags = AV_CODEC_FLAG_LOW_DELAY;
    codecCtx->flags2 = AV_CODEC_FLAG2_CHUNKS;
    codecCtx->extradata = (uint8_t*) av_malloc(EXTRADATA_MAX_SIZE);
    codecCtx->extradata_size = 0;
    frame = av_frame_alloc();

    outFrame.width = width;
    outFrame.height = height;
    outFrame.format = format;
}

VideoDecoder::~VideoDecoder()
{
    if (outFrame.data[0] != nullptr)
        av_freep(&outFrame.data[0]);

    av_frame_free(&frame);
    // avcodec_free_context will delete extradata
    avcodec_free_context(&codecCtx);
}

bool VideoDecoder::processPacket(AVPacket* packet)
{
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
            if (outFrame.width == CRP_WIDTH_AUTO && outFrame.height == CRP_HEIGHT_AUTO)
            {
                outFrame.width = codecCtx->width;
                outFrame.height = codecCtx->height;
            }
            av_image_alloc(outFrame.data, outFrame.linesize, outFrame.width, outFrame.height,
                to_av_format(outFrame.format), 1);
        }

        AVPixelFormat pixFmt;
        switch (codecCtx->pix_fmt)
        {
            case AV_PIX_FMT_YUVJ420P: pixFmt = AV_PIX_FMT_YUV420P; break;
            case AV_PIX_FMT_YUVJ422P: pixFmt = AV_PIX_FMT_YUV422P; break;
            case AV_PIX_FMT_YUVJ444P: pixFmt = AV_PIX_FMT_YUV444P; break;
            default: pixFmt = codecCtx->pix_fmt; break;
        }
        struct SwsContext* swsCxt = sws_getContext(codecCtx->width, codecCtx->height, pixFmt,
            outFrame.width, outFrame.height, to_av_format(outFrame.format), SWS_FAST_BILINEAR, NULL, NULL, NULL);
        sws_scale(swsCxt, frame->data, frame->linesize, 0, codecCtx->height, outFrame.data, outFrame.linesize);
        sws_freeContext(swsCxt);
        outFrame.pts = frame->pts;
        hasFrame = true;
    }
    return hasFrame;
}

void VideoDecoder::addExtraData(const uint8_t* data, int size)
{
    if (codecCtx->extradata_size + size > EXTRADATA_MAX_SIZE)
    {
        fprintf(stderr, "Decoder extradata overflowed.\n");
        return;
    }
    memcpy(codecCtx->extradata + codecCtx->extradata_size, data, size);
    codecCtx->extradata_size += size;
}

Frame* VideoDecoder::getFrame()
{
    return &outFrame;
}

bool H264VideoDecoder::processPacket(AVPacket* packet)
{
    if (state != 3)
    {
        if (state == 0 && codecCtx->extradata_size > 0)
        {
            state = 2;
        }

        int offset = 0;
        int size = 0;
        int marker_size = 0;
        while (extractH264Or5Frame(packet, offset, size, marker_size))
        {
            int nal_type = packet->data[offset + marker_size] & 0x1F;
            if (nal_type == 7) // SPS
            {
                if (state == 0)
                {
                    addExtraData(packet->data + offset, size);
                    state = 1;
                }
            }
            else if (nal_type == 8) // PPS
            {
                if (state == 1)
                {
                    addExtraData(packet->data + offset, size);
                    state = 2;
                }
            }
            else if (nal_type == 5) // IDR
            {
                if (state == 2)
                {
                    AVDictionary* opts = NULL;
                    av_dict_set(&opts, "preset", "ultrafast", 0);
                    av_dict_set(&opts, "tune", "zerolatency", 0);
                    //av_dict_set(&opts, "strict", "1", 0);
                    if (avcodec_open2(codecCtx, NULL, &opts) < 0)
                    {
                        av_dict_free(&opts);
                        return false;
                    }
                    av_dict_free(&opts);
                    state = 3;
                }
            }
        }

        if (state != 3)
        {
            printf("Waiting for the first keyframe, state: %d.\n", state);
            return false;
        }
    }

    codecCtx->has_b_frames = 0; // 丢帧使reordering buffer增大导致延迟变高
    return VideoDecoder::processPacket(packet);
}

bool H265VideoDecoder::processPacket(AVPacket* packet)
{
    if (state != 4)
    {
        if (state == 0 && codecCtx->extradata_size > 0)
        {
            state = 3;
        }

        int offset = 0;
        int size = 0;
        int marker_size = 0;
        while (extractH264Or5Frame(packet, offset, size, marker_size))
        {
            int nal_type = (packet->data[offset + marker_size] & 0x7E) >> 1;
            if (nal_type == 32) // VPS
            {
                if (state == 0)
                {
                    addExtraData(packet->data + offset, size);
                    state = 1;
                }
            }
            else if (nal_type == 33) // SPS
            {
                if (state == 1)
                {
                    addExtraData(packet->data + offset, size);
                    state = 2;
                }
            }
            else if (nal_type == 34) // PPS
            {
                if (state == 2)
                {
                    addExtraData(packet->data + offset, size);
                    state = 3;
                }
            }
            else if (nal_type >= 16 && nal_type <= 21) // IDR_W_RADL 19 / IDR_N_LP 20
            {
                if (state == 3)
                {
                    AVDictionary* opts = NULL;
                    av_dict_set(&opts, "preset", "ultrafast", 0);
                    av_dict_set(&opts, "tune", "zerolatency", 0);
                    if (avcodec_open2(codecCtx, NULL, &opts) < 0)
                    {
                        av_dict_free(&opts);
                        return false;
                    }
                    av_dict_free(&opts);
                    state = 4;
                }
            }
        }

        if (state != 4)
        {
            printf("Waiting for the first keyframe, state: %d.\n", state);
            return false;
        }
    }

    codecCtx->has_b_frames = 0;
    return VideoDecoder::processPacket(packet);
}

MJPEGVideoDecoder::MJPEGVideoDecoder(const AVCodec* codec, Format format, int width, int height)
    : VideoDecoder(codec, format, width, height)
{
    avcodec_open2(codecCtx, NULL, NULL);
}
