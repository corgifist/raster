#include "traverser/traverser.h"

namespace Raster {
    void Traverser::TraverseAll() {
        AbstractPinMap accumulator = {};
        Workspace::s_pinCache = {};
        if (Workspace::s_project.has_value()) {
            auto& project = Workspace::s_project.value();
            for (auto& composition : project.compositions) {
                for (auto& node : composition.nodes) {
                    if (node->flowInputPin.value_or(GenericPin()).connectedPinID < 0) {
                        accumulator = node->Execute(accumulator);
                    }
                }
            }
        }
    }
}