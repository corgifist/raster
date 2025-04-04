#include "combine_channels.h"
#include "common/attribute_metadata.h"
#include "common/ui_helpers.h"
#include "compositor/texture_interoperability.h"
#include "font/IconsFontAwesome5.h"
#include "gpu/gpu.h"

namespace Raster {

    std::optional<Pipeline> CombineChannels::s_pipeline;

    CombineChannels::CombineChannels() {
        NodeBase::Initialize();

        SetupAttribute("R", Framebuffer());
        SetupAttribute("G", Framebuffer());
        SetupAttribute("B", Framebuffer());
        SetupAttribute("A", Framebuffer());

        AddInputPin("R");
        AddInputPin("G");
        AddInputPin("B");
        AddInputPin("A");
        AddOutputPin("Output");
    }

    CombineChannels::~CombineChannels() {
    }

    AbstractPinMap CombineChannels::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto& project = Workspace::GetProject();
        auto rCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("R", t_contextData));
        auto gCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("G", t_contextData));
        auto bCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("B", t_contextData));
        auto aCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("A", t_contextData));
        
        if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "RENDERING_PASS", bool)) {
            return {};
        }

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "combine_channels/shader")
            );
        }

        if (s_pipeline && rCandidate && bCandidate && gCandidate && aCandidate) {
            auto& pipeline = s_pipeline.value();
            auto& rFramebuffer = *rCandidate;
            auto& gFramebuffer = *gCandidate;
            auto& bFramebuffer = *bCandidate;
            auto& aFramebuffer = *aCandidate;
            if (rFramebuffer.attachments.size() <= 0 || gFramebuffer.attachments.size() <= 0 || bFramebuffer.attachments.size() <= 0 || aFramebuffer.attachments.size() <= 0) return {};

            auto& framebuffer = m_framebuffer.Get(rCandidate);

            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);
            GPU::ClearFramebuffer(0, 0, 0, 0);
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            GPU::BindTextureToShader(pipeline.fragment, "uR", rFramebuffer.attachments[0], 0);
            GPU::BindTextureToShader(pipeline.fragment, "uG", gFramebuffer.attachments[0], 1);
            GPU::BindTextureToShader(pipeline.fragment, "uB", bFramebuffer.attachments[0], 2);
            GPU::BindTextureToShader(pipeline.fragment, "uA", aFramebuffer.attachments[0], 3);
            GPU::DrawArrays(3);
            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }

        return result;
    }

    void CombineChannels::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json CombineChannels::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void CombineChannels::AbstractRenderProperties() {
    }

    bool CombineChannels::AbstractDetailsAvailable() {
        return false;
    }

    std::string CombineChannels::AbstractHeader() {
        return "Combine Channels";
    }

    std::string CombineChannels::Icon() {
        return ICON_FA_PLUS;
    }

    std::optional<std::string> CombineChannels::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::CombineChannels>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Combine Channels",
            .packageName = RASTER_PACKAGED "combine_channels",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}