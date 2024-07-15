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
            if (Workspace::s_project.has_value()) {
                auto& project = Workspace::s_project.value();
                ImGui::PushFont(Font::s_denseFont);
                    ImGui::SetWindowFontScale(1.5f);
                    bool treeExpanded = ImGui::TreeNode(FormatString("%s %s###%s_project", ICON_FA_LIST, project.name.c_str(), project.name.c_str()).c_str());
                    ImGui::SetWindowFontScale(1.0f);
                ImGui::PopFont();
                if (treeExpanded) {
                    static bool isEditingDescription = false;
                        if (isEditingDescription) {
                            ImGui::Text("%s %s", ICON_FA_FONT, Localization::GetString("PROJECT_NAME").c_str());
                            ImGui::SameLine();
                            ImGui::InputText("##projectName", &project.name);

                            ImGui::Text("%s %s", ICON_FA_COMMENT, Localization::GetString("PROJECT_DESCRIPTION").c_str());
                            ImGui::SameLine();
                            isEditingDescription = !ImGui::InputTextMultiline("##descriptionEditor", &project.description, ImVec2(0, 0), ImGuiInputTextFlags_EnterReturnsTrue);
                            ImGui::SetItemTooltip("%s %s", ICON_FA_ARROW_POINTER, Localization::GetString("CTRL_CLICK_TO_CLOSE").c_str());

                            ImGui::Text("%s %s", ICON_FA_VIDEO, Localization::GetString("PROJECT_FRAMERATE").c_str());
                            ImGui::SameLine();

                            int signedFramerate = project.framerate;
                            ImGui::DragInt("##projectFramerate", &signedFramerate, 1, 1);
                            project.framerate = signedFramerate;

                            int signedPreferredResolution[2] = {
                                (int) project.preferredResolution.x,
                                (int) project.preferredResolution.y
                            };
                            ImGui::Text("%s %s", ICON_FA_EXPAND, Localization::GetString("PROJECT_RESOLUTION").c_str());
                            ImGui::SameLine();
                            ImGui::DragInt2("##preferredResolution", signedPreferredResolution);
                            project.preferredResolution = {signedPreferredResolution[0], signedPreferredResolution[1]};

                            ImGui::Text("%s %s: ", ICON_FA_DROPLET, Localization::GetString("PROJECT_BACKGROUND_COLOR").c_str());
                            ImGui::SameLine();
                            float colorPtr[4] = {
                                project.backgroundColor.r,
                                project.backgroundColor.g, 
                                project.backgroundColor.b,
                                project.backgroundColor.a
                            };
                            ImGui::PushItemWidth(200);
                                ImGui::ColorPicker4("##colorPreview", colorPtr, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoAlpha);
                            ImGui::PopItemWidth();
                            project.backgroundColor = {
                                colorPtr[0], colorPtr[1], colorPtr[2], colorPtr[3]
                            };

                            if (CenteredButton(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str())) {
                                isEditingDescription = false;
                            }
                        } else {
                            ImGui::Text("%s", project.description.c_str());
                            ImGui::Text("%s %s: %i", ICON_FA_VIDEO, Localization::GetString("PROJECT_FRAMERATE").c_str(), (int) project.framerate);
                            ImGui::Text("%s %s: %ix%i", ICON_FA_EXPAND, Localization::GetString("PROJECT_RESOLUTION").c_str(), (int) project.preferredResolution.x, (int) project.preferredResolution.y);
                            ImGui::Text("%s %s: ", ICON_FA_DROPLET, Localization::GetString("PROJECT_BACKGROUND_COLOR").c_str());
                            ImGui::SameLine();
                            float colorPtr[4] = {
                                project.backgroundColor.r,
                                project.backgroundColor.g, 
                                project.backgroundColor.b,
                                project.backgroundColor.a
                            };
                            ImGui::PushItemWidth(200);
                                ImGui::ColorPicker4("##colorPreview", colorPtr, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
                            ImGui::PopItemWidth();
                            ImGui::Text("%s %s: %i", ICON_FA_LAYER_GROUP, Localization::GetString("TOTAL_COMPOSITIONS_COUNT").c_str(), (int) project.compositions.size());
                            if (CenteredButton(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT").c_str()).c_str())) {
                                isEditingDescription = true;
                            }
                        }
                    ImGui::TreePop();
                }
            }
            auto compositionsCandidate = Workspace::GetSelectedCompositions();
            if (compositionsCandidate.has_value() && !compositionsCandidate.value().empty()) {
                auto composition = compositionsCandidate.value()[0];
                ImGui::PushID(composition->id);
                    ImGui::PushFont(Font::s_denseFont);
                        ImGui::SetWindowFontScale(1.5f);
                        bool treeExpanded = ImGui::TreeNode(FormatString("%s %s###composition", ICON_FA_LAYER_GROUP, composition->name.c_str()).c_str());
                        ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopFont();
                    if (treeExpanded) {
                        static bool isEditingDescription = false;
                        static int previousCompositionID = composition->id;
                        if (previousCompositionID != composition->id) {
                            isEditingDescription = false;
                        }
                        if (isEditingDescription) {
                            ImGui::Text("%s %s", ICON_FA_FONT, Localization::GetString("COMPOSITION_NAME").c_str());
                            ImGui::SameLine();
                            ImGui::InputText("##compositionName", &composition->name);

                            ImGui::Text("%s %s", ICON_FA_MESSAGE, Localization::GetString("COMPOSITION_DESCRIPTION").c_str());
                            ImGui::SameLine();
                            isEditingDescription = !ImGui::InputTextMultiline("##descriptionEditor", &composition->description, ImVec2(0, 0), ImGuiInputTextFlags_EnterReturnsTrue);
                            ImGui::SetItemTooltip("%s %s", ICON_FA_ARROW_POINTER, Localization::GetString("CTRL_CLICK_TO_CLOSE").c_str());
                        
                            int signedValues[] = {
                                (int) composition->beginFrame, (int) composition->endFrame
                            };
                            ImGui::Text("%s %s", ICON_FA_STOPWATCH, Localization::GetString("COMPOSITION_TIMING").c_str());
                            ImGui::SameLine();
                            ImGui::DragInt2("##compositionTiming", signedValues, 1, 0);
                            composition->beginFrame = signedValues[0];
                            composition->endFrame = signedValues[1];

                            if (CenteredButton(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str())) {
                                isEditingDescription = false;
                            }
                        } else {
                            auto& project = Workspace::s_project.value();
                            ImGui::Text("%s", composition->description.c_str());
                            ImGui::Text("%s %s: %i -> %i", ICON_FA_STOPWATCH, Localization::GetString("COMPOSITION_TIMING").c_str(), int(composition->beginFrame / project.framerate), int(composition->endFrame / project.framerate));
                            ImGui::Text("%s %s: %i", ICON_FA_CIRCLE_NODES, Localization::GetString("TOTAL_NODES_COUNT").c_str(), (int) composition->nodes.size());
                            isEditingDescription = CenteredButton(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT").c_str()).c_str());
                        }
                        ImGui::TreePop();
                        previousCompositionID = composition->id;
                    }
                ImGui::PopID();
                ImGui::Separator();
            }
            for (auto& nodeID : Workspace::s_selectedNodes) {
                auto maybeNode = Workspace::GetNodeByNodeID(nodeID);
                if (maybeNode.has_value()) {
                    auto& node = maybeNode.value();
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
                        ImGui::SameLine();
                        if (ImGui::Button(FormatString("%s %s", !node->enabled ? ICON_FA_XMARK : ICON_FA_CHECK, Localization::GetString("ENABLED").c_str()).c_str())) {
                            node->enabled = !node->enabled;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(FormatString("%s %s", node->bypassed ? ICON_FA_CHECK : ICON_FA_XMARK, Localization::GetString("BYPASSED").c_str()).c_str())) {
                            node->bypassed = !node->bypassed;
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