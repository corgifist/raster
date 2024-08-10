#include "ui/ui.h"
#include "windows/node_graph.h"
#include "windows/node_properties.h"
#include "windows/rendering.h"
#include "windows/timeline.h"
#include "windows/asset_manager.h"
#include "windows/easing_editor.h"

namespace Raster {
    AbstractUI UIFactory::SpawnNodeGraphUI() {
        return (AbstractUI) std::make_unique<NodeGraphUI>();
    }

    AbstractUI UIFactory::SpawnNodePropertiesUI() {
        return (AbstractUI) std::make_unique<NodePropertiesUI>();
    }

    AbstractUI UIFactory::SpawnRenderingUI() {
        return (AbstractUI) std::make_unique<RenderingUI>();
    }

    AbstractUI UIFactory::SpawnTimelineUI() {
        return (AbstractUI) std::make_unique<TimelineUI>();
    }

    AbstractUI UIFactory::SpawnAssetManagerUI() {
        return (AbstractUI) std::make_unique<AssetManagerUI>();
    }

    AbstractUI UIFactory::SpawnEasingEditor() {
        return (AbstractUI) std::make_unique<EasingEditorUI>();
    }
}