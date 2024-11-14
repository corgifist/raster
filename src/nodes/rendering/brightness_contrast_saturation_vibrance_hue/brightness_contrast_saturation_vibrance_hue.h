#pragma once
#include "raster.h"
#include "common/common.h"
#include "compositor/managed_framebuffer.h"
#include "compositor/texture_interoperability.h"
#include "gpu/gpu.h"

namespace Raster {
    struct BrightnessContrastSaturationVibranceHue : public NodeBase {
        BrightnessContrastSaturationVibranceHue();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    private:
        ManagedFramebuffer m_framebuffer;

        static std::optional<Pipeline> s_pipeline;
        std::optional<float> m_lastBrightness, m_lastContrast, 
                                m_lastSaturation, m_lastVibrance, m_lastHue;
    };
};