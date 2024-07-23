#include "traverser/traverser.h"

namespace Raster {
    void Traverser::TraverseAll() {
        AbstractPinMap accumulator = {};
        Workspace::s_pinCache = {};
        if (Workspace::s_project.has_value()) {
            auto& project = Workspace::s_project.value();
            for (auto& composition : project.compositions) {
                for (auto& node : composition.nodes) {
                    node->executionsPerFrame = 0;
                    node->ClearAttributesCache();
                }
                if (!IsInBounds(std::floor(project.currentFrame), std::floor(composition.beginFrame), std::floor(composition.endFrame))) continue;
                for (auto& node : composition.nodes) {
                    if (node->flowInputPin.has_value()) {
                        auto& flowInputPin = node->flowInputPin.value();
                        bool anyPinConnected = false;
                        for (auto& nodeCandidate : composition.nodes) {
                            if (nodeCandidate->flowOutputPin.has_value() && nodeCandidate->flowOutputPin.value().connectedPinID == flowInputPin.pinID) {
                                anyPinConnected = true;
                                break;
                            }
                        }
                        if (!anyPinConnected) {
                            accumulator = node->Execute(accumulator);
                        }
                    }
                }
            }
        }
    }
}