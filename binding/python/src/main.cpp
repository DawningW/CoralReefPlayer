#include <cstring>
#include <unordered_map>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include "coralreefplayer.h"

namespace py = pybind11;

std::unordered_map<crp_handle, py::function> g_callbacks;

void py_callback(int event, void* data, void* user_data) {
    py::gil_scoped_acquire gil;
    if (event == CRP_EV_NEW_FRAME) {
        Frame* frame = (Frame*) data;
        py::object obj = py::dict();
        obj["width"] = frame->width;
        obj["height"] = frame->height;
        obj["format"] = frame->format;
        if (frame->format == CRP_YUV420P) {
            py::list data;
            for (int i = 0; i < 3; i++) {
                data.append(py::memoryview::from_buffer(
                    frame->data[i], {frame->height >> !!i, frame->width >> !!i}, {frame->stride[i], 1}, true
                ));
            }
            obj["data"] = data;
        } else if (frame->format == CRP_NV12 || frame->format == CRP_NV21) {
            py::list data;
            for (int i = 0; i < 2; i++) {
                data.append(py::memoryview::from_buffer(
                    frame->data[i], {frame->height >> !!i, frame->width >> !!i, i + 1}, {frame->stride[i], i + 1, 1}, true
                ));
            }
            obj["data"] = data;
        } else {
            int pb = frame->format >= CRP_RGB24 && frame->format <= CRP_BGR24 ? 3 : 4;
            obj["data"] = py::memoryview::from_buffer(
                frame->data[0], {frame->height, frame->width, pb}, {frame->stride[0], pb, 1}, true
            );
        }
        obj["pts"] = frame->pts;
        g_callbacks[(crp_handle) user_data](event, obj);
    } else if (event == CRP_EV_NEW_AUDIO) {
        Frame* frame = (Frame*) data;
        py::object obj = py::dict();
        obj["sample_rate"] = frame->sample_rate;
        obj["channels"] = frame->channels;
        obj["format"] = frame->format;
        obj["data"] = py::memoryview::from_memory(
            frame->data[0], frame->stride[0], true
        );
        obj["pts"] = frame->pts;
        g_callbacks[(crp_handle) user_data](event, obj);
    } else {
        g_callbacks[(crp_handle) user_data](event, data);
    }
}

PYBIND11_MODULE(extension, m) {
    m.def("create", &crp_create, py::return_value_policy::reference);

    m.def("destroy", [](crp_handle handle) {
        crp_destroy(handle);
        g_callbacks.erase(handle);
    }, py::call_guard<py::gil_scoped_release>());

    m.def("auth", &crp_auth);

    m.def("play", [](crp_handle handle, const char* url, py::dict option, const py::function &callback) {
        Option opt{};
        if (option.contains("transport")) {
            opt.transport = option["transport"].cast<int>();
        }
        if (option.contains("width")) {
            opt.video.width = option["width"].cast<int>();
        }
        if (option.contains("height")) {
            opt.video.height = option["height"].cast<int>();
        }
        if (option.contains("video_format")) {
            opt.video.format = option["video_format"].cast<int>();
        }
        if (option.contains("hw_device")) {
            std::string hw_device = option["hw_device"].cast<std::string>();
            strncpy(opt.video.hw_device, hw_device.c_str(), sizeof(opt.video.hw_device));
            opt.video.hw_device[sizeof(opt.video.hw_device) - 1] = '\0';
        }
        if (option.contains("enable_audio")) {
            opt.enable_audio = option["enable_audio"].cast<bool>();
        }
        if (option.contains("sample_rate")) {
            opt.audio.sample_rate = option["sample_rate"].cast<int>();
        }
        if (option.contains("channels")) {
            opt.audio.channels = option["channels"].cast<int>();
        }
        if (option.contains("audio_format")) {
            opt.audio.format = option["audio_format"].cast<int>();
        }
        if (option.contains("timeout")) {
            opt.timeout = option["timeout"].cast<int64_t>();
        }
        g_callbacks[handle] = callback;
        crp_play(handle, url, &opt, py_callback, handle);
    });

    m.def("replay", &crp_replay, py::call_guard<py::gil_scoped_release>());

    m.def("stop", &crp_stop, py::call_guard<py::gil_scoped_release>());

    m.def("version_code", &crp_version_code);

    m.def("version_str", &crp_version_str);
}
