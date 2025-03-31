#include "rasterize.h"
#include "common/attribute_metadata.h"
#include "common/line2d.h"

#include "../../../ImGui/imgui.h"
#include "common/dispatchers.h"
#include "raster.h"



namespace Raster {
    Rasterize::Rasterize() {
        NodeBase::Initialize();

        AddInputPin("Base");
        AddOutputPin("Output");

        SetupAttribute("Base", Framebuffer());
        this->m_lastPassID = -1;
    }

    AbstractPinMap Rasterize::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto& project = Workspace::s_project.value();

        int renderingPassID = RASTER_GET_CONTEXT_VALUE(t_contextData, "RENDERING_PASS_ID", int);
        if (m_lastPassID < 0 || m_lastPassID != renderingPassID) {
            m_lastPassID = renderingPassID;
            auto baseCandidate = GetAttribute<Framebuffer>("Base", t_contextData);
            if (!baseCandidate) return {};
            auto& framebuffer = m_managedFramebuffer.Get(std::nullopt);
            auto& base = *baseCandidate;
            for (int i = 0; i < std::min(base.attachments.size(), framebuffer.attachments.size()); i++) {
                GPU::BlitTexture(framebuffer.attachments[i], base.attachments[i]);
            }
            m_lastFramebuffer = framebuffer;
            TryAppendAbstractPinMap(result, "Output", framebuffer);
        } else {
            TryAppendAbstractPinMap(result, "Output", m_lastFramebuffer);
        }

        return result;
    }

    void Rasterize::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Rasterize::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void Rasterize::AbstractRenderProperties() {
    }

    bool Rasterize::AbstractDetailsAvailable() {
        return false;
    }

    std::string Rasterize::AbstractHeader() {
        return "Rasterize";
    }

    std::string Rasterize::Icon() {
        return ICON_FA_GEARS;
    }

    std::optional<std::string> Rasterize::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Rasterize>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Rasterize",
            .packageName = RASTER_PACKAGED "rasterize",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}