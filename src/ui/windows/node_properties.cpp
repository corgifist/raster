#include "node_properties.h"

namespace Raster {

    bool NodePropertiesUI::CenteredButton(const char* label, float alignment) {
        ImGuiStyle &style = ImGui::GetStyle();

        float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
        float avail = ImGui::GetContentRegionAvail().x;

        float off = (avail - size) * alignment;
        if (off > 0.0f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

        return ImGui::Button(label);
    }

    void NodePropertiesUI::Render() {
        ImGui::Begin(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("NODE_PROPERTIES").c_str()).c_str());
            for (auto& nodeID : Workspace::s_selectedNodes) {
                auto maybeNode = Workspace::GetNodeByNodeID(nodeID);
                if (maybeNode.has_value()) {
                    auto node = maybeNode.value();
                    auto nodeImplementation = Workspace::GetNodeImplementationByLibraryName(node->libraryName).value();
                    ImGui::PushID(nodeID);
                        ImGui::SetWindowFontScale(1.5f);
                            bool treeExpanded = ImGui::TreeNodeEx(FormatString("%s %s###%s", ICON_FA_CIRCLE_NODES, node->Header().c_str(), nodeImplementation.description.prettyName.c_str()).c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                        ImGui::SetWindowFontScale(1.0f);
                        if (ImGui::IsItemHovered() && ImGui::BeginTooltip()) {
                            ImGui::Text("%s %s: %s", ICON_FA_BOX_OPEN, Localization::GetString("PACKAGE_NAME").c_str(), nodeImplementation.description.packageName.c_str());
                            ImGui::Text("%s %s: %s", ICON_FA_STAR, Localization::GetString("PRETTY_NODE_NAME").c_str(), nodeImplementation.description.prettyName.c_str());
                            ImGui::Text("%s %s: %s", ICON_FA_LIST, Localization::GetString("CATEGORY").c_str(), NodeCategoryUtils::ToString(nodeImplementation.description.category).c_str());
                            ImGui::EndTooltip();
                        }
                        if (treeExpanded) {
                            node->AbstractRenderProperties();
                            ImGui::TreePop();
                        }
                    ImGui::PopID();
                    ImGui::Spacing();
                }
            }

            ImGui::Spacing();
            std::string selectNodeText = FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("SELECT_NODE").c_str());
            if (CenteredButton(selectNodeText.c_str())) {
                ImGui::OpenPopup("##selectNode");
            }

            if (ImGui::BeginPopup("##selectNode", ImGuiWindowFlags_AlwaysAutoResize)) {
                static std::string searchFilter = "";
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                    ImGui::InputText("##searchFilter", &searchFilter);
                    ImGui::SetItemTooltip("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str());
                ImGui::PopItemWidth();
                if (ImGui::BeginChild("##nodesList", ImVec2(ImGui::GetContentRegionAvail().x, 120))) {
                    bool searchIsValid = false;
                    for (auto& node : Workspace::s_nodes) {
                        auto nodeImplementation = Workspace::GetNodeImplementationByLibraryName(node->libraryName).value();
                        auto prettyName = nodeImplementation.description.prettyName;
                        if (searchFilter != "" && StringLowercase(prettyName).find(StringLowercase(searchFilter)) == std::string::npos) continue;
                        if (ImGui::Selectable(FormatString("%s %s", ICON_FA_CIRCLE_NODES, prettyName.c_str(), node->nodeID).c_str())) {
                            Workspace::s_targetSelectNodes.push_back(node->nodeID);
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SetItemTooltip(FormatString("%s %s: %i", ICON_FA_ID_CARD, Localization::GetString("NODE_ID").c_str(), node->nodeID).c_str());
                        searchIsValid = true;
                    }
                    if (!searchIsValid) {
                        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 2.0f - ImGui::CalcTextSize(Localization::GetString("NOTHING_TO_SHOW").c_str()).x / 2.0f);
                        ImGui::Text(Localization::GetString("NOTHING_TO_SHOW").c_str());
                    }
                    ImGui::EndChild();
                }
                ImGui::EndPopup();
            }
        ImGui::End();
    }
};