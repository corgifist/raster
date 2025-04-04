#include "node_properties.h"
#include "common/localization.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "common/ui_helpers.h"
#include "raster.h"
#include "common/layouts.h"

namespace Raster {

    Json NodePropertiesUI::AbstractSerialize() {
        return {};
    }

    void NodePropertiesUI::AbstractLoad(Json t_data) {
        
    }

    void NodePropertiesUI::AbstractRender() {
        if (!open) {
            Layouts::DestroyWindow(id);
        }
        ImGui::SetNextWindowSize(ImVec2(300, 700), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(FormatString("%s %s###%i", ICON_FA_GEARS, Localization::GetString("NODE_PROPERTIES").c_str(), id).c_str(), &open)) {
            if (!Workspace::IsProjectLoaded()) {
                ImGui::PushFont(Font::s_denseFont);
                ImGui::SetWindowFontScale(2.0f);
                    ImVec2 exclamationSize = ImGui::CalcTextSize(ICON_FA_TRIANGLE_EXCLAMATION);
                    ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - exclamationSize / 2.0f);
                    ImGui::Text(ICON_FA_TRIANGLE_EXCLAMATION);
                ImGui::SetWindowFontScale(1.0f);
                ImGui::PopFont();
                ImGui::End();
                return;
            }


            if (ImGui::BeginChild("##propertiesTab", ImVec2(ImGui::GetContentRegionAvail().x, 0), ImGuiChildFlags_AutoResizeY)) {
                if (Workspace::s_project.has_value()) {
                    auto& project = Workspace::s_project.value();
                    ImGui::PushFont(Font::s_denseFont);
                        ImGui::SetWindowFontScale(1.6f);
                        bool treeExpanded = ImGui::TreeNode(FormatString("%s %s###%s_project", ICON_FA_LIST, project.name.c_str(), project.name.c_str()).c_str());
                        ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopFont();
                    if (treeExpanded) {
                        static bool isEditingDescription = false;
                            if (isEditingDescription) {
                                UIHelpers::RenderProjectEditor(project);
                                if (UIHelpers::CenteredButton(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str())) {
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
                                if (UIHelpers::CenteredButton(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT").c_str()).c_str())) {
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
                            ImGui::SetWindowFontScale(1.6f);
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
                                static std::vector<float> s_largestCursors;

                                float largestCursor = 0;
                                for (auto& cursor : s_largestCursors) {
                                    if (cursor > largestCursor) {
                                        largestCursor = cursor;
                                    }
                                }

                                s_largestCursors.clear();

                                ImGui::Text("%s %s", ICON_FA_FONT, Localization::GetString("COMPOSITION_NAME").c_str());
                                ImGui::SameLine();
                                s_largestCursors.push_back(ImGui::GetCursorPosX());
                                if (largestCursor != 0) ImGui::SetCursorPosX(largestCursor);
                                ImGui::InputText("##compositionName", &composition->name);

                                ImGui::Text("%s %s", ICON_FA_MESSAGE, Localization::GetString("COMPOSITION_DESCRIPTION").c_str());
                                ImGui::SameLine();
                                s_largestCursors.push_back(ImGui::GetCursorPosX());
                                if (largestCursor != 0) ImGui::SetCursorPosX(largestCursor);
                                isEditingDescription = !ImGui::InputTextMultiline("##descriptionEditor", &composition->description, ImVec2(0, 0), ImGuiInputTextFlags_EnterReturnsTrue);
                                ImGui::SetItemTooltip("%s %s", ICON_FA_ARROW_POINTER, Localization::GetString("CTRL_CLICK_TO_CLOSE").c_str());
                            
                                int signedValues[] = {
                                    (int) composition->beginFrame, (int) composition->endFrame
                                };
                                ImGui::Text("%s %s", ICON_FA_STOPWATCH, Localization::GetString("COMPOSITION_TIMING").c_str());
                                ImGui::SameLine();
                                s_largestCursors.push_back(ImGui::GetCursorPosX());
                                if (largestCursor != 0) ImGui::SetCursorPosX(largestCursor);
                                ImGui::DragInt2("##compositionTiming", signedValues, 1, 0);
                                composition->beginFrame = signedValues[0];
                                composition->endFrame = signedValues[1];

                                if (UIHelpers::CenteredButton(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str())) {
                                    isEditingDescription = false;
                                }
                            } else {
                                auto& project = Workspace::s_project.value();
                                ImGui::Text("%s", composition->description.c_str());
                                ImGui::Text("%s %s: %i -> %i", ICON_FA_STOPWATCH, Localization::GetString("COMPOSITION_TIMING").c_str(), int(composition->beginFrame / project.framerate), int(composition->GetEndFrame() / project.framerate));
                                ImGui::Text("%s %s: %i", ICON_FA_CIRCLE_NODES, Localization::GetString("TOTAL_NODES_COUNT").c_str(), (int) composition->nodes.size());
                                isEditingDescription = UIHelpers::CenteredButton(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT").c_str()).c_str());
                            }
                            ImGui::TreePop();
                            previousCompositionID = composition->id;
                        }
                    ImGui::PopID();
                }
                auto& project = Workspace::GetProject();
                std::string attributesTreeText = project.selectedAttributes.empty() ? Localization::GetString("NO_ATTRIBUTES_SELECTED") : std::to_string(project.selectedAttributes.size()) + " " + Localization::GetString("ATTRIBUTES_SELECTED");
                ImGui::SetWindowFontScale(1.6f);
                ImGui::PushFont(Font::s_denseFont);
                    bool attributesTreeExpanded = ImGui::TreeNode(FormatString("%s %s###_attributesTree", ICON_FA_LINK, attributesTreeText.c_str()).c_str());
                ImGui::PopFont();
                ImGui::SetWindowFontScale(1.0f);
                if (attributesTreeExpanded) {
                    for (auto& attributeID : project.selectedAttributes) {
                        auto attributeCandidate = Workspace::GetAttributeByAttributeID(attributeID);
                        if (attributeCandidate.has_value()) {
                            auto& attribute = attributeCandidate.value();
                            attribute->RenderLegend(Workspace::GetCompositionByAttributeID(attributeID).value());
                        }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::EndChild();
            
            ImGui::Separator();

            if (ImGui::BeginChild("##nodesProperties", ImGui::GetContentRegionAvail(), 0, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
                auto& project = Workspace::GetProject();
                for (auto& nodeID : project.selectedNodes) {
                    auto maybeNode = Workspace::GetNodeByNodeID(nodeID);
                    if (maybeNode.has_value()) {
                        auto& node = maybeNode.value();
                        auto nodeImplementation = Workspace::GetNodeImplementationByLibraryName(node->libraryName).value();
                        ImGui::PushID(nodeID);
                            ImGui::PushFont(Font::s_denseFont);
                                ImGui::SetWindowFontScale(1.6f);
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
                            ImGui::AlignTextToFramePadding();
                            if (ImGui::Button(FormatString("%s %s", !node->enabled ? ICON_FA_XMARK : ICON_FA_CHECK, Localization::GetString("ENABLED").c_str()).c_str())) {
                                node->enabled = !node->enabled;
                            }
                            ImGui::SameLine();
                            ImGui::AlignTextToFramePadding();
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
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
};