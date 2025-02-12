#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "export_renderable.h"
#include "../../../ImGui/imgui.h"

namespace Raster {


    ExportRenderable::ExportRenderable() {
        NodeBase::Initialize();
        NodeBase::GenerateFlowPins();

        SetupAttribute("Renderable", Framebuffer());
        AddInputPin("Renderable");
    }

    AbstractPinMap ExportRenderable::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto renderableCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Renderable", t_contextData));
        auto compositionCandidate = Workspace::GetCompositionByNodeID(nodeID);
        if (renderableCandidate.has_value() && renderableCandidate.value().handle && compositionCandidate) {
            auto& renderable = renderableCandidate.value();
            auto& bundles = Compositor::s_bundles;
            auto& composition = *compositionCandidate;
            if (bundles.Get().find(composition->id) == bundles.Get().end()) {
                bundles.Get()[composition->id] = {};
            }

            auto& bundle = bundles.Get()[composition->id];
            bundle.primaryFramebuffer = renderable;

            auto& targets = Compositor::s_targets;
            targets.Lock();
            targets.GetReference().push_back(CompositorTarget{
                .colorAttachment = renderable.attachments[0],
                .uvAttachment = renderable.attachments[1],
                .opacity = composition->GetOpacity(),
                .blendMode = composition->blendMode,
                .compositionID = composition->id,
                .masks = composition->masks
            });
            targets.Unlock();

            this->lastExportedType = std::type_index(typeid(Framebuffer));
        } else {
            this->lastExportedType = std::nullopt;
        }

        return result;
    }

    void ExportRenderable::AbstractRenderProperties() {
    }

    bool ExportRenderable::AbstractDetailsAvailable() {
        return false;
    }

    std::string ExportRenderable::AbstractHeader() {
        return "Export To Compositor";
    }

    std::string ExportRenderable::Icon() {
        return ICON_FA_IMAGE;
    }

    std::optional<std::string> ExportRenderable::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::ExportRenderable>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Export Renderable",
            .packageName = RASTER_PACKAGED "export_renderable",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}