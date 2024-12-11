#pragma once

#include "raster.h"
#include "common.h"
#include "shared_mutex.h"

namespace Raster {

    enum class VideoFramePrecision {
        Usual, Half, Full
    };

    struct ImageAllocation {
        ImageAllocation() : m_data(nullptr), allocationSize(0), width(0), height(0), elementSize(0) {}
        void Allocate(size_t t_width, size_t t_height, size_t t_elementSize) {
            Deallocate();
            allocationSize = t_width * t_height * 4 * t_elementSize;
            m_data = new uint8_t[allocationSize];
            width = t_width;
            height = t_height;
            elementSize = t_elementSize;
        }

        void Deallocate() {
            if (m_data) {
                delete[] m_data;
                width = height = elementSize = 0;
            }
        }
        
        uint8_t* Get() {
            return (uint8_t*) m_data;
        }
        
        size_t allocationSize;
        size_t width, height, elementSize;
    private:
        uint8_t* m_data;
    };

    struct GenericVideoDecoder {
        int assetID;
        std::shared_ptr<std::unordered_map<float, int>> decoderContexts;
        std::shared_ptr<float> seekTarget;
        VideoFramePrecision targetPrecision;

        GenericVideoDecoder();

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