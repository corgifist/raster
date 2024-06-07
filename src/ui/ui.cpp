#include "ui/ui.h"
#include "windows/node_graph.h"
#include "windows/node_properties.h"
#include "windows/rendering.h"

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
}