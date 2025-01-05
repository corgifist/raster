#pragma once

#include "raster.h"
#include "common.h"
#include "shared_mutex.h"

namespace Raster {

    enum class VideoFramePrecision {
        Usual, Half, Full
    };

    struct ImageAllocation {
        ImageAllocation() : data(nullptr), allocationSize(0), width(0), height(0), channels(0), elementSize(0) {}
        void Allocate(size_t t_width, size_t t_height, size_t t_channels, size_t t_elementSize) {
            Deallocate();
            allocationSize = t_width * t_height * t_channels * t_elementSize;
            width = t_width;
            height = t_height;
            elementSize = t_elementSize;
            channels = t_channels;
        }

        void Deallocate() {
            width = height = channels = elementSize = 0;
            data = nullptr;
        }
        
        size_t allocationSize;
        size_t width, height, channels, elementSize;
        uint8_t* data;
    };

    struct GenericVideoDecoder {
        int assetID;
        std::shared_ptr<std::unordered_map<float, int>> decoderContexts;
        std::shared_ptr<float> seekTarget;
        VideoFramePrecision targetPrecision;

        GenericVideoDecoder();

        // size in MB (Megabytes)
        static void InitializeCache(size_t t_size = 512);

        void SetVideoAsset(int t_assetID);
        bool DecodeFrame(ImageAllocation& t_imageAllocation, int t_renderPassID, std::optional<float> t_targetFrame = std::nullopt);
        void Seek(float t_second);
        std::optional<float> GetContentDuration();
        std::optional<glm::vec2> GetContentResolution();
        std::optional<float> GetContentFramerate();
        std::optional<float> GetDecodingProgress();

        void Destroy();

    private:
        SharedMutex m_decodingMutex;
    };
};