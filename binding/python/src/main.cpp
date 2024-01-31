#include <unordered_map>
#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include "coralreefplayer.h"

namespace py = pybind11;

std::unordered_map<crp_handle, py::function> g_callbacks;

void py_callback(int event, void* data, void* user_data) {
    py::gil_scoped_acquire gil;
    if (event == CRP_EV_NEW_FRAME) {
        py::object frame = py::cast(*(Frame*) data);
        g_callbacks[(crp_handle) user_data](event, frame);
    } else {
        g_callbacks[(crp_handle) user_data](event, data);
    }
}

PYBIND11_MODULE(cwrapper, m) {
    py::class_<Frame>(m, "Frame", py::buffer_protocol())
        .def_readonly("width", &Frame::width)
        .def_readonly("height", &Frame::height)
        .def_readonly("format", &Frame::format)
        .def_readonly("pts", &Frame::pts)
        .def_buffer([](Frame &frame) -> py::buffer_info {
            return py::buffer_info(
                frame.data[0],
                sizeof(uint8_t),
                py::format_descriptor<uint8_t>::format(),
                3,
                {frame.height, frame.width, 3},
                {frame.linesize[0], 3, 1}
            );
        });

    m.def("create", &crp_create, py::return_value_policy::reference);

    m.def("destroy", [](crp_handle handle) {
        crp_destroy(handle);
        g_callbacks.erase(handle);
    });

    m.def("auth", &crp_auth);

    m.def("play", [](crp_handle handle, const char* url, int transport,
        int width, int height, int format, const py::function &callback) {
        g_callbacks[handle] = callback;
        crp_play(handle, url, transport, width, height, format, py_callback, handle);
    });

    m.def("replay", &crp_replay);

    m.def("stop", &crp_stop);

    m.def("version_code", &crp_version_code);

    m.def("version_str", &crp_version_str);
}
