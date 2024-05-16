#include "traverser/traverser.h"

namespace Raster {
    void Traverser::TraverseAll() {
        for (auto& node : Workspace::s_nodes) {
            if (node->flowOutputPin.value_or(GenericPin()).pinID < 0) {
                node->Execute();
            }
        }
    }
}