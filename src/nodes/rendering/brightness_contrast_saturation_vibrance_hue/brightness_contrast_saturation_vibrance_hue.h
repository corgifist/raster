#pragma once
#include "raster.h"
#include "common/common.h"
#include "compositor/managed_framebuffer.h"
#include "compositor/texture_interoperability.h"
#include "gpu/gpu.h"

namespace Raster {
    struct BrightnessContrastSaturationVibranceHue : public NodeBase {
        BrightnessContrastSaturationVibranceHue();
        ~BrightnessContrastSaturationVibranceHue();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    private:
        DoubleBufferedFramebuffer m_framebuffer;

        static std::optional<Pipeline> s_pipeline;
    };
};