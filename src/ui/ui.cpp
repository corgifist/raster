#include "ui/ui.h"
#include "windows/node_graph.h"
#include "windows/node_properties.h"

namespace Raster {
    std::unique_ptr<UI> UIFactory::SpawnNodeGraphUI() {
        return (std::unique_ptr<UI>) std::make_unique<NodeGraphUI>();
    }

    std::unique_ptr<UI> UIFactory::SpawnNodePropertiesUI() {
        return (std::unique_ptr<UI>) std::make_unique<NodePropertiesUI>();
    }
}