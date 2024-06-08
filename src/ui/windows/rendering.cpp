#include "rendering.h"
#include "font/font.h"

namespace Raster {
    void RenderingUI::Render() {
        ImGui::Begin(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("RENDERING").c_str()).c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollWithMouse);
            if (Workspace::s_project.has_value()) {
                auto& project = Workspace::s_project.value();
                auto& selectedNodes = Workspace::s_selectedNodes;

                static std::string selectedAttribute = "";
                static std::optional<std::any> dispatcherTarget;
                if (ImGui::BeginMenuBar()) {
                    if (!selectedNodes.empty()) {
                        auto nodeCandidate = Workspace::GetNodeByNodeID(selectedNodes.at(0));
                        if (nodeCandidate.has_value()) {
                            auto& node = nodeCandidate.value();
                            auto attributes = node->GetAttributesList();
                            for (auto& pin : node->outputPins) {
                                attributes.insert(pin.linkedAttribute);
                            }
                            if (attributes.empty()) {
                                selectedAttribute = "";
                            }
                            if (!attributes.empty() && attributes.find(selectedAttribute) == attributes.end()) {
                                selectedAttribute = *std::next(attributes.begin(), 0);
                            }
                        }
                    }
                    
                    ImGui::Text("%s", Localization::GetString("ATTRIBUTE").c_str());
                    int attributesCount = 0;
                    int selectedAttributeIndex = 0;
                    if (!selectedNodes.empty()) {
                        auto nodeCandidate = Workspace::GetNodeByNodeID(selectedNodes.at(0));
                        if (nodeCandidate.has_value()) {
                            auto& node = nodeCandidate.value();
                            auto attributes = node->GetAttributesList();
                            for (auto& pin : node->outputPins) {
                                attributes.insert(pin.linkedAttribute);
                            }
                            attributesCount = attributes.size();
                            int attributeSearchIndex = 0;
                            for (auto& attribute : attributes) {
                                if (attribute == selectedAttribute) break;
                                attributeSearchIndex++;
                            }
                            selectedAttributeIndex = attributeSearchIndex;

                            std::vector<const char*> transformedAttributes;
                            for (auto& attribute : attributes) {
                                transformedAttributes.push_back(attribute.c_str());
                            }
                            ImGui::PushItemWidth(ImGui::CalcTextSize(transformedAttributes[selectedAttributeIndex]).x + 50);
                            ImGui::Combo("##attributesList", &selectedAttributeIndex, transformedAttributes.data(), transformedAttributes.size());
                            ImGui::PopItemWidth();
                            selectedAttribute = transformedAttributes[selectedAttributeIndex];

                            if (dispatcherTarget.has_value()) {
                                auto& value = dispatcherTarget.value();
                                ImGui::Text("%s %s", ICON_FA_INFO, value.type().name());
                            }
                        }
                    }
                    ImGui::EndMenuBar();
                }

                static float miniTimelineSize = 20;
                ImGui::BeginChild("##renderPreview", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - miniTimelineSize));
                    bool attributeWasDispatched = false;
                    if (!selectedNodes.empty()) {
                        auto nodeCandidate = Workspace::GetNodeByNodeID(selectedNodes.at(0));
                        if (nodeCandidate.has_value()) {
                            auto& node = nodeCandidate.value();
                            auto pinCandidate = node->GetAttributePin(selectedAttribute);
                            if (pinCandidate.has_value()) {
                                auto& pin = pinCandidate.value();
                                if (Workspace::s_pinCache.find(pin.connectedPinID) != Workspace::s_pinCache.end()) {
                                    dispatcherTarget = Workspace::s_pinCache[pin.connectedPinID];
                                }
                                if (Workspace::s_pinCache.find(pin.pinID) != Workspace::s_pinCache.end()) {
                                    dispatcherTarget = Workspace::s_pinCache[pin.pinID];
                                }
                            } else {
                                dispatcherTarget = node->GetDynamicAttribute(selectedAttribute);
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
                            ImGui::PushFont(Font::s_denseFont);
                            std::string infoText = FormatString("%s: %s", Localization::GetString("NO_PREVIEW_DISPATCHER_FOR_TYPE").c_str(), dispatcherTarget.value().type().name());
                            // std::cout << infoText << std::endl;
                            ImVec2 infoSize = ImGui::CalcTextSize(infoText.c_str());
                            ImGui::SetCursorPos({
                                ImGui::GetContentRegionAvail().x / 2.0f - infoSize.x / 2.0f,
                                ImGui::GetContentRegionAvail().y / 2.0f - infoSize.y / 2.0f + warningSize.x / 1.7f / 2
                            });
                            ImGui::Text(infoText.c_str());
                            ImGui::PopFont();
                        }
                    }
                ImGui::EndChild();

                ImGui::BeginChild("##miniTimeline", ImVec2(ImGui::GetContentRegionAvail().x, miniTimelineSize), ImGuiChildFlags_AutoResizeY);
                    float firstTimelineCursorY = ImGui::GetCursorPosY();
                    std::string firstTimestampText = project.FormatFrameToTime(project.currentFrame);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
                    ImGui::Button(firstTimestampText.c_str());
                    ImGui::PopStyleColor(3);
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_FA_BACKWARD)) {
                        project.currentFrame -= 1;
                    }
                    ImGui::SameLine();
                    static float forwardSeekSize = 20;
                    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - forwardSeekSize);
                        int signedCurrentFrame = static_cast<int>(project.currentFrame);
                        ImGui::SliderInt("##timelineSlider", &signedCurrentFrame, 0, project.GetProjectLength(), firstTimestampText.c_str());
                        project.currentFrame = signedCurrentFrame;
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    float firstForwardSeekCursor = ImGui::GetCursorPosX();
                    if (ImGui::Button(ICON_FA_FORWARD)) {
                        project.currentFrame += 1;
                    }
                    ImGui::SameLine();
                    firstTimestampText = project.FormatFrameToTime(project.GetProjectLength() - project.currentFrame);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
                    ImGui::Button(firstTimestampText.c_str());
                    miniTimelineSize = ImGui::GetCursorPosY() - firstTimelineCursorY;
                    ImGui::PopStyleColor(3);
                    ImGui::SameLine();
                    forwardSeekSize = ImGui::GetCursorPosX() - firstForwardSeekCursor;
                    ImGui::NewLine();
                ImGui::EndChild();
            }
        ImGui::End();
    }
};