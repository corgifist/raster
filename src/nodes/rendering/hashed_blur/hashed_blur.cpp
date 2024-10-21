#include "hashed_blur.h"

namespace Raster {

    std::optional<Pipeline> HashedBlur::s_pipeline;

    HashedBlur::HashedBlur() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Radius", 0.5f);
        SetupAttribute("HashOffset", glm::vec2(0));
        SetupAttribute("Iterations", 30);

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    HashedBlur::~HashedBlur() {
        if (m_framebuffer.Get().handle) {
            m_framebuffer.Destroy();
        }
    }

    AbstractPinMap HashedBlur::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base"));
        auto radiusCandidate = GetAttribute<float>("Radius");
        auto hashOffsetCandidate = GetAttribute<glm::vec2>("HashOffset");
        auto iterationsCandidate = GetAttribute<int>("Iterations");

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "hashed_blur/shader")
            );
        }

        if (s_pipeline.has_value() && baseCandidate.has_value() && radiusCandidate.has_value() && hashOffsetCandidate.has_value() && iterationsCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto& radius = radiusCandidate.value();
            auto& hashOffset = hashOffsetCandidate.value();
            auto& iterations = iterationsCandidate.value();

            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);

            auto& framebuffer = m_framebuffer.GetFrontFramebuffer();
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);
            GPU::ClearFramebuffer(0, 0, 0, 0);
            
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(m_framebuffer.width, m_framebuffer.height));
            GPU::SetShaderUniform(pipeline.fragment, "uRadius", radius);
            GPU::SetShaderUniform(pipeline.fragment, "uHashOffset", hashOffset);
            GPU::SetShaderUniform(pipeline.fragment, "uIterations", iterations);

            GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);

            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }


        return result;
    }

    void HashedBlur::AbstractRenderProperties() {
        RenderAttributeProperty("Radius", {
            SliderStepMetadata(0.01f)
        });
        RenderAttributeProperty("HashOffset");
        RenderAttributeProperty("Iterations");
    }

    void HashedBlur::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json HashedBlur::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool HashedBlur::AbstractDetailsAvailable() {
        return false;
    }

    std::string HashedBlur::AbstractHeader() {
        return "Hashed Blur";
    }

    std::string HashedBlur::Icon() {
        return ICON_FA_DROPLET;
    }

    std::optional<std::string> HashedBlur::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::HashedBlur>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Hashed Blur",
            .packageName = RASTER_PACKAGED "hashed_blur",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}