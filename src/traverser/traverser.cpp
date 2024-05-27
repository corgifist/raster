#include "traverser/traverser.h"

namespace Raster {
    void Traverser::TraverseAll() {
        AbstractPinMap accumulator = {};
        Workspace::s_pinCache = {};
        for (auto& node : Workspace::s_nodes) {
            if (node->flowInputPin.value_or(GenericPin()).connectedPinID < 0) {
                accumulator = node->Execute(accumulator);
            }
        }
    }
}