#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "export_renderable.h"
#include "../../../ImGui/imgui.h"

namespace Raster {


    ExportRenderable::ExportRenderable() {
        NodeBase::Initialize();
        NodeBase::GenerateFlowPins();

        this->m_attributes["Renderable"] = Framebuffer();
        AddInputPin("Renderable");
    }

    AbstractPinMap ExportRenderable::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto renderableCandidate = GetAttribute<Framebuffer>("Renderable");
        if (renderableCandidate.has_value() && renderableCandidate.value().handle) {
            auto& renderable = renderableCandidate.value();
            auto composition = Workspace::GetCompositionByNodeID(nodeID).value();
            auto& bundles = Compositor::s_bundles;
            if (bundles.find(composition->id) == bundles.end()) {
                bundles[composition->id] = {};
            }

            auto& bundle = bundles[composition->id];
            bundle.primaryFramebuffer = renderable;

            auto& targets = Compositor::s_targets;
            targets.push_back(CompositorTarget{
                .colorAttachment = renderable.attachments[0],
                .uvAttachment = renderable.attachments[1],
                .owner = composition
            });

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
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::ExportRenderable>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Export Renderable",
            .packageName = RASTER_PACKAGED "export_renderable",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}