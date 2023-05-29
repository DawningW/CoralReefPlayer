#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

#ifdef CORALREEFPLAYER_EXPORTS
#define CRP_DLL_EXPORT __declspec(dllexport)
#else
#define CRP_DLL_EXPORT __declspec(dllimport)
#endif

#define CRP_WIDTH_AUTO 0
#define CRP_HEIGHT_AUTO 0
#define CRP_EV_NEW_FRAME 0 // 收到新视频帧事件

enum Transport
{
    CRP_UDP,
    CRP_TCP
};

enum Format
{
    CRP_YUV420P, // AV_PIX_FMT_YUV420P
    CRP_RGB24,   // AV_PIX_FMT_RGB24
    CRP_BGR24,   // AV_PIX_FMT_BGR24
    CRP_ARGB32,  // AV_PIX_FMT_ARGB
    CRP_RGBA32,  // AV_PIX_FMT_RGBA
    CRP_ABGR32,  // AV_PIX_FMT_ABGR
    CRP_BGRA32,  // AV_PIX_FMT_BGRA
};

struct Frame
{
    int width;
    int height;
    Format format;
    uint8_t* data[4];
    int linesize[4];
    uint64_t pts;
};

typedef void* crp_handle;
typedef void (*crp_callback)(int /* event */, void* /* data */);

CRP_DLL_EXPORT crp_handle crp_create();
CRP_DLL_EXPORT void crp_destroy(crp_handle handle);
CRP_DLL_EXPORT void crp_play(crp_handle handle, const char* url, Transport transport,
    int width, int height, Format format, crp_callback callback);

#ifdef __cplusplus
}
#endif
