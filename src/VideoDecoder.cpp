#include "VideoDecoder.h"
#include <cstdio>
extern "C"
{
#include "libavutil/opt.h"
#include "libavutil/imgutils.h"
}

#define EXTRADATA_MAX_SIZE (AV_INPUT_BUFFER_PADDING_SIZE * 2)

const uint8_t startCode3[3] = { 0x00, 0x00, 0x01 };
const uint8_t startCode4[4] = { 0x00, 0x00, 0x00, 0x01 };
const uint8_t jpegStartCode[2] = { 0xFF, 0xD8 };
const uint8_t jpegEndCode[2] = { 0xFF, 0xD9 };

static AVPixelFormat to_av_format(Format format)
{
    switch (format)
    {
        case CRP_YUV420P: return AV_PIX_FMT_YUV420P;
        case CRP_NV12: return AV_PIX_FMT_NV12;
        case CRP_NV21: return AV_PIX_FMT_NV21;
        case CRP_RGB24: return AV_PIX_FMT_RGB24;
        case CRP_BGR24: return AV_PIX_FMT_BGR24;
        case CRP_ARGB32: return AV_PIX_FMT_ARGB;
        case CRP_RGBA32: return AV_PIX_FMT_RGBA;
        case CRP_ABGR32: return AV_PIX_FMT_ABGR;
        case CRP_BGRA32: return AV_PIX_FMT_BGRA;
    }
    return AV_PIX_FMT_NONE;
}

static const AVCodec* find_hw_decoder(const std::string& codecName, const std::string& deviceName,
                                        AVHWDeviceType& deviceType, std::string& device)
{
    size_t sep = deviceName.find(':');
    std::string deviceTypeName = sep == std::string::npos ? deviceName : deviceName.substr(0, sep);
    device = sep == std::string::npos ? "auto" : deviceName.substr(sep + 1);
    deviceType = av_hwdevice_find_type_by_name(deviceTypeName.c_str());
    if (deviceType == AV_HWDEVICE_TYPE_NONE)
    {
        fprintf(stderr, "Hardware accel device %s not found.\n", deviceTypeName.c_str());
    }
    std::string decoderName = codecName + "_" + deviceTypeName;
    const AVCodec* codec = avcodec_find_decoder_by_name(decoderName.c_str());
    if (codec == nullptr)
    {
        fprintf(stderr, "Hardware decoder %s not found, use software decoder instead.\n", decoderName.c_str());
    }
    return codec;
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

VideoDecoder* VideoDecoder::createNew(const std::string& codecName, Format format, int width, int height,
                                        const std::string& deviceName)
{
    const AVCodec* codec = nullptr;
    AVHWDeviceType deviceType = AV_HWDEVICE_TYPE_NONE;
    std::string device;
    if (codecName == "H264")
    {
        if (!deviceName.empty())
            codec = find_hw_decoder("h264", deviceName, deviceType, device);
        if (codec == nullptr)
            codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        return new H264VideoDecoder(codec, deviceType, device, format, width, height);
    }
    else if (codecName == "H265")
    {
        if (!deviceName.empty())
            codec = find_hw_decoder("hevc", deviceName, deviceType, device);
        if (codec == nullptr)
            codec = avcodec_find_decoder(AV_CODEC_ID_H265);
        return new H265VideoDecoder(codec, deviceType, device, format, width, height);
    }
    else if (codecName == "JPEG")
    {
        if (!deviceName.empty())
            codec = find_hw_decoder("mjpeg", deviceName, deviceType, device);
        if (codec == nullptr)
            codec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
        return new MJPEGVideoDecoder(codec, deviceType, device, format, width, height);
    }
    return nullptr;
}

VideoDecoder::VideoDecoder(const AVCodec* codec, AVHWDeviceType deviceType, const std::string& device,
                            Format format, int width, int height)
    : codecCtx(nullptr), hwDeviceCtx(nullptr), hwPixFmt(AV_PIX_FMT_NONE),
        hwFrame(nullptr), frame(nullptr), swsCtx(nullptr), outFrame{}
{
    if (deviceType != AV_HWDEVICE_TYPE_NONE)
    {
        if (av_hwdevice_ctx_create(&hwDeviceCtx, deviceType, device.c_str(), NULL, 0) < 0)
        {
            fprintf(stderr, "Open hardware device failed.\n");
            deviceType = AV_HWDEVICE_TYPE_NONE;
        }
        else
        {
            for (int i = 0; ; i++)
            {
                const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
                if (!config)
                    break;
                if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == deviceType)
                {
                    hwPixFmt = config->pix_fmt;
                    break;
                }
            }
            if (hwPixFmt == AV_PIX_FMT_NONE)
            {
                fprintf(stderr, "Decoder %s does not support device type %s.\n",
                    codec->name, av_hwdevice_get_type_name(deviceType));
                av_buffer_unref(&hwDeviceCtx);
                deviceType = AV_HWDEVICE_TYPE_NONE;
            }
        }
    }

    codecCtx = avcodec_alloc_context3(codec);
    codecCtx->opaque = this;
    codecCtx->flags = AV_CODEC_FLAG_LOW_DELAY;
    //codecCtx->flags2 = AV_CODEC_FLAG2_CHUNKS; // BUG have bug when using drm on linux
    codecCtx->extradata = (uint8_t*) av_malloc(EXTRADATA_MAX_SIZE);
    codecCtx->extradata_size = 0;
    if (deviceType != AV_HWDEVICE_TYPE_NONE)
    {
        codecCtx->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
        codecCtx->get_format = [](AVCodecContext* avctx, const AVPixelFormat* pix_fmts) -> AVPixelFormat
        {
            while (*pix_fmts != AV_PIX_FMT_NONE)
            {
                if (*pix_fmts == ((VideoDecoder*) avctx->opaque)->getHwPixFormat())
                {
                    return *pix_fmts;
                }
                pix_fmts++;
            }
            fprintf(stderr, "Failed to get hardware surface format.\n");
            return AV_PIX_FMT_NONE;
        };
        switch (deviceType)
        {
            case AV_HWDEVICE_TYPE_QSV:
                av_opt_set(codecCtx->priv_data, "async_depth", "1", 0);
                break;
            case AV_HWDEVICE_TYPE_CUDA:
                av_opt_set(codecCtx->priv_data, "delay", "0", 0);
                break;
        }
    }

    if (deviceType != AV_HWDEVICE_TYPE_NONE)
       hwFrame = av_frame_alloc();
    frame = av_frame_alloc();

    outFrame.width = width;
    outFrame.height = height;
    outFrame.format = format;
}

VideoDecoder::~VideoDecoder()
{
    if (swsCtx != nullptr && outFrame.data[0] != nullptr)
        av_freep(&outFrame.data[0]);

    if (swsCtx != nullptr)
        sws_freeContext(swsCtx);
    av_frame_free(&frame);
    if (hwFrame != nullptr)
       av_frame_free(&hwFrame);
    // avcodec_free_context will delete extradata
    avcodec_free_context(&codecCtx);
    if (hwDeviceCtx != nullptr)
       av_buffer_unref(&hwDeviceCtx);
}

bool VideoDecoder::processPacket(AVPacket* packet)
{
    if (avcodec_send_packet(codecCtx, packet) < 0)
    {
        fprintf(stderr, "Decode Error.\n");
        return false;
    }

    bool hasFrame = false;
    AVFrame* receivedFrame = hwDeviceCtx == nullptr ? frame : hwFrame;
    while (avcodec_receive_frame(codecCtx, receivedFrame) >= 0)
    {
        if (hwDeviceCtx != nullptr)
        {
            if (av_hwframe_transfer_data(frame, hwFrame, 0) < 0)
            {
                fprintf(stderr, "Error transferring the data to system memory.\n");
                continue;
            }
        }

        if (outFrame.data[0] == nullptr)
        {
            if (outFrame.width == CRP_WIDTH_AUTO && outFrame.height == CRP_HEIGHT_AUTO)
            {
                outFrame.width = frame->width;
                outFrame.height = frame->height;
            }
            AVPixelFormat srcPixFmt;
            switch (frame->format)
            {
                case AV_PIX_FMT_YUVJ420P: srcPixFmt = AV_PIX_FMT_YUV420P; break;
                case AV_PIX_FMT_YUVJ422P: srcPixFmt = AV_PIX_FMT_YUV422P; break;
                case AV_PIX_FMT_YUVJ444P: srcPixFmt = AV_PIX_FMT_YUV444P; break;
                default: srcPixFmt = (enum AVPixelFormat) frame->format; break;
            }
            AVPixelFormat dstPixFmt = to_av_format((enum Format) outFrame.format);
            bool needConvert = (codecCtx->codec->wrapper_name == NULL && hwDeviceCtx == nullptr) || // 软解增加一帧缓冲区
                frame->width != outFrame.width || frame->height != outFrame.height || srcPixFmt != dstPixFmt;
            if (needConvert)
            {
                swsCtx = sws_getContext(frame->width, frame->height, srcPixFmt,
                                        outFrame.width, outFrame.height, dstPixFmt,
                                        SWS_FAST_BILINEAR, NULL, NULL, NULL);
                av_image_alloc(outFrame.data, outFrame.stride, outFrame.width, outFrame.height, dstPixFmt, 1);
            }
            else
            {
                for (int i = 0; i < 4; i++)
                {
                    outFrame.data[i] = frame->data[i];
                    outFrame.stride[i] = frame->linesize[i];
                }
            }
        }

        if (swsCtx != nullptr)
        {
            sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, outFrame.data, outFrame.stride);
        }
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

MJPEGVideoDecoder::MJPEGVideoDecoder(const AVCodec* codec, AVHWDeviceType deviceType, const std::string& device,
                                        Format format, int width, int height)
    : VideoDecoder(codec, deviceType, device, format, width, height)
{
    avcodec_open2(codecCtx, NULL, NULL);
}
