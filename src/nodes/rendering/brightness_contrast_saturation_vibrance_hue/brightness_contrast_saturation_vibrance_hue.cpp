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

    AbstractPinMap BrightnessContrastSaturationVibranceHue::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        
        auto inputCandidate = TextureInteroperability::GetTexture(GetDynamicAttribute("Input"));
        auto brightnessCandidate = GetAttribute<float>("Brightness");
        auto contrastCandidate = GetAttribute<float>("Contrast");
        auto saturationCandidate = GetAttribute<float>("Saturation");
        auto vibranceCandidate = GetAttribute<float>("Vibrance");
        auto hueCandidate = GetAttribute<float>("Hue");

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

        auto brightnessCandidate = GetAttribute<float>("Brightness");
        auto contrastCandidate = GetAttribute<float>("Contrast");
        auto saturationCandidate = GetAttribute<float>("Saturation");
        auto vibranceCandidate = GetAttribute<float>("Vibrance");
        auto hueCandidate = GetAttribute<float>("Hue");
        if (brightnessCandidate.has_value() && brightnessCandidate.value() != 0.0f) {
            appliedEffectsList.push_back("Brightness");
        }
        if (contrastCandidate.has_value() && contrastCandidate.value() != 1.0) {
            appliedEffectsList.push_back("Contrast");
        }
        if (saturationCandidate.has_value() && saturationCandidate.value() != 1.0) {
            appliedEffectsList.push_back("Saturaton");
        }
        if (vibranceCandidate.has_value() && vibranceCandidate.value() != 1.0) {
            appliedEffectsList.push_back("Vibrance");
        }
        if (hueCandidate.has_value() && hueCandidate.value() != 0.0) {
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