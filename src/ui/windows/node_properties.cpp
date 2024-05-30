#include "node_properties.h"
#include "font/font.h"

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
                        ImGui::PushFont(Font::s_denseFont);
                            ImGui::SetWindowFontScale(1.5f);
                                bool treeExpanded = 
                                    ImGui::TreeNodeEx(FormatString("%s%s###%s", (node->Icon() + (node->Icon().empty() ? "" : " ")).c_str(), node->Header().c_str(), nodeImplementation.description.prettyName.c_str()).c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                            ImGui::SetWindowFontScale(1.0f);
                        ImGui::PopFont();
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
        ImGui::End();
    }
};