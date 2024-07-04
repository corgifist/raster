#include "common/attribute.h"
#include "common/randomizer.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_drag.h"
#include "../ImGui/imgui_stdlib.h"
#include "common/ui_shared.h"
#include "common/composition.h"
#include "common/workspace.h"
#include "common/dispatchers.h"

namespace Raster {
    static void DrawRect(RectBounds bounds, ImVec4 color) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            bounds.UL, bounds.BR, ImGui::ColorConvertFloat4ToU32(color));
    }

    static bool MouseHoveringBounds(RectBounds bounds) {
        return ImGui::IsMouseHoveringRect(bounds.UL, bounds.BR);
    }

    static void PushClipRect(RectBounds bounds) {
        ImGui::GetWindowDrawList()->PushClipRect(bounds.UL, bounds.BR, true);
    }

    static void PopClipRect() { ImGui::GetWindowDrawList()->PopClipRect(); }

    AttributeKeyframe::AttributeKeyframe(float t_timestamp, std::any t_value) {
        this->id = Randomizer::GetRandomInteger();
        this->timestamp = t_timestamp;
        this->value = t_value;
    }

    AttributeKeyframe::AttributeKeyframe(int t_id, float t_timestamp, std::any t_value) {
        this->id = t_id;
        this->timestamp = t_timestamp;
        this->value = t_value;
    }

    AttributeBase::AttributeBase() {
        
    }

    void AttributeBase::Initialize() {
        this->name = "New Attribute";
        this->id = Randomizer::GetRandomInteger();

        this->composition = nullptr;
    }

    Json AttributeBase::Serialize() {
        return {
            {"PackageName", packageName},
            {"Name", name},
            {"ID", id},
            {"Data", AbstractSerialize()}
        };
    }

    std::any AttributeBase::Get(float t_frame, Composition* t_composition) {
        this->composition = composition;
        SortKeyframes();
        auto& project = Workspace::s_project.value();

        int targetKeyframeIndex = -1;
        int keyframesLength = keyframes.size();
        float renderViewTime = project.currentFrame - composition->beginFrame;

        for (int i = 0; i < keyframesLength; i++) {
            float keyframeTimestamp = keyframes.at(i).timestamp;
            if (renderViewTime <= keyframeTimestamp) {
                targetKeyframeIndex = i;
                break;
            }
        }

        if (targetKeyframeIndex == -1) {
            return keyframes.back().value;
        }

        if (targetKeyframeIndex == 0) {
            return keyframes.front().value;
        }

        float keyframeTimestamp = keyframes.at(targetKeyframeIndex).timestamp;
        float interpolationPercentage = 0;
        if (targetKeyframeIndex == 1) {
            interpolationPercentage = renderViewTime / keyframeTimestamp;
        } else {
            float previousFrame = keyframes.at(targetKeyframeIndex - 1).timestamp;
            interpolationPercentage = (renderViewTime - previousFrame) / (keyframeTimestamp - previousFrame);
        }

        auto& beginKeyframeValue = keyframes.at(targetKeyframeIndex - 1).value;
        auto& endkeyframeValue = keyframes.at(targetKeyframeIndex).value;

        return AbstractInterpolate(beginKeyframeValue, endkeyframeValue, interpolationPercentage, t_frame, t_composition);
    }

    void AttributeBase::RenderLegend(Composition* t_composition) {
        this->composition = t_composition;
        auto& project = Workspace::s_project.value();
        float currentFrame = project.currentFrame - t_composition->beginFrame;
        auto currentValue = Get(currentFrame, t_composition);
        ImGui::PushID(id);
            bool buttonPressed = ImGui::Button(KeyframeExists(currentFrame) ? ICON_FA_TRASH_CAN : ICON_FA_PLUS);
            bool shouldAddKeyframe = buttonPressed;
            ImGui::SameLine();
            ImGui::Text("%s %s", ICON_FA_LINK, name.c_str()); 
            ImGui::SetItemTooltip("%s %s", ICON_FA_ARROW_POINTER, Localization::GetString("RIGHT_CLICK_FOR_CONTEXT_MENU").c_str());
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(FormatString("##attribute%i", id).c_str());
            }
            if (ImGui::BeginPopup(FormatString("##attribute%i", id).c_str())) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_STOPWATCH, name.c_str()).c_str());
                if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT_METADATA").c_str()).c_str())) {
                    ImGui::InputText("##attributeName", &name);
                    ImGui::SetItemTooltip("%s %s", ICON_FA_PENCIL, Localization::GetString("ATTRIBUTE_NAME").c_str());
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_COPY, Localization::GetString("COPY_ATTRIBUTE_NAME").c_str()).c_str())) {
                    ImGui::SetClipboardText(name.c_str());
                }
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_CLONE, Localization::GetString("DUPLICATE").c_str()).c_str())) {
                    auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
                    parentComposition->attributes.push_back(Attributes::CopyAttribute(Attributes::InstantiateSerializedAttribute(Serialize()).value()).value());
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x);
                bool isItemEdited = false;
                currentValue = AbstractRenderLegend(t_composition, currentValue, isItemEdited);
                shouldAddKeyframe = shouldAddKeyframe || isItemEdited;
            ImGui::PopItemWidth();
        ImGui::PopID();

        if (shouldAddKeyframe && !KeyframeExists(currentFrame)) {
            keyframes.push_back(
                AttributeKeyframe(
                    currentFrame,
                    currentValue
                )
            );
        } else if (shouldAddKeyframe && !buttonPressed) {
            auto keyframeCandidate = GetKeyframeByTimestamp(currentFrame);
            if (keyframeCandidate.has_value()) {
                auto* keyframe = keyframeCandidate.value();
                keyframe->value = currentValue;
            }
        } else if (shouldAddKeyframe && buttonPressed) {
            auto indexCandidate = GetKeyframeIndexByTimestamp(currentFrame);
            if (indexCandidate.has_value()) {
                keyframes.erase(keyframes.begin() + indexCandidate.value());
            }
        }
    }

    void AttributeBase::SortKeyframes() {
        for (int step = 0; step < keyframes.size() - 1; ++step) {
            for (int i = 1; i < keyframes.size() - step - 1; ++i) {
                if (keyframes.at(i).timestamp > keyframes.at(i + 1).timestamp ) {
                    std::swap(keyframes.at(i), keyframes.at(i + 1));
                }
            }
        }
    }

    bool AttributeBase::KeyframeExists(float t_timestamp) {
        for (auto& keyframe : keyframes) {
            if (std::floor(t_timestamp) == std::floor(keyframe.timestamp)) return true;
        }
        return false;
    }

    std::optional<AttributeKeyframe*> AttributeBase::GetKeyframeByTimestamp(float t_timestamp) {
        for (auto& keyframe : keyframes) {
            if (std::floor(t_timestamp) == std::floor(keyframe.timestamp)) return &keyframe;
        }
        return std::nullopt;
    }

    std::optional<int> AttributeBase::GetKeyframeIndexByTimestamp(float t_timestamp) {
        int index;
        for (auto& keyframe : keyframes) {
            if (std::floor(keyframe.timestamp) == std::floor(t_timestamp)) return index;
            index++;
        }
        return std::nullopt;
    }

    static std::vector<int> s_selectedKeyframes;

    void AttributeBase::RenderKeyframe(AttributeKeyframe t_keyframe) {
        if (!composition) return;
        if (UIShared::s_timelineAttributeHeights.find(composition->id) == UIShared::s_timelineAttributeHeights.end()) return;
        SortKeyframes();
        PushClipRect(RectBounds(
            ImVec2(composition->beginFrame * UIShared::s_timelinePixelsPerFrame, ImGui::GetScrollY()),
            ImVec2((composition->endFrame - composition->beginFrame) * UIShared::s_timelinePixelsPerFrame, ImGui::GetWindowSize().y)
        ));
        float keyframeHeight = UIShared::s_timelineAttributeHeights[composition->id];
        float keyframeWidth = 6;
        RectBounds keyframeBounds(
            ImVec2((composition->beginFrame + t_keyframe.timestamp) * UIShared::s_timelinePixelsPerFrame - keyframeWidth / 2.0f, 0),
            ImVec2(keyframeWidth, keyframeHeight)
        );

        keyframeWidth *= 1.5f;
        RectBounds keyframeLogicBounds(
            ImVec2((composition->beginFrame + t_keyframe.timestamp) * UIShared::s_timelinePixelsPerFrame - keyframeWidth / 2.0f, 0),
            ImVec2(keyframeWidth, keyframeHeight)
        );

        ImVec4 keyframeColor = ImGui::GetStyleColorVec4(ImGuiCol_Separator);
        if (!MouseHoveringBounds(keyframeLogicBounds)) {
            keyframeColor = keyframeColor * 0.8f;
        }

        static std::unordered_map<int, std::vector<DragStructure>> s_keyframeDrags;
        if (s_keyframeDrags.find(id) == s_keyframeDrags.end()) {
            s_keyframeDrags[id] = {};
        }

        auto& drags = s_keyframeDrags[id];
        if (drags.size() != keyframes.size()) {
            drags = std::vector<DragStructure>(keyframes.size());
        }

        int targetKeyframeIndex = 0;
        for (auto& keyframe : keyframes) {
            if (t_keyframe.id == keyframe.id) break; 
            targetKeyframeIndex++;
        }

        auto& keyframeDrag = drags[targetKeyframeIndex];

        bool anyKeyframesDragged = false;
        for (auto& dragPair : s_keyframeDrags) {
            for (auto& iterableDrag : dragPair.second) {
                if (iterableDrag.isActive && iterableDrag.id != keyframeDrag.id)  {
                    anyKeyframesDragged = true;
                    break;
                }
            }
        }

        UIShared::s_timelineAnykeyframeDragged = UIShared::s_timelineAnykeyframeDragged || anyKeyframesDragged;

        if ((MouseHoveringBounds(keyframeLogicBounds) || keyframeDrag.isActive)) {
            if (ImGui::BeginTooltip()) {
                auto& project = Workspace::s_project.value();
                ImGui::Text("%s %s", ICON_FA_STOPWATCH, project.FormatFrameToTime(composition->beginFrame + t_keyframe.timestamp).c_str());
                Dispatchers::DispatchString(t_keyframe.value);
                ImGui::EndTooltip();
            }
            bool previousDragActive = keyframeDrag.isActive;
            keyframeDrag.Activate();
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                if (!ImGui::GetIO().KeyCtrl && !previousDragActive & s_selectedKeyframes.size() <= 1) {
                    s_selectedKeyframes = {t_keyframe.id};
                    std::cout << "overriding selected keyframes" << std::endl;
                } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyCtrl) {
                    auto keyframeIterator = std::find(s_selectedKeyframes.begin(), s_selectedKeyframes.end(), t_keyframe.id);
                    if (keyframeIterator == s_selectedKeyframes.end()) {
                        s_selectedKeyframes.push_back(t_keyframe.id);
                        std::cout << "appending selected keyframes" << std::endl;
                    } else if (ImGui::GetIO().KeyCtrl) {
                        std::cout << "removing unused keyframes" << std::endl;
                        s_selectedKeyframes.erase(keyframeIterator);
                    }
                }
            }

            float keyframeDragDistance;
            if (keyframeDrag.GetDragDistance(keyframeDragDistance) && !UIShared::s_timelineDragged) {
                for (auto& keyframeID : s_selectedKeyframes) {
                    bool breakDrag = false;
                    for (auto& testingKeyframeID : s_selectedKeyframes) {
                        auto selectedKeyframeCandidate = Workspace::GetKeyframeByKeyframeID(testingKeyframeID);
                        if (selectedKeyframeCandidate.has_value()) {
                            auto& selectedKeyframe = selectedKeyframeCandidate.value();
                            if (selectedKeyframe->timestamp <= 0) {
                                breakDrag = true;
                                break;
                            }
                        }
                    }
                    if (breakDrag) break;
                    auto selectedKeyframeCandidate = Workspace::GetKeyframeByKeyframeID(keyframeID);
                    if (selectedKeyframeCandidate.has_value()) {
                        auto& selectedKeyframe = selectedKeyframeCandidate.value();
                        selectedKeyframe->timestamp += keyframeDragDistance / UIShared::s_timelinePixelsPerFrame;
                        selectedKeyframe->timestamp = std::max(selectedKeyframe->timestamp, 1.0f);
                        selectedKeyframe->timestamp = std::min(selectedKeyframe->timestamp, composition->endFrame - composition->beginFrame);
                    }
                }
            } else {
                keyframeDrag.Deactivate();
            }
        } else if (!keyframeDrag.isActive) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().KeyCtrl) {
                auto keyframeIterator = std::find(s_selectedKeyframes.begin(), s_selectedKeyframes.end(), t_keyframe.id);
                if (keyframeIterator != s_selectedKeyframes.end() && !UIShared::s_timelineDragged && keyframeDrag.isActive) {
                    s_selectedKeyframes.erase(keyframeIterator);
                }

                bool oneKeyframeTouched = false;
                for (auto& keyframe : keyframes) {
                    RectBounds keyframeTestLogicBounds(
                        ImVec2((composition->beginFrame + keyframe.timestamp) * UIShared::s_timelinePixelsPerFrame - keyframeWidth / 2.0f, 0),
                        ImVec2(keyframeWidth, keyframeHeight)
                    );
                    if (MouseHoveringBounds(keyframeTestLogicBounds)) {
                        oneKeyframeTouched = true;
                    }
                }
                if (!oneKeyframeTouched) {
                    s_selectedKeyframes.clear();
                }
            } else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                for (auto& keyframeID : s_selectedKeyframes) {
                    auto selectedKeyframeCandidate = Workspace::GetKeyframeByKeyframeID(keyframeID);
                    if (selectedKeyframeCandidate.has_value()) {
                        auto& selectedKeyframe = selectedKeyframeCandidate.value();
                        selectedKeyframe->timestamp = std::floor(selectedKeyframe->timestamp);
                    }
                }
            }
        }


        if (std::find(s_selectedKeyframes.begin(), s_selectedKeyframes.end(), t_keyframe.id) != s_selectedKeyframes.end()) {
            keyframeColor = keyframeColor * 2;
        }
        keyframeColor.w = 1.0f;
        DrawRect(keyframeBounds, keyframeColor);

        PopClipRect();
    }

    std::vector<int> AttributeBase::m_deletedKeyframes;

    void AttributeBase::ProcessKeyframeShortcuts() {
        if (ImGui::Shortcut(ImGuiKey_Delete)) {
            m_deletedKeyframes = s_selectedKeyframes;
        }
        auto deletedKeyframes = m_deletedKeyframes;
        m_deletedKeyframes.clear();
        if (!deletedKeyframes.empty()) {
            for (auto& keyframeID : deletedKeyframes) {
                auto attributeCandidate = Workspace::GetAttributeByKeyframeID(keyframeID);
                if (attributeCandidate.has_value()) {
                    auto& attribute = attributeCandidate.value();
                    int keyframeIndex = 0;
                    for (auto& keyframe : attribute->keyframes) {
                        if (keyframe.id == keyframeID) break;
                        keyframeIndex++;
                    }
                    attribute->keyframes.erase(attribute->keyframes.begin() + keyframeIndex);
                }
            }
        }
    }
};