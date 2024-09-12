#include "angular_blur.h"

namespace Raster {

    std::optional<Pipeline> AngularBlur::s_pipeline;

    AngularBlur::AngularBlur() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Angle", 0.0f);
        SetupAttribute("Center", glm::vec2(0));
        SetupAttribute("Samples", 50);

        AddInputPin("Base");
        AddOutputPin("Output");

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "angular_blur/shader")
            );
        }
    }

    AngularBlur::~AngularBlur() {
        if (m_framebuffer.handle) {
            GPU::DestroyFramebufferWithAttachments(m_framebuffer);
        }
    }

    AbstractPinMap AngularBlur::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base"));
        auto angleCandidate = GetAttribute<float>("Angle");
        auto centerCandidate = GetAttribute<glm::vec2>("Center");
        auto samplesCandidate = GetAttribute<int>("Samples");
        
        if (s_pipeline.has_value() && baseCandidate.has_value() && angleCandidate.has_value() && centerCandidate.has_value() && samplesCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto angle = glm::radians(angleCandidate.value());
            auto& center = centerCandidate.value();
            auto samples = (float) samplesCandidate.value();    

            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);

            GPU::BindFramebuffer(m_framebuffer);
            GPU::BindPipeline(pipeline);
            GPU::ClearFramebuffer(0, 0, 0, 0);

            GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);
            GPU::SetShaderUniform(pipeline.fragment, "uAngularBlurAngle", angle);
            GPU::SetShaderUniform(pipeline.fragment, "uSamples", samples);
            GPU::SetShaderUniform(pipeline.fragment, "uCenter", center);

            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(m_framebuffer.width, m_framebuffer.height));
            
            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", m_framebuffer);
        }


        return result;
    }

    void AngularBlur::AbstractRenderProperties() {
        RenderAttributeProperty("Angle");
        RenderAttributeProperty("Center");
        RenderAttributeProperty("Samples");
    }

    void AngularBlur::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json AngularBlur::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool AngularBlur::AbstractDetailsAvailable() {
        return false;
    }

    std::string AngularBlur::AbstractHeader() {
        return "Angular Blur";
    }

    std::string AngularBlur::Icon() {
        return ICON_FA_ROTATE;
    }

    std::optional<std::string> AngularBlur::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::AngularBlur>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Angular Blur",
            .packageName = RASTER_PACKAGED "angular_blur",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}