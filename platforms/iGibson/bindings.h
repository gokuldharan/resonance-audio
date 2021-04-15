
#ifndef RESONANCE_AUDIO_PLATFORM_IGIBSON_BINDINGS_H_
#define RESONANCE_AUDIO_PLATFORM_IGIBSON_BINDINGS_H_

#include "api/resonance_audio_api.h"
#include "platforms/common/room_properties.h"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace vraudio {
namespace igibson {

py::array_t<float> InitializeFromMeshAndTest(int num_vertices, int num_triangles,
    py::array_t<float> vertices, py::array_t<int> triangles,
    py::array_t<int> material_indices,
    float scattering_coefficient);
}  // namespace vraudio
}
#endif  // RESONANCE_AUDIO_PLATFORM_IGIBSON_BINDINGS_H_