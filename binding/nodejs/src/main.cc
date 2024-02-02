#include <unordered_map>
#include <napi.h>
#include "coralreefplayer.h"

using namespace Napi;

std::unordered_map<crp_handle, Napi::ThreadSafeFunction> g_callbacks;

void js_callback(int event, void* data, void* user_data) {
    Napi::ThreadSafeFunction& tsfn = g_callbacks[(crp_handle) user_data];
    tsfn.BlockingCall([event, data](Napi::Env env, Napi::Function callback) {
        if (event == CRP_EV_NEW_FRAME) {
            Frame* frame = (Frame*) data;
            Object obj = Object::New(env);
            obj.DefineProperties({
                PropertyDescriptor::Value("width", Napi::Number::New(env, frame->width), napi_enumerable),
                PropertyDescriptor::Value("height", Napi::Number::New(env, frame->height), napi_enumerable),
                PropertyDescriptor::Value("format", Napi::Number::New(env, frame->format), napi_enumerable),
                // Electron 21+ not allow to use external buffer, must copy. See https://github.com/nodejs/node-addon-api/blob/main/doc/external_buffer.md
                PropertyDescriptor::Value("data", Napi::Buffer<uint8_t>::NewOrCopy(
                    env, frame->data[0], frame->linesize[0] * frame->height), napi_enumerable)
            });
            callback.Call({
                Napi::Number::New(env, event),
                obj
            });
        } else {
            callback.Call({
                Napi::Number::New(env, event),
                Napi::Number::New(env, (int) data)
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
    int transport = info[2].As<Napi::Number>().Int32Value();
    int width = info[3].As<Napi::Number>().Int32Value();
    int height = info[4].As<Napi::Number>().Int32Value();
    int format = info[5].As<Napi::Number>().Int32Value();
    Napi::Function callback = info[6].As<Napi::Function>();
    if (g_callbacks.find(handle) != g_callbacks.end()) {
        g_callbacks[handle].Release();
    }
    g_callbacks[handle] = Napi::ThreadSafeFunction::New(
        env, callback, "AsyncCallback", 0, 1, [](Napi::Env) {});
    crp_play(handle, url.c_str(), transport, width, height, format, js_callback, handle);
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

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
