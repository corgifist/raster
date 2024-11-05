#include "brightness_contrast_saturation_vibrance_hue.h"

namespace Raster {

    std::optional<Pipeline> BrightnessContrastSaturationVibranceHue::s_pipeline;

    BrightnessContrastSaturationVibranceHue::BrightnessContrastSaturationVibranceHue() {
        NodeBase::Initialize();

        SetupAttribute("Input", Framebuffer());
        SetupAttribute("Brightness", 0.0f);
        SetupAttribute("Contrast", 1.0f);
        SetupAttribute("Saturation", 1.0f);
        SetupAttribute("Vibrance", 1.0f);
        SetupAttribute("Hue", 0.0f);

        AddInputPin("Input");
        AddOutputPin("Output");
    }

    BrightnessContrastSaturationVibranceHue::~BrightnessContrastSaturationVibranceHue() {
        if (m_framebuffer.Get().handle) {
            m_framebuffer.Destroy();
        }
    }

    AbstractPinMap BrightnessContrastSaturationVibranceHue::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        
        auto inputCandidate = TextureInteroperability::GetTexture(GetDynamicAttribute("Input", t_contextData));
        auto brightnessCandidate = GetAttribute<float>("Brightness", t_contextData);
        auto contrastCandidate = GetAttribute<float>("Contrast", t_contextData);
        auto saturationCandidate = GetAttribute<float>("Saturation", t_contextData);
        auto vibranceCandidate = GetAttribute<float>("Vibrance", t_contextData);
        auto hueCandidate = GetAttribute<float>("Hue", t_contextData);

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "brightness_contrast_saturation_vibrance_hue/shader")
            );
        }

        Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);
        if (s_pipeline.has_value() && inputCandidate.has_value() && brightnessCandidate.has_value() && contrastCandidate.has_value() && saturationCandidate.has_value() && vibranceCandidate.has_value() && hueCandidate.has_value()) {
            auto& framebuffer = m_framebuffer.GetFrontFramebuffer();
            auto& input = inputCandidate.value();
            auto& pipeline = s_pipeline.value();
            auto& brightness = brightnessCandidate.value();
            auto& contrast = contrastCandidate.value();
            auto& saturation = saturationCandidate.value();
            auto& vibrance = vibranceCandidate.value();
            auto& hue = hueCandidate.value();
            m_lastBrightness = brightness;
            m_lastContrast = contrast;
            m_lastSaturation = saturation;
            m_lastVibrance = vibrance;
            m_lastHue = hue;

            GPU::BindFramebuffer(framebuffer);
            GPU::ClearFramebuffer(0, 0, 0, 0);

            GPU::BindPipeline(pipeline);

            GPU::SetShaderUniform(pipeline.fragment, "uBrightness", brightness);
            GPU::SetShaderUniform(pipeline.fragment, "uContrast", contrast);
            GPU::SetShaderUniform(pipeline.fragment, "uSaturation", saturation);
            GPU::SetShaderUniform(pipeline.fragment, "uVibrance", vibrance);
            GPU::SetShaderUniform(pipeline.fragment, "uHue", hue);
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            GPU::BindTextureToShader(pipeline.fragment, "uTexture", input, 0);

            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", framebuffer); 
        }

        return result;
    }

    void BrightnessContrastSaturationVibranceHue::AbstractRenderProperties() {
        RenderAttributeProperty("Brightness");
        RenderAttributeProperty("Contrast");
        RenderAttributeProperty("Saturation");
        RenderAttributeProperty("Vibrance");
        RenderAttributeProperty("Hue");
    }

    void BrightnessContrastSaturationVibranceHue::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json BrightnessContrastSaturationVibranceHue::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool BrightnessContrastSaturationVibranceHue::AbstractDetailsAvailable() {
        return false;
    }

    std::string BrightnessContrastSaturationVibranceHue::AbstractHeader() {
        std::vector<std::string> appliedEffectsList;

        if (m_lastBrightness.has_value() && m_lastBrightness.value() != 0.0f) {
            appliedEffectsList.push_back("Brightness");
        }
        if (m_lastContrast.has_value() && m_lastContrast.value() != 1.0) {
            appliedEffectsList.push_back("Contrast");
        }
        if (m_lastSaturation.has_value() && m_lastSaturation.value() != 1.0) {
            appliedEffectsList.push_back("Saturaton");
        }
        if (m_lastVibrance.has_value() && m_lastVibrance.value() != 1.0) {
            appliedEffectsList.push_back("Vibrance");
        }
        if (m_lastHue.has_value() && m_lastHue.value() != 0.0) {
            appliedEffectsList.push_back("Hue");
        }

        std::string header = "";

        int index = 0;
        for (auto& effect : appliedEffectsList) {
            header += effect + (index + 1 == appliedEffectsList.size() ? "" : " / ");
            index++;
        }

        if (header.empty()) header = "No Effects Applied";
        return header;
    }

    std::string BrightnessContrastSaturationVibranceHue::Icon() {
        return ICON_FA_IMAGE " " ICON_FA_DROPLET;
    }

    std::optional<std::string> BrightnessContrastSaturationVibranceHue::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::BrightnessContrastSaturationVibranceHue>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Brightness / Contrast / Saturation / Hue",
            .packageName = RASTER_PACKAGED "brightness_contrast_saturation_hue",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}