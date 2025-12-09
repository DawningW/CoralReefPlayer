#include <string>
#include <array>
#include <unordered_map>
#include <optional>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/threading.h>
#include <emscripten/proxying.h>
#ifndef __EMSCRIPTEN_PTHREADS__
extern "C"
{
#include "emft-pthread.h"
}
#endif
#include "coralreefplayer.h"

using namespace emscripten;

struct JSOption {
    std::optional<Transport> transport;
    std::optional<int> width;
    std::optional<int> height;
    std::optional<Format> video_format;
    std::optional<std::string> hw_device;
    std::optional<bool> enable_audio;
    std::optional<int> sample_rate;
    std::optional<int> channels;
    std::optional<Format> audio_format;
    std::optional<int64_t> timeout;
};

struct JSFrame {
    int width;
    int height;
    int sample_rate;
    int channels;
    Format format;
    val data;
    int stride[4];
    uint64_t pts;
};

#ifdef __EMSCRIPTEN_PTHREADS__
ProxyingQueue g_queue;
#else
int g_player_count = 0;
#endif
std::unordered_map<crp_handle, val> g_callbacks;

void js_callback(int event, void* data, void* user_data) {
#ifdef __EMSCRIPTEN_PTHREADS__
    g_queue.proxySync(emscripten_main_runtime_thread_id(), [&] {
#endif
        val callback = g_callbacks[(crp_handle) user_data];
        if (event == CRP_EV_NEW_FRAME) {
            Frame* cFrame = (Frame*) data;
            JSFrame frame{};
            frame.width = cFrame->width;
            frame.height = cFrame->height;
            frame.format = (Format) cFrame->format;
            frame.pts = cFrame->pts;
            if (frame.format == CRP_YUV420P || frame.format == CRP_NV12 || frame.format == CRP_NV21) {
                val data = val::array();
                for (int i = 0; i < 4; i++) {
                    if (cFrame->data[i] == nullptr) {
                        break;
                    }
                    data.set(i, val(typed_memory_view(frame.height >> !!i * cFrame->stride[i], cFrame->data[i])));
                    frame.stride[i] = cFrame->stride[i];
                }
                frame.data = data;
            } else {
                frame.data = val(typed_memory_view(frame.height * cFrame->stride[0], cFrame->data[0]));
                frame.stride[0] = cFrame->stride[0];
            }
            callback((Event) event, frame);
        } else if (event == CRP_EV_NEW_AUDIO) {
            Frame* cFrame = (Frame*) data;
            JSFrame frame{};
            frame.sample_rate = cFrame->sample_rate;
            frame.channels = cFrame->channels;
            frame.format = (Format) cFrame->format;
            frame.pts = cFrame->pts;
            frame.data = val(typed_memory_view(cFrame->stride[0], cFrame->data[0]));
            frame.stride[0] = cFrame->stride[0];
            callback((Event) event, frame);
        } else if (event == CRP_EV_VIDEO_EXTRADATA || event == CRP_EV_AUDIO_EXTRADATA) {
            EventData* ed = (EventData*) data;
            callback((Event) event, val(typed_memory_view(ed->extra_data.size, ed->extra_data.data)));
        } else {
            callback((Event) event, (uintptr_t) data);
        }
#ifdef __EMSCRIPTEN_PTHREADS__
    });
#endif
}

EMSCRIPTEN_BINDINGS(coralreefplayer) {
    enum_<Transport>("Transport")
        .value("UDP", CRP_UDP)
        .value("TCP", CRP_TCP);
    enum_<Format>("Format")
        .value("YUV420P", CRP_YUV420P)
        .value("NV12", CRP_NV12)
        .value("NV21", CRP_NV21)
        .value("RGB24", CRP_RGB24)
        .value("BGR24", CRP_BGR24)
        .value("ARGB32", CRP_ARGB32)
        .value("RGBA32", CRP_RGBA32)
        .value("ABGR32", CRP_ABGR32)
        .value("BGRA32", CRP_BGRA32)
        .value("U8", CRP_U8)
        .value("S16", CRP_S16)
        .value("S32", CRP_S32)
        .value("F32", CRP_F32);
    enum_<Event>("Event")
        .value("NEW_FRAME", CRP_EV_NEW_FRAME)
        .value("ERROR", CRP_EV_ERROR)
        .value("START", CRP_EV_START)
        .value("PLAYING", CRP_EV_PLAYING)
        .value("END", CRP_EV_END)
        .value("STOP", CRP_EV_STOP)
        .value("NEW_AUDIO", CRP_EV_NEW_AUDIO)
        .value("VIDEO_EXTRADATA", CRP_EV_VIDEO_EXTRADATA)
        .value("AUDIO_EXTRADATA", CRP_EV_AUDIO_EXTRADATA);
    value_object<JSOption>("Option")
        .field("transport", &JSOption::transport)
        .field("width", &JSOption::width)
        .field("height", &JSOption::height)
        .field("video_format", &JSOption::video_format)
        .field("hw_device", &JSOption::hw_device)
        .field("enable_audio", &JSOption::enable_audio)
        .field("sample_rate", &JSOption::sample_rate)
        .field("channels", &JSOption::channels)
        .field("audio_format", &JSOption::audio_format)
        .field("timeout", &JSOption::timeout);
    value_object<JSFrame>("Frame")
        .field("width", &JSFrame::width)
        .field("height", &JSFrame::height)
        .field("sample_rate", &JSFrame::sample_rate)
        .field("channels", &JSFrame::channels)
        .field("format", &JSFrame::format)
        .field("data", &JSFrame::data)
        .field("stride", &JSFrame::stride)
        .field("pts", &JSFrame::pts);
    value_array<std::array<int, 4>>("array_int_4")
        .element(emscripten::index<0>())
        .element(emscripten::index<1>())
        .element(emscripten::index<2>())
        .element(emscripten::index<3>());
    function("create", +[]() {
#ifndef __EMSCRIPTEN_PTHREADS__
        if (g_player_count == 0) {
            emscripten_set_main_loop([]() {
                emfiber_pthread_yield();
            }, 0, false);
        }
        g_player_count++;
#endif
        return (uintptr_t) crp_create();
    }, return_value_policy::reference());
    function("destroy", +[](uintptr_t handle) {
#ifndef __EMSCRIPTEN_PTHREADS__
        if (g_player_count > 0) {
            g_player_count--;
            if (g_player_count == 0) {
                emscripten_cancel_main_loop();
            }
        }
#endif
        crp_destroy((crp_handle) handle);
        g_callbacks.erase((crp_handle) handle);
    }, allow_raw_pointers());
    function("auth", +[](uintptr_t handle, std::string username, std::string password, bool is_md5) {
        crp_auth((crp_handle) handle, username.c_str(), password.c_str(), is_md5);
    }, allow_raw_pointers());
    function("play", +[](uintptr_t handle, std::string url, JSOption &option, val callback) {
        Option cOption{};
        if (option.transport.has_value()) {
            cOption.transport = option.transport.value();
        }
        if (option.width.has_value()) {
            cOption.video.width = option.width.value();
        }
        if (option.height.has_value()) {
            cOption.video.height = option.height.value();
        }
        if (option.video_format.has_value()) {
            cOption.video.format = option.video_format.value();
        }
        if (option.hw_device.has_value()) {
            std::strncpy(cOption.video.hw_device, option.hw_device->c_str(), sizeof(cOption.video.hw_device) - 1);
        }
        if (option.enable_audio.has_value()) {
            cOption.enable_audio = option.enable_audio.value();
        }
        if (option.sample_rate.has_value()) {
            cOption.audio.sample_rate = option.sample_rate.value();
        }
        if (option.channels.has_value()) {
            cOption.audio.channels = option.channels.value();
        }
        if (option.audio_format.has_value()) {
            cOption.audio.format = option.audio_format.value();
        }
        if (option.timeout.has_value()) {
            cOption.timeout = option.timeout.value();
        }
        g_callbacks[(crp_handle) handle] = callback;
        crp_play((crp_handle) handle, url.c_str(), &cOption, js_callback, (crp_handle) handle);
    }, allow_raw_pointers());
    function("replay", +[](uintptr_t handle) {
        return crp_replay((crp_handle) handle);
    }, allow_raw_pointers());
    function("stop", +[](uintptr_t handle) {
        return crp_stop((crp_handle) handle);
    }, allow_raw_pointers());
    function("version_code", &crp_version_code);
    function("version_str", +[]() {
        return std::string(crp_version_str());
    });
    register_optional<int>();
    register_optional<int64_t>();
    register_optional<bool>();
    register_optional<std::string>();
    register_optional<Transport>();
    register_optional<Format>();
}
