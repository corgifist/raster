#include "hashed_blur.h"

namespace Raster {

    std::optional<Pipeline> HashedBlur::s_pipeline;

    HashedBlur::HashedBlur() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Radius", 0.5f);
        SetupAttribute("HashOffset", glm::vec2(0));
        SetupAttribute("Opacity", 1.0f);
        SetupAttribute("Iterations", 30);

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    AbstractPinMap HashedBlur::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base", t_contextData));
        auto radiusCandidate = GetAttribute<float>("Radius", t_contextData);
        auto hashOffsetCandidate = GetAttribute<glm::vec2>("HashOffset", t_contextData);
        auto iterationsCandidate = GetAttribute<int>("Iterations", t_contextData);
        auto opacityCandidate = GetAttribute<float>("Opacity", t_contextData);

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

            auto& framebuffer = m_framebuffer.Get(baseCandidate);
            if (framebuffer.attachments.size() >= 1) {
                GPU::BindFramebuffer(framebuffer);
                GPU::BindPipeline(pipeline);
                GPU::ClearFramebuffer(0, 0, 0, 0);
                
                GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
                GPU::SetShaderUniform(pipeline.fragment, "uRadius", radius);
                GPU::SetShaderUniform(pipeline.fragment, "uHashOffset", hashOffset);
                GPU::SetShaderUniform(pipeline.fragment, "uIterations", iterations);

                GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);

                GPU::DrawArrays(3);
            }

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }


        return result;
    }

    void HashedBlur::AbstractRenderProperties() {
        RenderAttributeProperty("Radius", {
            SliderStepMetadata(0.01f)
        });
        RenderAttributeProperty("HashOffset");
        RenderAttributeProperty("Opacity", {
            IconMetadata(ICON_FA_DROPLET),
            SliderBaseMetadata(100),
            SliderRangeMetadata(0, 100),
            FormatString("%")
        });
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