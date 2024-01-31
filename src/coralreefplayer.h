#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

#ifdef WIN32
#ifdef CORALREEFPLAYER_EXPORTS
#define CRP_DLL_EXPORT __declspec(dllexport)
#else
#define CRP_DLL_EXPORT __declspec(dllimport)
#endif
#else // UNIX
#define CRP_DLL_EXPORT
#endif

#define CRP_WIDTH_AUTO 0
#define CRP_HEIGHT_AUTO 0

enum Transport
{
    CRP_UDP,
    CRP_TCP,
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

enum Event
{
    CRP_EV_NEW_FRAME, // 收到新视频帧事件
    CRP_EV_ERROR,     // 发生错误事件
    CRP_EV_START,     // 开始拉流事件
    CRP_EV_PLAYING,   // 开始播放事件
    CRP_EV_END,       // 播放结束事件
    CRP_EV_STOP,      // 停止拉流事件
};

struct Frame
{
    int width;
    int height;
    enum Format format;
    uint8_t* data[4];
    int linesize[4];
    uint64_t pts;
};

typedef void* crp_handle;
typedef void (*crp_callback)(int /* event */, void* /* data */, void* /* user_data */);

CRP_DLL_EXPORT crp_handle crp_create();
CRP_DLL_EXPORT void crp_destroy(crp_handle handle);
CRP_DLL_EXPORT void crp_auth(crp_handle handle, const char* username, const char* password, bool is_md5);
CRP_DLL_EXPORT void crp_play(crp_handle handle, const char* url, int transport,
    int width, int height, int format, crp_callback callback, void* user_data);
CRP_DLL_EXPORT void crp_replay(crp_handle handle);
CRP_DLL_EXPORT void crp_stop(crp_handle handle);
CRP_DLL_EXPORT int crp_version_code();
CRP_DLL_EXPORT const char* crp_version_str();

#ifdef __cplusplus
}
#endif
