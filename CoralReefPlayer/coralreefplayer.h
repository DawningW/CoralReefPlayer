#pragma once

#include <cstdint>

#ifdef CORALREEFPLAYER_EXPORTS
#define CRP_DLL_EXPORT __declspec(dllexport)
#else
#define CRP_DLL_EXPORT __declspec(dllimport)
#endif

enum class Transport
{
    UDP,
    TCP
};

enum class Format
{
    YUV420P, // AV_PIX_FMT_YUV420P
    RGB24,   // AV_PIX_FMT_RGB24
    BGR24,   // AV_PIX_FMT_BGR24
    ARGB32,  // AV_PIX_FMT_ARGB
    RGBA32,  // AV_PIX_FMT_RGBA
    ABGR32,  // AV_PIX_FMT_ABGR
    BGRA32,  // AV_PIX_FMT_BGRA
};

struct Frame
{
    uint8_t* data[4];
    int linesize[4];
    uint64_t pts;
};

typedef void* crp_handle;
typedef void (*crp_callback)(int /* event */, void* /* data */);

extern "C"
{
    CRP_DLL_EXPORT crp_handle crp_create();
    CRP_DLL_EXPORT void crp_destroy(crp_handle handle);
    CRP_DLL_EXPORT void crp_play(crp_handle handle, const char* url, Transport transport,
        int width, int height, Format format, crp_callback callback);
}
