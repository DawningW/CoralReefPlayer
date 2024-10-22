#include <cstring>
#include <unordered_map>
#include <napi.h>
#ifdef __OHOS__
extern "C" {
#include "libavutil/log.h"
}
#endif
#include "coralreefplayer.h"

using namespace Napi;

// The Buffer in HarmonyOS has a limit of 2MB, so we use ArrayBuffer instead.
#ifdef __OHOS__
// HarmonyOS need FinalizeCallback, or throw napi_invalid_arg exception
static void FinalizeCallback(napi_env env, void *finalize_data) {}
#define NAPI_NEW_BUFFER(env, data, length) Napi::ArrayBuffer::New(env, data, length, FinalizeCallback)
#else
#define NAPI_NEW_BUFFER(env, data, length) Napi::Buffer<uint8_t>::NewOrCopy(env, data, length)
#endif

std::unordered_map<crp_handle, Napi::ThreadSafeFunction> g_callbacks;

void js_callback(int event, void* data, void* user_data) {
    Napi::ThreadSafeFunction& tsfn = g_callbacks[(crp_handle) user_data];
    tsfn.BlockingCall([event, data](Napi::Env env, Napi::Function callback) {
        if (event == CRP_EV_NEW_FRAME) {
            Frame* frame = (Frame*) data;
            Napi::Object obj = Napi::Object::New(env);
            obj.DefineProperties({
                PropertyDescriptor::Value("width", Napi::Number::New(env, frame->width), napi_enumerable),
                PropertyDescriptor::Value("height", Napi::Number::New(env, frame->height), napi_enumerable),
                PropertyDescriptor::Value("format", Napi::Number::New(env, frame->format), napi_enumerable),
                PropertyDescriptor::Value("pts", Napi::Number::New(env, frame->pts), napi_enumerable)
            });
            if (frame->format == CRP_YUV420P || frame->format == CRP_NV12 || frame->format == CRP_NV21) {
                Napi::Array data = Napi::Array::New(env);
                Napi::Array stride = Napi::Array::New(env);
                for (int i = 0; i < 4; i++) {
                    if (frame->data[i] == nullptr) {
                        break;
                    }
                    data.Set(i, NAPI_NEW_BUFFER(env, frame->data[i], frame->stride[i] * (frame->height >> !!i)));
                    stride.Set(i, Napi::Number::New(env, frame->stride[i]));
                }
                obj.DefineProperties({
                    PropertyDescriptor::Value("data", data, napi_enumerable),
                    PropertyDescriptor::Value("stride", stride, napi_enumerable)
                });
            } else {
                obj.DefineProperties({
                    // Electron 21+ not allow to use external buffer, must copy.
                    // See https://github.com/nodejs/node-addon-api/blob/main/doc/external_buffer.md
                    PropertyDescriptor::Value("data", NAPI_NEW_BUFFER(
                        env, frame->data[0], frame->stride[0] * frame->height), napi_enumerable),
                    PropertyDescriptor::Value("stride", Napi::Number::New(env, frame->stride[0]), napi_enumerable),
                });
            }
            callback.Call({
                Napi::Number::New(env, event),
                obj
            });
        } else if (event == CRP_EV_NEW_AUDIO) {
            Frame* frame = (Frame*) data;
            Napi::Object obj = Napi::Object::New(env);
            obj.DefineProperties({
                PropertyDescriptor::Value("sample_rate", Napi::Number::New(env, frame->sample_rate), napi_enumerable),
                PropertyDescriptor::Value("channels", Napi::Number::New(env, frame->channels), napi_enumerable),
                PropertyDescriptor::Value("format", Napi::Number::New(env, frame->format), napi_enumerable),
                PropertyDescriptor::Value("data", NAPI_NEW_BUFFER(
                    env, frame->data[0], frame->stride[0]), napi_enumerable),
                PropertyDescriptor::Value("stride", Napi::Number::New(env, frame->stride[0]), napi_enumerable),
                PropertyDescriptor::Value("pts", Napi::Number::New(env, frame->pts), napi_enumerable)
            });
            callback.Call({
                Napi::Number::New(env, event),
                obj
            });
        } else {
            callback.Call({
                Napi::Number::New(env, event),
                Napi::Number::New(env, (uintptr_t) data)
            });
        }
    });
}

Napi::Value Create(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    crp_handle handle = crp_create();
    return Napi::External<void>::New(env, handle);
}

void Destroy(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    crp_handle handle = (crp_handle) info[0].As<Napi::External<void>>().Data();
    crp_destroy(handle);
    if (g_callbacks.find(handle) != g_callbacks.end()) {
        g_callbacks[handle].Release();
        g_callbacks.erase(handle);
    }
}

void Auth(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    crp_handle handle = (crp_handle) info[0].As<Napi::External<void>>().Data();
    std::string username = info[1].As<Napi::String>().Utf8Value();
    std::string password = info[2].As<Napi::String>().Utf8Value();
    bool is_md5 = info[3].As<Napi::Boolean>().Value();
    crp_auth(handle, username.c_str(), password.c_str(), is_md5);
}

void Play(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    crp_handle handle = (crp_handle) info[0].As<Napi::External<void>>().Data();
    std::string url = info[1].As<Napi::String>().Utf8Value();
    Napi::Object obj = info[2].As<Napi::Object>();
    Option option{};
    if (obj.Has("transport")) {
        option.transport = obj.Get("transport").As<Napi::Number>().Int32Value();
    }
    if (obj.Has("width")) {
        option.video.width = obj.Get("width").As<Napi::Number>().Int32Value();
    }
    if (obj.Has("height")) {
        option.video.height = obj.Get("height").As<Napi::Number>().Int32Value();
    }
    if (obj.Has("video_format")) {
        option.video.format = obj.Get("video_format").As<Napi::Number>().Int32Value();
    }
    if (obj.Has("hw_device")) {
        std::string hw_device = obj.Get("hw_device").As<Napi::String>().Utf8Value();
        strncpy(option.video.hw_device, hw_device.c_str(), sizeof(option.video.hw_device));
        option.video.hw_device[sizeof(option.video.hw_device) - 1] = '\0';
    }
    if (obj.Has("enable_audio")) {
        option.enable_audio = obj.Get("enable_audio").As<Napi::Boolean>().Value();
    }
    if (obj.Has("sample_rate")) {
        option.audio.sample_rate = obj.Get("sample_rate").As<Napi::Number>().Int32Value();
    }
    if (obj.Has("channels")) {
        option.audio.channels = obj.Get("channels").As<Napi::Number>().Int32Value();
    }
    if (obj.Has("audio_format")) {
        option.audio.format = obj.Get("audio_format").As<Napi::Number>().Int32Value();
    }
    if (obj.Has("timeout")) {
        option.timeout = obj.Get("timeout").As<Napi::Number>().Int64Value();
    }
    Napi::Function callback = info[3].As<Napi::Function>();
    if (g_callbacks.find(handle) != g_callbacks.end()) {
        g_callbacks[handle].Release();
    }
    g_callbacks[handle] = Napi::ThreadSafeFunction::New(
        env, callback, "AsyncCallback", 0, 1, [](Napi::Env) {});
    crp_play(handle, url.c_str(), &option, js_callback, handle);
}

void Replay(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    crp_handle handle = (crp_handle) info[0].As<Napi::External<void>>().Data();
    crp_replay(handle);
}

void Stop(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    crp_handle handle = (crp_handle) info[0].As<Napi::External<void>>().Data();
    crp_stop(handle);
}

Napi::Value VersionStr(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    const char* version = crp_version_str();
    return Napi::String::New(env, version);
}

Napi::Value VersionCode(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    int code = crp_version_code();
    return Napi::Number::New(env, code);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
#ifdef __OHOS__
    av_log_set_callback(nullptr);
#endif
    exports.Set(Napi::String::New(env, "create"), Napi::Function::New(env, Create));
    exports.Set(Napi::String::New(env, "destroy"), Napi::Function::New(env, Destroy));
    exports.Set(Napi::String::New(env, "auth"), Napi::Function::New(env, Auth));
    exports.Set(Napi::String::New(env, "play"), Napi::Function::New(env, Play));
    exports.Set(Napi::String::New(env, "replay"), Napi::Function::New(env, Replay));
    exports.Set(Napi::String::New(env, "stop"), Napi::Function::New(env, Stop));
    exports.Set(Napi::String::New(env, "versionStr"), Napi::Function::New(env, VersionStr));
    exports.Set(Napi::String::New(env, "versionCode"), Napi::Function::New(env, VersionCode));
    return exports;
}

#ifndef __OHOS__
NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
#else
NODE_API_MODULE(crp_ohos, Init)
#endif
