#include "rendering.h"
#include "font/font.h"
#include "compositor/compositor.h"
#include "common/transform2d.h"
#include "common/dispatchers.h"
#include "compositor/async_rendering.h"

namespace Raster {
    void RenderingUI::Render() {
        ImGui::Begin(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("RENDERING").c_str()).c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollWithMouse);
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
            if (Workspace::s_project.has_value()) {
                auto& project = Workspace::s_project.value();
                auto& selectedNodes = project.selectedNodes;
                auto selectedCompositionsCandidate = Workspace::GetSelectedCompositions();

                static std::unordered_map<int, std::string> selectedPinsMap;
                if (project.customData.contains("RenderingSelectedPinsMap")) {
                    selectedPinsMap = project.customData["RenderingSelectedPinsMap"];
                }
                static bool compositionLock = false;
                if (project.customData.contains("RenderingCompositionLock")) {
                    compositionLock = project.customData["RenderingCompositionLock"];
                }
                static std::string nullPin = "";
                nullPin = "";
                static int previousNodeID = -1;

                static std::string selectedPin = nullPin;
 
                static std::optional<std::any> dispatcherTarget;
                static int selectedAttributeID = -1;
                bool mustDispatchOverlay = false;
                if (ImGui::BeginMenuBar()) {
                    if (!selectedNodes.empty()) {
                        auto nodeCandidate = Workspace::GetNodeByNodeID(selectedNodes.at(0));
                        if (nodeCandidate.has_value()) {
                            auto& node = nodeCandidate.value();
                            auto attributes = node->GetAttributesList();
                            for (auto& pin : node->outputPins) {
                                attributes.push_back(pin.linkedAttribute);
                            }
                            if (attributes.empty()) {
                                selectedPin = "";
                            }
/*                             if (!attributes.empty() && std::find(attributes.begin(), attributes.end(), selectedPin) == attributes.end()) {
                                selectedPin = *std::next(attributes.begin(), 0);
                            } */
                            if (!selectedNodes.empty() && (selectedPin.empty() || previousNodeID != selectedNodes[0])) {
                                if (selectedPinsMap.find(selectedNodes[0]) == selectedPinsMap.end()) {
                                    auto nodeCandidate = Workspace::GetNodeByNodeID(selectedNodes[0]);
                                    if (nodeCandidate.has_value()) {
                                        auto& node = nodeCandidate.value();
                                        if (!node->GetAttributesList().empty()) {
                                            selectedPinsMap[selectedNodes[0]] = node->GetAttributesList()[0];
                                        }
                                    }
                                }
                                if (selectedPinsMap.find(selectedNodes[0]) != selectedPinsMap.end()) {
                                    selectedPin = selectedPinsMap[selectedNodes[0]];
                                }
                            }
                        }
                    } else selectedPin = "";

                    if (selectedPin.empty() || compositionLock) {
                        if (Compositor::primaryFramebuffer.has_value()) {
                            dispatcherTarget = Compositor::primaryFramebuffer.value().GetFrontFramebufferWithoutSwapping();
                            mustDispatchOverlay = true;
                        }
                    }
                
                    auto& selectedCompositions = project.selectedCompositions;
                    if (!selectedNodes.empty() && !compositionLock) {
                        auto nodeCandidate = Workspace::GetNodeByNodeID(selectedNodes[0]);
                        if (nodeCandidate.has_value()) {
                            auto& node = nodeCandidate.value();
                            ImGui::Text("%s %s", ICON_FA_CIRCLE_NODES, node->Header().c_str());
                            ImGui::Separator();
                            ImGui::SameLine(0, 8.0f);
                        }
                    } else if (!selectedCompositions.empty()) {
                        auto compositionCandidate = Workspace::GetCompositionByID(selectedCompositions[0]);
                        if (compositionCandidate.has_value()) {
                            auto& composition = compositionCandidate.value();
                            static bool textHovered = false;
                            float factor = textHovered ? 0.7f : 1.0f;
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(factor));
                                ImGui::Text("%s%s %s", compositionLock ? ICON_FA_LOCK " " : ICON_FA_LOCK_OPEN " ", ICON_FA_LAYER_GROUP, composition->name.c_str());
                            ImGui::PopStyleColor();
                            textHovered = ImGui::IsItemHovered();
                            if (textHovered && ImGui::IsItemClicked() && ImGui::IsWindowFocused()) {
                                compositionLock = !compositionLock;
                            }
                            ImGui::Separator();
                            ImGui::SameLine(0, 8.0f);
                        }
                    } else {
                        ImGui::Text("%s %s", ICON_FA_LIST, project.name.c_str());
                        ImGui::Separator();
                        ImGui::SameLine(0, 8.0f);
                    }

                    if (!project.customData.contains("PreviewResolutionScale")) {
                        project.customData["PreviewResolutionScale"] = 1.0f;
                    }

                    float previewResolutionScale = project.customData["PreviewResolutionScale"];

                    std::string previewResolutionName = Localization::GetString("CUSTOM");
                    if (previewResolutionScale == 1.0f) previewResolutionName = Localization::GetString("FULL");
                    else if (previewResolutionScale == 0.5f) previewResolutionName = Localization::GetString("HALF");
                    else if (previewResolutionScale == 0.3f) previewResolutionName = Localization::GetString("THIRD");
                    else if (previewResolutionScale == 0.2f) previewResolutionName = Localization::GetString("QUARTER");

                    if (ImGui::MenuItem(FormatString("%s %s: %s", ICON_FA_IMAGE, Localization::GetString("PREVIEW_RESOLUTION").c_str(), previewResolutionName.c_str()).c_str())) {
                        ImGui::OpenPopup("##previewResolutionPresets");
                    }

                    if (ImGui::BeginPopup("##previewResolutionPresets")) {
                        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("PREVIEW_RESOLUTION").c_str()).c_str());
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_EXPAND, Localization::GetString("FULL").c_str()).c_str())) {
                            previewResolutionScale = 1.0f;
                        }
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_EXPAND, Localization::GetString("HALF").c_str()).c_str())) {
                            previewResolutionScale = 0.5f;
                        }
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_EXPAND, Localization::GetString("THIRD").c_str()).c_str())) {
                            previewResolutionScale = 0.3f;
                        }
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_EXPAND, Localization::GetString("QUARTER").c_str()).c_str())) {
                            previewResolutionScale = 0.2f;
                        }
                        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_EXPAND, Localization::GetString("CUSTOM").c_str()).c_str())) {
                            ImGui::SliderFloat("##customResolution", &previewResolutionScale, 0.1f, 1.0f, "%0.2f");
                            ImGui::EndMenu();
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::Separator();

                    project.customData["PreviewResolutionScale"] = previewResolutionScale;

                    Compositor::previewResolutionScale = previewResolutionScale;

                    int attributesCount = 0;
                    int selectedAttributeIndex = 0;
                    if (!selectedNodes.empty()) {
                        auto nodeCandidate = Workspace::GetNodeByNodeID(selectedNodes.at(0));
                        if (nodeCandidate.has_value() && !nodeCandidate.value()->GetAttributesList().empty()) {
                            auto& node = nodeCandidate.value();
                            auto attributes = node->GetAttributesList();
                            for (auto& pin : node->outputPins) {
                                attributes.push_back(pin.linkedAttribute);
                            }
                            attributesCount = attributes.size();
                            int attributeSearchIndex = 0;
                            for (auto& attribute : attributes) {
                                if (attribute == selectedPin) break;
                                attributeSearchIndex++;
                            }
                            selectedAttributeIndex = attributeSearchIndex;

                            std::vector<const char*> transformedAttributes;
                            for (auto& attribute : attributes) {
                                transformedAttributes.push_back(attribute.c_str());
                            }
                            ImGui::Text("%s", Localization::GetString("ATTRIBUTE").c_str());
                            try {
                                ImGui::PushItemWidth(ImGui::CalcTextSize(transformedAttributes[selectedAttributeIndex]).x + 50);
                                ImGui::Combo("##attributesList", &selectedAttributeIndex, transformedAttributes.data(), transformedAttributes.size());
                                ImGui::PopItemWidth();
                                selectedPin = transformedAttributes[selectedAttributeIndex];
                            } catch (...) {
                                // TODO: investigate this further
                            }

                            if (dispatcherTarget.has_value()) {
                                auto& value = dispatcherTarget.value();
                                ImGui::Text("%s %s", ICON_FA_CIRCLE_INFO, Workspace::GetTypeName(value).c_str());
                            }
                        }
                    }

                    if (!selectedNodes.empty()) {
                        selectedPinsMap[selectedNodes[0]] = selectedPin;
                    }

                    std::string timingText = FormatString("%s %0.1f ms", ICON_FA_STOPWATCH, AsyncRendering::s_renderTime);
                    ImVec2 timingTextSize = ImGui::CalcTextSize(timingText.c_str());
                    ImGui::SetCursorPosX(ImGui::GetWindowSize().x - ImGui::GetStyle().FramePadding.x - timingTextSize.x);
                    ImGui::Text("%s", timingText.c_str());

                    ImGui::EndMenuBar();
                }

                if (!selectedNodes.empty()) {
                    previousNodeID = selectedNodes[0];
                }
                project.customData["RenderingSelectedPinsMap"] = selectedPinsMap;
                project.customData["RenderingCompositionLock"] = compositionLock;

                static float miniTimelineSize = 20;
                ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
                if (ImGui::BeginChild("##renderPreview", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - miniTimelineSize))) {
                    bool attributeWasDispatched = false;
                    if (!selectedNodes.empty() && !compositionLock) {
                        auto nodeCandidate = Workspace::GetNodeByNodeID(selectedNodes.at(0));
                        if (nodeCandidate.has_value()) {
                            auto& node = nodeCandidate.value();
                            auto pinCandidate = node->GetAttributePin(selectedPin);
                            if (pinCandidate.has_value()) {
                                auto& pin = pinCandidate.value();
                                RASTER_SYNCHRONIZED(Workspace::s_pinCacheMutex);
                                if (Workspace::s_pinCache.GetFrontValue().find(pin.connectedPinID) != Workspace::s_pinCache.GetFrontValue().end()) {
                                    dispatcherTarget = Workspace::s_pinCache.GetFrontValue()[pin.connectedPinID];
                                }
                                if (Workspace::s_pinCache.GetFrontValue().find(pin.pinID) != Workspace::s_pinCache.GetFrontValue().end()) {
                                    dispatcherTarget = Workspace::s_pinCache.GetFrontValue()[pin.pinID];
                                }
                            } else {
                                // dispatcherTarget = node->GetDynamicAttribute(selectedPin);
                            }
                        }
                    }
                    if (dispatcherTarget.has_value()) {
                        auto& value = dispatcherTarget.value();
                        for (auto& dispatcher : Dispatchers::s_previewDispatchers) {
                            if (dispatcher.first == std::type_index(value.type())) {
                                dispatcher.second(value);
                                attributeWasDispatched = true;
                            }
                        }
                    }
                    if (!attributeWasDispatched) {
                        std::string warningText = Localization::GetString("PREVIEW_DISPATCHER_UNAVAILABLE");
                        ImGui::PushFont(Font::s_denseFont);
                        ImGui::SetWindowFontScale(1.7f);
                        ImVec2 warningSize = ImGui::CalcTextSize(warningText.c_str());
                        ImGui::SetCursorPos({
                            ImGui::GetContentRegionAvail().x / 2.0f - warningSize.x / 2.0f,
                            ImGui::GetContentRegionAvail().y / 2.0f - warningSize.y / 2.0f
                        });
                        ImGui::Text("%s", warningText.c_str());
                        ImGui::SetWindowFontScale(1.0f);
                        ImGui::PopFont();

                        if (dispatcherTarget.has_value()) {
                            std::string infoText = FormatString("%s: %s", Localization::GetString("NO_PREVIEW_DISPATCHER_FOR_TYPE").c_str(), dispatcherTarget.value().type().name());
                            ImVec2 infoSize = ImGui::CalcTextSize(infoText.c_str());
                            ImGui::SetCursorPos({
                                ImGui::GetContentRegionAvail().x / 2.0f - infoSize.x / 2.0f,
                                ImGui::GetContentRegionAvail().y / 2.0f - infoSize.y / 2.0f + warningSize.x / 1.7f / 2
                            });
                            ImGui::Text(infoText.c_str());
                        }
                    }
                    static ImVec2 settingsChildSize = ImVec2(30, 10);
                    ImGui::SetCursorPos({
                        ImGui::GetWindowSize().x - settingsChildSize.x,
                        0
                    });
                    if (ImGui::BeginChild("##settingsChild", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
                        if (!Dispatchers::s_enableOverlays) {
                            buttonColor = buttonColor * 0.8f;
                        }
                        ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
                        if (ImGui::Button(FormatString("%s %s", Dispatchers::s_enableOverlays ? ICON_FA_CHECK : ICON_FA_XMARK, ICON_FA_UP_DOWN_LEFT_RIGHT).c_str())) {
                            Dispatchers::s_enableOverlays = !Dispatchers::s_enableOverlays;
                        }
                        ImGui::SetItemTooltip("%s %s", ICON_FA_UP_DOWN_LEFT_RIGHT, Localization::GetString("ENABLE_PREVIEW_OVERLAYS").c_str());
                        ImGui::PopStyleColor();
                        settingsChildSize = ImGui::GetWindowSize();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndChild();

                if (ImGui::BeginChild("##miniTimeline", ImVec2(ImGui::GetContentRegionAvail().x, miniTimelineSize), ImGuiChildFlags_AutoResizeY)) {
                    static bool usingDragTimeline = false;
                    float firstTimelineCursorY = ImGui::GetCursorPosY();
                    std::string firstTimestampText = project.FormatFrameToTime(project.currentFrame);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
                    if (ImGui::Button(firstTimestampText.c_str())) {
                        usingDragTimeline = !usingDragTimeline;
                    }
                    ImGui::PopStyleColor(3);
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_FA_BACKWARD)) {
                        project.currentFrame -= 1;
                        project.OnTimelineSeek();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(project.playing ? ICON_FA_PAUSE : ICON_FA_PLAY)) {
                        project.playing = !project.playing;
                    }
                    ImGui::SameLine();
                    static float forwardSeekSize = 20;
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - forwardSeekSize);
                        if (!usingDragTimeline) {
                            ImGui::SliderFloat("##timelineSlider", &project.currentFrame, 0, project.GetProjectLength(), firstTimestampText.c_str());
                        } else {
                            ImGui::DragFloat("##timelineSlider", &project.currentFrame, 0.05f, 0, project.GetProjectLength(), FormatString("%s (%0.2f)", firstTimestampText.c_str(), project.currentFrame / project.framerate).c_str());
                        }
                        if (ImGui::IsItemEdited()) {
                            project.OnTimelineSeek();
                        }
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    float firstForwardSeekCursor = ImGui::GetCursorPosX();
                    buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
                    if (!project.looping) buttonColor = buttonColor * 0.8f;
                    ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
                        if (ImGui::Button(ICON_FA_CIRCLE_NOTCH)) {
                            project.looping = !project.looping;
                        }
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_FA_FORWARD)) {
                        project.currentFrame += 1;
                        project.OnTimelineSeek();
                    }
                    ImGui::SetItemTooltip("%s %s", ICON_FA_CIRCLE_NOTCH, Localization::GetString("LOOP_PLAYBACK").c_str());
                    ImGui::SameLine();
                    firstTimestampText = project.FormatFrameToTime(project.GetProjectLength() - project.currentFrame);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
                    if (ImGui::Button(firstTimestampText.c_str())) {
                        usingDragTimeline = !usingDragTimeline;
                    }
                    miniTimelineSize = ImGui::GetCursorPosY() - firstTimelineCursorY;
                    ImGui::PopStyleColor(3);
                    ImGui::SameLine();
                    forwardSeekSize = ImGui::GetCursorPosX() - firstForwardSeekCursor;
                    ImGui::NewLine();
                }
                ImGui::EndChild();
            }
        ImGui::End();
    }
};