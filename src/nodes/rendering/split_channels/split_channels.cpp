#include "split_channels.h"
#include "common/attribute_metadata.h"
#include "common/ui_helpers.h"
#include "font/IconsFontAwesome5.h"
#include "gpu/gpu.h"

namespace Raster {

    std::optional<Pipeline> SplitChannels::s_pipeline;

    SplitChannels::SplitChannels() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("ReplaceAlpha", false);
        SetupAttribute("AlphaForRGBChannels", 1.0f);

        AddInputPin("Base");
        AddOutputPin("R");
        AddOutputPin("G");
        AddOutputPin("B");
        AddOutputPin("A");
    }

    SplitChannels::~SplitChannels() {
    }

    AbstractPinMap SplitChannels::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "split_channels/shader")
            );
        }

        auto& project = Workspace::GetProject();
        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base", t_contextData));
        auto replaceAlphaCandidate = GetAttribute<bool>("ReplaceAlpha", t_contextData);
        auto alphaCandidate = GetAttribute<float>("AlphaForRGBChannels", t_contextData);
        
        if (baseCandidate.has_value() && alphaCandidate && s_pipeline.has_value() && replaceAlphaCandidate && baseCandidate->attachments.size() > 0) {
            auto& pipeline = *s_pipeline;
            auto& base = baseCandidate.value();
            auto& rFramebuffer = m_rFramebuffer.Get(baseCandidate);
            auto& gFramebuffer = m_gFramebuffer.Get(baseCandidate);
            auto& bFramebuffer = m_bFramebuffer.Get(baseCandidate);
            auto& aFramebuffer = m_aFramebuffer.Get(baseCandidate);
            auto& alpha = alphaCandidate.value();
            auto& replaceAlpha = *replaceAlphaCandidate;
            GPU::BindPipeline(pipeline);
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(base.width, base.height));
            GPU::SetShaderUniform(pipeline.fragment, "uAlpha", alpha);
            GPU::SetShaderUniform(pipeline.fragment, "uReplaceAlpha", replaceAlpha);
            GPU::BindTextureToShader(pipeline.fragment, "uBase", base.attachments.at(0), 0);

            GPU::BindFramebuffer(rFramebuffer);
            GPU::ClearFramebuffer(0, 0, 0, 0);
            GPU::SetShaderUniform(pipeline.fragment, "uChannel", 0); // r
            GPU::DrawArrays(3);

            GPU::BindFramebuffer(gFramebuffer);
            GPU::ClearFramebuffer(0, 0, 0, 0);
            GPU::SetShaderUniform(pipeline.fragment, "uChannel", 1); // g
            GPU::DrawArrays(3);

            GPU::BindFramebuffer(bFramebuffer);
            GPU::ClearFramebuffer(0, 0, 0, 0);
            GPU::SetShaderUniform(pipeline.fragment, "uChannel", 2); // b
            GPU::DrawArrays(3);

            GPU::BindFramebuffer(aFramebuffer);
            GPU::ClearFramebuffer(0, 0, 0, 0);
            GPU::SetShaderUniform(pipeline.fragment, "uChannel", 3); // a
            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "R", rFramebuffer);
            TryAppendAbstractPinMap(result, "G", gFramebuffer);
            TryAppendAbstractPinMap(result, "B", bFramebuffer);
            TryAppendAbstractPinMap(result, "A", aFramebuffer);

        }


        return result;
    }

    void SplitChannels::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json SplitChannels::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void SplitChannels::AbstractRenderProperties() {
        RenderAttributeProperty("ReplaceAlpha", {
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("AlphaForRGBChannels", {
            IconMetadata(ICON_FA_DROPLET),
            SliderRangeMetadata(0, 1),
            SliderBaseMetadata(100),
            FormatStringMetadata("%")
        });
    }

    bool SplitChannels::AbstractDetailsAvailable() {
        return false;
    }

    std::string SplitChannels::AbstractHeader() {
        return "Split Channels";
    }

    std::string SplitChannels::Icon() {
        return ICON_FA_SHUFFLE;
    }

    std::optional<std::string> SplitChannels::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SplitChannels>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Split Channels",
            .packageName = RASTER_PACKAGED "split_channels",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}