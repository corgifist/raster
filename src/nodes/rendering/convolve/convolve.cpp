#include "convolve.h"
#include "common/attribute_metadata.h"
#include "common/bezier_curve.h"
#include "common/convolution_kernel.h"
#include "common/gradient_1d.h"
#include "../../../ImGui/imgui.h"
#include "common/dispatchers.h"
#include "compositor/texture_interoperability.h"
#include "font/IconsFontAwesome5.h"



namespace Raster {
    std::optional<Pipeline> Convolve::s_pipeline;

    Convolve::Convolve() {
        NodeBase::Initialize();

        AddInputPin("Base");
        AddOutputPin("Framebuffer");

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Kernel", ConvolutionKernel());
        SetupAttribute("Multiplier", 1.0f);
    }

    AbstractPinMap Convolve::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto& project = Workspace::s_project.value();

        auto& framebuffer = m_managedFramebuffer.Get(GetAttribute<Framebuffer>("Base", t_contextData));
        auto baseCandidate = TextureInteroperability::GetTexture(GetDynamicAttribute("Base", t_contextData));
        auto kernelCandidate = GetAttribute<ConvolutionKernel>("Kernel", t_contextData);
        auto multiplierCandidate = GetAttribute<float>("Multiplier", t_contextData);

        if (!s_pipeline) {
            s_pipeline = GPU::GeneratePipeline(GPU::s_basicShader, GPU::GenerateShader(ShaderType::Fragment, "convolve/shader"));
        }

        if (s_pipeline && baseCandidate && kernelCandidate && multiplierCandidate) {
            auto& pipeline = *s_pipeline;
            auto& base = *baseCandidate;
            auto& kernel = *kernelCandidate;
            auto& multiplier = *multiplierCandidate;

            GPU::BindPipeline(pipeline);
            GPU::BindFramebuffer(framebuffer);
            GPU::ClearFramebuffer(0, 0, 0, 0);
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            GPU::SetShaderUniform(pipeline.fragment, "uKernel", kernel.kernel * (kernel.multiplier * multiplier));
            GPU::BindTextureToShader(pipeline.fragment, "uBase", base, 0);
            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Framebuffer", framebuffer);
        }

        return result;
    }

    void Convolve::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Convolve::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void Convolve::AbstractRenderProperties() {
        RenderAttributeProperty("Kernel", {
            IconMetadata(ICON_FA_IMAGE)
        });
        RenderAttributeProperty("Multiplier", {
            SliderStepMetadata(0.1f),
            IconMetadata(ICON_FA_GEARS)
        });
    }

    bool Convolve::AbstractDetailsAvailable() {
        return false;
    }

    std::string Convolve::AbstractHeader() {
        return "Convolve";
    }

    std::string Convolve::Icon() {
        return ICON_FA_IMAGE;
    }

    std::optional<std::string> Convolve::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Convolve>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Convolve",
            .packageName = RASTER_PACKAGED "convolve",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}