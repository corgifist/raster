#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "sampler_constants_base/sampler_constants_base.h"

namespace Raster {

    SamplerConstantsBase::SamplerConstantsBase(std::any t_constant) {
        NodeBase::Initialize();
        this->m_constant = t_constant;

        AddOutputPin("Value");
    }

    AbstractPinMap SamplerConstantsBase::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        if (m_constant.type() == typeid(TextureFilteringMode)) {
            auto mode = std::any_cast<TextureFilteringMode>(m_constant);
            TryAppendAbstractPinMap(result, "Value", static_cast<int>(mode));
        } else {
            auto wrapping = std::any_cast<TextureWrappingMode>(m_constant);
            TryAppendAbstractPinMap(result, "Value", static_cast<int>(wrapping));
        }
        return result;
    }

    void SamplerConstantsBase::AbstractRenderProperties() {
        RenderAttributeProperty("ArbitraryValue");
    }

    bool SamplerConstantsBase::AbstractDetailsAvailable() {
        return false;
    }

    std::string SamplerConstantsBase::AbstractHeader() {
        if (m_constant.type() == typeid(TextureFilteringMode)) {
            auto mode = std::any_cast<TextureFilteringMode>(m_constant);
            if (mode == TextureFilteringMode::Linear) {
                return "Linear Filtering";
            } else return "Nearest Filtering";
        } else {
            auto wrapping = std::any_cast<TextureWrappingMode>(m_constant);
            if (wrapping == TextureWrappingMode::ClampToBorder) {
                return "Clamp To Border Wrapping";
            } else if (wrapping == TextureWrappingMode::ClampToEdge) {
                return "Clamp To Edge Wrapping";
            } else if (wrapping == TextureWrappingMode::MirroredRepeat) {
                return "Mirrored Repeat Wrapping";
            } else {
                return "Repeat Wrapping";
            }
        }
    }

    std::string SamplerConstantsBase::Icon() {
        return ICON_FA_GEARS;
    }

    std::optional<std::string> SamplerConstantsBase::Footer() {
        return std::nullopt;
    }
}