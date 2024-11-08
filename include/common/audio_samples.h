#pragma once

#include "raster.h"
#include "gpu/gpu.h"

namespace Raster {
    using SharedRawAudioSamples = std::shared_ptr<std::vector<float>>;

    // TODO: implement ability to store multiple attached pictures
    struct AudioSamples {
        int sampleRate;
        SharedRawAudioSamples samples;
        std::vector<Texture> attachedPictures;

        AudioSamples();
    };

    using SharedRawInterleavedAudioSamples = SharedRawAudioSamples;
    using SharedRawDeinterleavedAudioSamples = std::shared_ptr<std::vector<std::vector<float>>>;

    static void DeinterleaveAudioSamples(SharedRawInterleavedAudioSamples t_input, SharedRawDeinterleavedAudioSamples t_output, int frameCount, int channels) {
        const float* pSrcF32 = (const float*)t_input->data();
        auto pDeinterleavedSamples = t_output->data();
        uint64_t iPCMFrame;
        for (iPCMFrame = 0; iPCMFrame < frameCount; ++iPCMFrame) {
            uint32_t iChannel;
            for (iChannel = 0; iChannel < channels; ++iChannel) {
                float* pDstF32 = (float*) (pDeinterleavedSamples[iChannel].data());
                pDstF32[iPCMFrame] = pSrcF32[iPCMFrame*channels+iChannel];
            }
        }
    }

    static void InterleaveAudioSamples(SharedRawDeinterleavedAudioSamples t_input, SharedRawInterleavedAudioSamples t_output, int frameCount, int channels) {
        float* pDstF32 = (float*)t_output->data();
        uint64_t iPCMFrame;
        for (iPCMFrame = 0; iPCMFrame < frameCount; ++iPCMFrame) {
            uint32_t iChannel;
            for (iChannel = 0; iChannel < channels; ++iChannel) {
                const float* pSrcF32 = (const float*)(t_input->data()[iChannel].data());
                pDstF32[iPCMFrame*channels+iChannel] = pSrcF32[iPCMFrame];
            }
        }
    } 

    static SharedRawInterleavedAudioSamples MakeInterleavedAudioSamples(int periodSize, int channels) {
        return std::make_shared<std::vector<float>>(periodSize * channels);
    }

    static SharedRawDeinterleavedAudioSamples MakeDeinterleavedAudioSamples(int periodSize, int channels) {
        std::shared_ptr<std::vector<std::vector<float>>> result = std::make_shared<std::vector<std::vector<float>>>(channels);
        for (int i = 0; i < channels; i++) {
            result->at(i).resize(periodSize);
        }
        return result;
    }
};