#include "node_properties.h"

namespace Raster {
    void NodePropertiesUI::Render() {
        ImGui::Begin(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("NODE_PROPERTIES").c_str()).c_str());
            for (auto& nodeID : Workspace::s_selectedNodes) {
                auto maybeNode = Workspace::GetNodeByNodeID(nodeID);
                if (maybeNode.has_value()) {
                    auto node = maybeNode.value();
                    auto nodeImplementation = Workspace::GetNodeImplementationByLibraryName(node->libraryName).value();
                    ImGui::PushID(nodeID);
                        ImGui::SetWindowFontScale(1.5f);
                            bool treeExpanded = ImGui::TreeNodeEx(FormatString("%s %s (%i)", ICON_FA_CIRCLE_NODES, nodeImplementation.description.prettyName.c_str(), nodeID).c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                        ImGui::SetWindowFontScale(1.0f);
                        if (treeExpanded) {
                            node->AbstractRenderProperties();
                            ImGui::TreePop();
                        }
                    ImGui::PopID();
                    ImGui::Spacing();
                }
            }
        ImGui::End();
    }
};