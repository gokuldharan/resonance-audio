
#include <algorithm>
#include <memory>

#include "platforms/igibson/igibson.h"
#include "platforms/igibson/bindings.h"
#include "platforms/igibson/igibson_reverb_computer.h"
#include "platforms/common/room_properties.h"


#include "base/audio_buffer.h"
#include "base/constants_and_types.h"
#include "base/logging.h"
#include "base/misc_math.h"
#include "graph/resonance_audio_api_impl.h"
#include "platforms/common/room_effects_utils.h"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <iostream>
#include <fstream>

#include "utils/wav.h"

namespace py = pybind11;

namespace vraudio {
namespace igibson {


    py::array_t<float> InitializeFromMeshAndTest(int num_vertices, int num_triangles,
        py::array_t<float> vertices, py::array_t<int> triangles,
        py::array_t<int> material_indices,
        float scattering_coefficient, py::str file, py::array_t<float> source_location, py::array_t<float> head_pos) {

        // Number of frames per buffer.
        const size_t kFramesPerBuffer = 256;

        // Sampling rate.
        const int kSampleRate = 48000;

        auto resonance_audio = Initialize(kSampleRate, 2, kFramesPerBuffer);

        py::buffer_info mi_buf = material_indices.request();
        int* material_indices_arr = (int*)mi_buf.ptr;

        py::buffer_info v_buf = vertices.request();
        float* verts = (float*)v_buf.ptr;

        py::buffer_info t_buf = triangles.request();
        int* tris = (int*)t_buf.ptr;

        InitializeReverbComputer(num_vertices, num_triangles,
            verts, tris,
            material_indices_arr,
            scattering_coefficient);

        // Ray-tracing related fields.
        const int kNumRays = 20000;
        const int kNumRaysPerBatch = 2000;
        const int kMaxDepth = 3;
        const float kEnergyThresold = 1e-6f;
        const float listener_sphere_radius = 0.1f;
        const size_t impulse_response_num_samples = 96000;

        RoomProperties proxy_room_properties;
        float rt60s [kNumReverbOctaveBands];

        py::buffer_info sl_buf = source_location.request();
        float* source_location_arr = (float*)t_buf.ptr;


        if (!ComputeRt60sAndProxyRoom(kNumRays, kNumRaysPerBatch,
            kMaxDepth, kEnergyThresold,
            source_location_arr,
            listener_sphere_radius, kSampleRate,
            impulse_response_num_samples,
            rt60s,
            &proxy_room_properties)) assert(1);

        SetRoomProperties(&proxy_room_properties, rt60s);

        const char* fName = file.cast<const char*>();

        std::filebuf fb;

        if (!fb.open(fName, std::ios::in)) assert(1);

        std::istream is(&fb);
        std::unique_ptr<const Wav> wav = Wav::CreateOrNull(&is);
        fb.close();

        assert(wav != nullptr);

        int source_id = resonance_audio->api->CreateSoundObjectSource(RenderingMode::kBinauralHighQuality);
        resonance_audio->api->SetSourcePosition(source_id, source_location_arr[0], source_location_arr[1], source_location_arr[2]);

        py::buffer_info hp_buf = head_pos.request();
        int* head_pos_arr = (int*)hp_buf.ptr;

        resonance_audio->api->SetHeadPosition(head_pos_arr[0], head_pos_arr[1], head_pos_arr[2]);

        int num_frames = (int) wav->interleaved_samples().size() / wav->GetNumChannels();
        // Process the next buffer.
        resonance_audio->api->SetInterleavedBuffer(source_id, &(wav->interleaved_samples()[0]), 1, num_frames);

        py::array_t<float> output_py = py::array_t<float>(kNumOutputChannels * num_frames);
        py::buffer_info out_buf = output_py.request();
        float* output = (float*)out_buf.ptr;

        if (!resonance_audio->api->FillInterleavedOutputBuffer(
            2, num_frames, output)) {
            // No valid output was rendered, fill the output buffer with zeros.
            assert(1);
            const size_t buffer_size_samples = 2 * num_frames;
            CHECK(!vraudio::DoesIntegerMultiplicationOverflow<size_t>(
                2, num_frames, buffer_size_samples));

            std::fill(output, output + buffer_size_samples, 0.0f);
        }

        return output_py; //Memory leak?
    }
}
}  // namespace vraudio


PYBIND11_MODULE(audio, m) {
    m.def("InitializeFromMeshAndTest", &vraudio::igibson::InitializeFromMeshAndTest, py::return_value_policy::take_ownership);
}