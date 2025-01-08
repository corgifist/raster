#include "common/attribute.h"
#include "common/randomizer.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_drag.h"
#include "../ImGui/imgui_stdlib.h"
#include "../ImGui/imgui_stripes.h"
#include "common/ui_shared.h"
#include "common/composition.h"
#include "common/waveform_manager.h"
#include "common/workspace.h"
#include "common/dispatchers.h"
#include "common/easings.h"
#include "common/rendering.h"

namespace Raster {

    struct AttributeDuplicateBundle {
        Composition* targetComposition;
        AbstractAttribute attribute;
    };

    static std::vector<int> s_deletedAttributes;
    static std::vector<AttributeDuplicateBundle> s_duplicatedAttributes;

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

    void AttributeBase::Initialize() {
        this->name = "New Attribute";
        this->internalAttributeName = "";
        this->id = Randomizer::GetRandomInteger();

        this->composition = nullptr;
    }

    Json AttributeBase::Serialize() {
        Json serializedKeyframes = {};

        for (auto& keyframe : keyframes) {
            serializedKeyframes.push_back({
                {"Timestamp", keyframe.timestamp},
                {"ID", keyframe.id},
                {"Value", SerializeKeyframeValue(keyframe.value)},
                {"Easing", keyframe.easing.has_value() ? keyframe.easing.value()->Serialize() : nullptr}
            });
        }

        return {
            {"PackageName", packageName},
            {"Name", name},
            {"ID", id},
            {"Keyframes", serializedKeyframes},
            {"Data", AbstractSerialize()},
            {"InternalName", internalAttributeName}
        };
    }

    std::any AttributeBase::Get(float t_frame, Composition* t_composition) {
        this->composition = t_composition;
        SortKeyframes();
        auto& project = Workspace::s_project.value();

        int targetKeyframeIndex = -1;
        int keyframesLength = keyframes.size();
        float renderViewTime = t_frame;

        for (int i = 0; i < keyframesLength; i++) {
            float keyframeTimestamp = keyframes[i].timestamp;
            if (renderViewTime <= keyframeTimestamp) {
                targetKeyframeIndex = i;
                break;
            }
        }

        if (targetKeyframeIndex == -1) {
            return AbstractInterpolate(keyframes.back().value, keyframes.back().value, 0.0f, 0.0f, composition);
        }

        if (targetKeyframeIndex == 0) {
            return AbstractInterpolate(keyframes.front().value, keyframes.front().value, 0.0f, 0.0f, composition);
        }

        float keyframeTimestamp = keyframes[targetKeyframeIndex].timestamp;
        float interpolationPercentage = 0;
        if (targetKeyframeIndex == 1) {
            interpolationPercentage = renderViewTime / keyframeTimestamp;
        } else {
            float previousFrame = keyframes[targetKeyframeIndex - 1].timestamp;
            interpolationPercentage = (renderViewTime - previousFrame) / (keyframeTimestamp - previousFrame);
        }

        auto& beginKeyframeValue = keyframes[targetKeyframeIndex - 1].value;
        auto& endkeyframeValue = keyframes[targetKeyframeIndex].value;
        if (keyframes[targetKeyframeIndex].easing.has_value()) {
            interpolationPercentage = keyframes[targetKeyframeIndex].easing.value()->Get(interpolationPercentage);
        }

        return AbstractInterpolate(beginKeyframeValue, endkeyframeValue, interpolationPercentage, t_frame, t_composition);
    }

    static bool s_legendFocused = false;

    void AttributeBase::RenderLegend(Composition* t_composition) {
        this->composition = t_composition;
        s_legendFocused = ImGui::IsWindowFocused();
        auto& project = Workspace::s_project.value();
        float currentFrame = project.currentFrame - t_composition->beginFrame;
        auto currentValue = Get(currentFrame, t_composition);
        bool openRenamePopup = false;
        ImGui::PushID(id);
            bool graphButtonEnabled = true;
            if (keyframes.size() <= 1) graphButtonEnabled = false;
            if (!graphButtonEnabled) ImGui::BeginDisabled();
            std::string easingPopupID = FormatString("##easingPopupLegendID", id);
            if (ImGui::Button(ICON_FA_BEZIER_CURVE)) {
                ImGui::OpenPopup(easingPopupID.c_str());
            }
            ImGui::SameLine();
            if (!graphButtonEnabled) ImGui::EndDisabled();
            if (ImGui::BeginPopup(easingPopupID.c_str(), ImGuiWindowFlags_NoMove)) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_BEZIER_CURVE, name.c_str()).c_str());
                int targetKeyframeIndex = -1;
                int keyframesLength = keyframes.size();
                float renderViewTime = currentFrame;

                for (int i = 0; i < keyframesLength; i++) {
                    float keyframeTimestamp = keyframes.at(i).timestamp;
                    if (renderViewTime <= keyframeTimestamp) {
                        targetKeyframeIndex = i;
                        break;
                    }
                }
                if (targetKeyframeIndex != -1) {
                    auto& nextKeyframe = keyframes[targetKeyframeIndex];
                    if (nextKeyframe.easing.has_value()) {
                        nextKeyframe.easing.value()->RenderDetails();
                    } else {
                        for (auto& implementation : Easings::s_implementations) {
                            bool isEasingSelected = nextKeyframe.easing.has_value() && nextKeyframe.easing.value()->packageName == implementation.description.packageName;
                            if (ImGui::MenuItem(FormatString("%s%s %s", isEasingSelected ? ICON_FA_CHECK " " : "", ICON_FA_BEZIER_CURVE, implementation.description.prettyName.c_str()).c_str())) {
                                nextKeyframe.easing = Easings::InstantiateEasing(implementation.description.packageName);
                            }
                        }
                    }
                }
                ImGui::EndPopup();
            }
            bool buttonPressed = ImGui::Button(KeyframeExists(currentFrame) && keyframes.size() != 1 ? ICON_FA_TRASH_CAN : ICON_FA_PLUS);
            bool shouldAddKeyframe = buttonPressed;
            ImGui::SameLine();
            static std::unordered_map<int, bool> s_attributeTextHovered;
            static std::unordered_map<int, bool> s_attributeTextClicked;
            if (s_attributeTextHovered.find(id) == s_attributeTextHovered.end()) {
                s_attributeTextHovered[id] = false;
                s_attributeTextClicked[id] = false;
            }
            auto& selectedAttributes = project.selectedAttributes;

            bool& attributeTextHovered = s_attributeTextHovered[id];
            bool& attributeTextClicked = s_attributeTextClicked[id];
            attributeTextHovered = attributeTextHovered || std::find(selectedAttributes.begin(), selectedAttributes.end(), id) != selectedAttributes.end();

            ImVec4 textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
            if (attributeTextHovered) textColor = textColor * 0.9f;
            if (attributeTextClicked) textColor = textColor * 0.9f;
            ImGui::PushStyleColor(ImGuiCol_Text, textColor);
                ImGui::Text("%s%s%s %s", internalAttributeName.empty() ? "" : ICON_FA_CIRCLE_NODES " ", t_composition->opacityAttributeID == id ? ICON_FA_DROPLET " " : "", ICON_FA_LINK, name.c_str()); 
            ImGui::PopStyleColor();
            if (!internalAttributeName.empty()) {
                ImGui::SetItemTooltip("%s %s: %s", ICON_FA_CIRCLE_INFO, Localization::GetString("INTERNAL_NAME").c_str(), internalAttributeName.c_str());
            }

            std::string renamePopupID = FormatString("##renameAttribute%i", id);
            if (ImGui::IsItemHovered() && ImGui::IsWindowFocused() && ImGui::GetIO().MouseDoubleClicked[ImGuiMouseButton_Left]) {
                openRenamePopup = true;
            }

            attributeTextHovered = ImGui::IsItemHovered();
            attributeTextClicked = ImGui::IsItemClicked();
            if (attributeTextClicked && !ImGui::GetIO().KeyCtrl) {
                project.selectedAttributes = {id};
            } else if (attributeTextClicked && ImGui::GetIO().KeyCtrl) {
                auto attributeIterator = std::find(selectedAttributes.begin(), selectedAttributes.end(), id);
                if (attributeIterator == selectedAttributes.end()) {
                    selectedAttributes.push_back(id);
                } else {
                    selectedAttributes.erase(attributeIterator);
                }
            }

            if (ImGui::BeginItemTooltip()) {
                if (t_composition->opacityAttributeID == id) {
                    ImGui::Text("%s %s", ICON_FA_DROPLET, Localization::GetString("USED_AS_OPACITY_ATTRIBUTE").c_str());
                }
                ImGui::Text("%s %s", ICON_FA_ARROW_POINTER, Localization::GetString("RIGHT_CLICK_FOR_CONTEXT_MENU").c_str());
                ImGui::EndTooltip();
            }
            
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                AttributeDragDropPayload payload;
                payload.attributeID = id;
                ImGui::SetDragDropPayload(ATTRIBUTE_TIMELINE_PAYLOAD, &payload, sizeof(payload));
                ImGui::Text("%s %s", ICON_FA_STOPWATCH, name.c_str());
                AbstractRenderDetails();
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ATTRIBUTE_TIMELINE_PAYLOAD)) {
                    AttributeDragDropPayload attributePayload = *(AttributeDragDropPayload*) payload->Data;
                    int fromAttributeID = attributePayload.attributeID;
                    int toAttributeID = id;

                    if (Workspace::s_project.has_value()) {
                        auto& project = Workspace::s_project.value();
                        for (auto& composition : project.compositions) {
                            for (auto& attribute : composition.attributes) {
                                if (attribute->id == fromAttributeID) {
                                    AbstractAttribute& fromAttribute = attribute;
                                    for (auto& anotherAttribute : composition.attributes) {
                                        if (anotherAttribute->id == toAttributeID) {
                                            AbstractAttribute& toAttribute = anotherAttribute;
                                            std::swap(fromAttribute, anotherAttribute);
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                ImGui::EndDragDropTarget();
            }
            
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(FormatString("##attribute%i", id).c_str());
            }
            if (ImGui::BeginPopup(FormatString("##attribute%i", id).c_str())) {
                ImGui::SeparatorText(FormatString("%s%s %s (%i)", t_composition->opacityAttributeID == id ? ICON_FA_DROPLET " " : "", ICON_FA_STOPWATCH, name.c_str(), id).c_str());
                RenderAttributePopup(t_composition);
                ImGui::EndPopup();
            }
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x);
                bool isItemEdited = false;
                currentValue = AbstractRenderLegend(t_composition, currentValue, isItemEdited);
                shouldAddKeyframe = shouldAddKeyframe || isItemEdited;
                if (shouldAddKeyframe) {
                    Rendering::ForceRenderFrame();
                    WaveformManager::RequestWaveformRefresh(t_composition->id);
                }
            ImGui::PopItemWidth();
        ImGui::PopID();

        if (openRenamePopup) ImGui::OpenPopup(renamePopupID.c_str());

        static bool renameFieldFocused = false;
        if (ImGui::BeginPopup(renamePopupID.c_str())) {
            if (!renameFieldFocused) {
                ImGui::SetKeyboardFocusHere(0);
                renameFieldFocused = true;
            }
            ImGui::InputTextWithHint("##renameField", FormatString("%s %s", ICON_FA_FONT, Localization::GetString("ATTRIBUTE_NAME").c_str()).c_str(), &name);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        } else renameFieldFocused = false;

        if (shouldAddKeyframe && keyframes.size() == 1 && !buttonPressed) {
            keyframes[0].value = currentValue;
        } else if (shouldAddKeyframe && !KeyframeExists(currentFrame)) {
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
        } else if (shouldAddKeyframe && buttonPressed && keyframes.size() != 1) {
            auto indexCandidate = GetKeyframeIndexByTimestamp(currentFrame);
            if (indexCandidate.has_value()) {
                keyframes.erase(keyframes.begin() + indexCandidate.value());
            }
        }
    }

    void AttributeBase::RenderAttributePopup(Composition* t_composition) {
        RenderPopup();
        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT_METADATA").c_str()).c_str())) {
            ImGui::InputText("##attributeName", &name);
            ImGui::SetItemTooltip("%s %s", ICON_FA_PENCIL, Localization::GetString("ATTRIBUTE_NAME").c_str());
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_COPY, Localization::GetString("COPY_ATTRIBUTES_DATA").c_str()).c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_COPY, Localization::GetString("COPY_ATTRIBUTES_DATA").c_str()).c_str());
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_COPY, Localization::GetString("ID").c_str()).c_str())) {
                ImGui::SetClipboardText(std::to_string(id).c_str());
            }
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_COPY, Localization::GetString("NAME").c_str()).c_str())) {
                ImGui::SetClipboardText(name.c_str());
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem(FormatString("%s%s %s", t_composition->opacityAttributeID == id ? ICON_FA_CHECK " " : "", ICON_FA_DROPLET, Localization::GetString("USE_AS_OPACITY_ATTRIBUTE").c_str()).c_str())) {
            t_composition->opacityAttributeID = id;
        }
        ImGui::Separator();
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_CLONE, Localization::GetString("DUPLICATE_ATTRIBUTE").c_str()).c_str(), "Ctrl+D")) {
            auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
            s_duplicatedAttributes.push_back(AttributeDuplicateBundle{
                .targetComposition = parentComposition,
                .attribute = Attributes::CopyAttribute(Attributes::InstantiateSerializedAttribute(Serialize()).value()).value()
            });
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("DELETE_ATTRIBUTE").c_str()).c_str(), "Delete")) {
            s_deletedAttributes.push_back(id);
        }
    }

    void AttributeBase::SortKeyframes() {
        for (int step = 0; step < keyframes.size() - 1; ++step) {
            for (int i = 1; i < keyframes.size() - step - 1; ++i) {
                if (keyframes[i].timestamp > keyframes[i + 1].timestamp ) {
                    std::swap(keyframes[i], keyframes[i + 1]);
                }
            }
        }

        for (int i = 0; i < keyframes.size(); i++) {
            AttributeKeyframe& stamp = keyframes.at(i);
            for (int j = i + 1; j < keyframes.size(); j++) {
                if (int(stamp.timestamp) ==
                    int(keyframes[j].timestamp)) {
                    stamp.timestamp = int(keyframes[j].timestamp) + 1;
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

    std::optional<int> AttributeBase::GetKeyframeIndexByID(int t_id) {
        int index;
        for (auto& keyframe : keyframes) {
            if (keyframe.id == t_id) return index;
            index++;
        }
        return std::nullopt;
    }


    static bool s_timelineFocused = false;

    void AttributeBase::RenderKeyframe(AttributeKeyframe& t_keyframe) {
        auto& project = Workspace::GetProject();
        auto& selectedKeyframes = project.selectedKeyframes;
        s_timelineFocused = ImGui::IsWindowFocused();
        if (!composition) return;
        if (UIShared::s_timelineAttributeHeights.find(composition->id) == UIShared::s_timelineAttributeHeights.end()) return;
        SortKeyframes();
        float keyframeYOffset = -3;
        PushClipRect(RectBounds(
            ImVec2(composition->beginFrame * UIShared::s_timelinePixelsPerFrame, keyframeYOffset),
            ImVec2((composition->endFrame - composition->beginFrame) * UIShared::s_timelinePixelsPerFrame, ImGui::GetWindowSize().y)
        ));
        float keyframeHeight = UIShared::s_timelineAttributeHeights[composition->id];
        float keyframeWidth = 9;
        RectBounds keyframeBounds(
            ImVec2((composition->beginFrame + std::floor(t_keyframe.timestamp)) * UIShared::s_timelinePixelsPerFrame - keyframeWidth / 2.0f, keyframeYOffset),
            ImVec2(keyframeWidth, keyframeHeight)
        );

        keyframeWidth *= 1.5f;
        RectBounds keyframeLogicBounds(
            ImVec2((composition->beginFrame + std::floor(t_keyframe.timestamp)) * UIShared::s_timelinePixelsPerFrame - keyframeWidth / 2.0f, keyframeYOffset),
            ImVec2(keyframeWidth, keyframeHeight)
        );

        ImVec4 keyframeColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        ImVec4 baseKeyframeColor = keyframeColor;
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


        std::string popupID = FormatString("##attributeKeyframePopup%i", t_keyframe.id);

        bool keyframePopupActive = false;

        if (ImGui::BeginPopup(popupID.c_str())) {
            RenderKeyframePopup(t_keyframe);
            keyframePopupActive = true;
            UIShared::s_timelineBlockPopup = true;
            ImGui::EndPopup();
        }

        if ((MouseHoveringBounds(keyframeLogicBounds) || keyframeDrag.isActive)) {
            if (!keyframePopupActive && ImGui::BeginTooltip()) {
                auto& project = Workspace::s_project.value();
                ImGui::Text("%s %s", ICON_FA_STOPWATCH, project.FormatFrameToTime(composition->beginFrame + t_keyframe.timestamp).c_str());
                Dispatchers::DispatchString(t_keyframe.value);
                ImGui::EndTooltip();
            }
            bool previousDragActive = keyframeDrag.isActive;
            keyframeDrag.Activate();
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowFocused() && !UIShared::s_timelineDragged) {
                if (!ImGui::GetIO().KeyCtrl && !previousDragActive & selectedKeyframes.size() <= 1) {
                    selectedKeyframes = {t_keyframe.id};
                    UIShared::s_lastClickedObjectType = LastClickedObjectType::Keyframe;
                } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::GetIO().KeyCtrl) {
                    auto keyframeIterator = std::find(selectedKeyframes.begin(), selectedKeyframes.end(), t_keyframe.id);
                    if (keyframeIterator == selectedKeyframes.end()) {
                        selectedKeyframes.push_back(t_keyframe.id);
                        UIShared::s_lastClickedObjectType = LastClickedObjectType::Keyframe;
                    } else if (ImGui::GetIO().KeyCtrl) {
                        selectedKeyframes.erase(keyframeIterator);
                    }
                }
            }

            if (ImGui::IsMouseDown(ImGuiMouseButton_Right) && ImGui::IsWindowFocused() && MouseHoveringBounds(keyframeLogicBounds)) {
                ImGui::OpenPopup(popupID.c_str());
                UIShared::s_timelineBlockPopup = true;
            }

            float keyframeDragDistance;
            if (keyframeDrag.GetDragDistance(keyframeDragDistance) && !UIShared::s_timelineDragged && ImGui::IsWindowFocused()) {
                for (auto& keyframeID : selectedKeyframes) {
                    bool breakDrag = false;
                    for (auto& testingKeyframeID : selectedKeyframes) {
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
                auto keyframeIterator = std::find(selectedKeyframes.begin(), selectedKeyframes.end(), t_keyframe.id);
                if (keyframeIterator != selectedKeyframes.end() && !UIShared::s_timelineDragged && keyframeDrag.isActive) {
                    selectedKeyframes.erase(keyframeIterator);
                }

                bool oneKeyframeTouched = false;
                for (auto& keyframe : keyframes) {
                    RectBounds keyframeTestLogicBounds(
                        ImVec2((composition->beginFrame + keyframe.timestamp) * UIShared::s_timelinePixelsPerFrame - keyframeWidth / 2.0f, 0),
                        ImVec2(keyframeWidth, keyframeHeight)
                    );
                    if (MouseHoveringBounds(keyframeTestLogicBounds) || !ImGui::IsWindowHovered() || !UIShared::s_timelineDragged) {
                        oneKeyframeTouched = true;
                    }
                }
                if (!oneKeyframeTouched) {
                    for (auto& keyframe : keyframes) {
                        auto keyframeIterator = std::find(selectedKeyframes.begin(), selectedKeyframes.end(), keyframe.id);
                        if (keyframeIterator != selectedKeyframes.end()) {
                            selectedKeyframes.erase(keyframeIterator);
                        }
                    }
                    UIShared::s_lastClickedObjectType = LastClickedObjectType::Composition;
                }
            } else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                for (auto& keyframeID : selectedKeyframes) {
                    auto selectedKeyframeCandidate = Workspace::GetKeyframeByKeyframeID(keyframeID);
                    if (selectedKeyframeCandidate.has_value()) {
                        auto& selectedKeyframe = selectedKeyframeCandidate.value();
                        selectedKeyframe->timestamp = std::floor(selectedKeyframe->timestamp);
                    }
                }
            }
        }

        if (std::find(selectedKeyframes.begin(), selectedKeyframes.end(), t_keyframe.id) != selectedKeyframes.end()) {
            keyframeColor = keyframeColor * 2;
        }
        keyframeColor.w = 1.0f;
        DrawRect(keyframeBounds, keyframeColor);
        
        auto& keyframeIndex = targetKeyframeIndex;
        if (keyframeIndex + 1 < keyframes.size()) {
            auto& nextKeyframe = keyframes[keyframeIndex + 1];
            float timestampDifference = nextKeyframe.timestamp - t_keyframe.timestamp;
            float distanceBetweenKeyframes = timestampDifference * UIShared::s_timelinePixelsPerFrame;

            if (nextKeyframe.easing.has_value()) {
                auto& nextEasing = nextKeyframe.easing.value();
                const int SMOOTHNESS = 64;
                std::vector<float> percentages(SMOOTHNESS);
                float step = 1.0f / (float) SMOOTHNESS;
                for (int i = 0; i < SMOOTHNESS; i++) {
                    percentages[i] = std::clamp(nextEasing->Get(i * step), 0.0f, 0.95f);
                }

                ImVec2 canvasPos = keyframeBounds.BR;
                canvasPos.y -= 2;

                ImVec2 lastLinePosition = canvasPos;
                lastLinePosition.y -= percentages[0] * (keyframeBounds.size.y - 2);

                float distanceStep = distanceBetweenKeyframes / SMOOTHNESS;

                auto reservedCursor = ImGui::GetCursorPos();
                ImGui::SetCursorPos(reservedCursor + ImVec2{
                    keyframeBounds.pos.x + keyframeBounds.size.x,
                    keyframeBounds.pos.y
                });
                ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 20, 28, ImVec2(distanceBetweenKeyframes, keyframeBounds.size.y));
                ImGui::SetCursorPos(reservedCursor);

                RectBounds stripesBounds(
                    ImVec2(keyframeBounds.pos.x + keyframeBounds.size.x, 0),
                    ImVec2(distanceBetweenKeyframes, keyframeBounds.size.y)
                );
                ImVec4 graphColor = ImGui::GetStyleColorVec4(ImGuiCol_PlotLines);

                std::string easingPopupID = FormatString("##easingPopup%i", nextKeyframe.id);

                if (MouseHoveringBounds(stripesBounds) && ImGui::IsWindowFocused()) {
                    if (ImGui::GetIO().MouseClicked[ImGuiMouseButton_Left]) {
                        graphColor = graphColor * 1.5f;
                    }
                    if (ImGui::GetIO().MouseDoubleClicked[ImGuiMouseButton_Left]) {
                        ImGui::OpenPopup(easingPopupID.c_str());
                    }
                    if (ImGui::GetIO().MouseClicked[ImGuiMouseButton_Left]) {
                        selectedKeyframes = {nextKeyframe.id};
                    }
                    ImGui::SetTooltip("%s %s", ICON_FA_CIRCLE_QUESTION " " ICON_FA_BEZIER_CURVE, Localization::GetString("DOUBLE_CLICK_FOR_EASING_EDITING").c_str());
                    graphColor = graphColor * 1.2f;
                }
                graphColor.w = 1.0f;

                if (ImGui::BeginPopup(easingPopupID.c_str(), ImGuiWindowFlags_NoMove)) {
                    ImGui::SeparatorText(FormatString("%s %s", ICON_FA_BEZIER_CURVE, nextEasing->prettyName.c_str()).c_str());
                    nextEasing->RenderDetails();
                    ImGui::EndPopup();
                }


                for (int i = 1; i < SMOOTHNESS; i++) {
                    ImVec2 nextLinePosition = canvasPos;
                    nextLinePosition.x += distanceStep * i;
                    nextLinePosition.y -= percentages[i] * (keyframeBounds.size.y - 2);

                    ImGui::GetWindowDrawList()->AddLine(lastLinePosition, nextLinePosition, ImGui::GetColorU32(graphColor), 2);

                    lastLinePosition = nextLinePosition;
                }
            } 
        } 


        PopClipRect();
    }

    void AttributeBase::RenderKeyframePopup(AttributeKeyframe& t_keyframe) {
        auto& selectedKeyframes = Workspace::GetProject().selectedKeyframes;
        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_STOPWATCH, name.c_str()).c_str());
        RenderPopup();
        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_BEZIER_CURVE, Localization::GetString("KEYFRAME_EASING").c_str()).c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_BEZIER_CURVE, Localization::GetString("KEYFRAME_EASING").c_str()).c_str());
            if (t_keyframe.easing.has_value()) {
                t_keyframe.easing.value()->RenderDetails();
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("NO_EASING").c_str()).c_str())) {
                t_keyframe.easing = std::nullopt;
            }
            for (auto& implementation : Easings::s_implementations) {
                bool isEasingSelected = t_keyframe.easing.has_value() && t_keyframe.easing.value()->packageName == implementation.description.packageName;
                if (ImGui::MenuItem(FormatString("%s%s %s", isEasingSelected ? ICON_FA_CHECK " " : "", ICON_FA_BEZIER_CURVE, implementation.description.prettyName.c_str()).c_str())) {
                    t_keyframe.easing = Easings::InstantiateEasing(implementation.description.packageName);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("DELETE_SELECTED_KEYFRAMES").c_str()).c_str(), "Delete")) {
            m_deletedKeyframes = selectedKeyframes;
            selectedKeyframes.clear();
        }
        ImGui::Separator();
        ImGui::Text("%s %s %i %s", ICON_FA_CIRCLE_INFO, Localization::GetString("TOTAL").c_str(), (int) keyframes.size(), Localization::GetString("KEYFRAMES").c_str());
    }

    void AttributeBase::RenderPopup() {
        AbstractRenderPopup();
    }

    std::vector<int> AttributeBase::m_deletedKeyframes;

    void AttributeBase::ProcessKeyframeShortcuts() {
        if (!Workspace::s_project.has_value()) return;
        auto& project = Workspace::GetProject();
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && s_timelineFocused && UIShared::s_lastClickedObjectType == LastClickedObjectType::Keyframe) {
            auto& selectedKeyframes = Workspace::GetProject().selectedKeyframes;
            m_deletedKeyframes = selectedKeyframes;
            selectedKeyframes.clear();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && s_legendFocused) {
            s_deletedAttributes = project.selectedAttributes;
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
        
        auto deletedAttributes = s_deletedAttributes;
        s_deletedAttributes.clear();
        if (!deletedAttributes.empty()) {
            for (auto& id : deletedAttributes) {
                auto compositionCandidate = Workspace::GetCompositionByAttributeID(id);
                if (compositionCandidate.has_value()) {
                    auto& composition = compositionCandidate.value();
                    int attributeIndex = 0;
                    for (auto& attribute : composition->attributes) {
                        if (attribute->id == id) break;
                        attributeIndex++;
                    }
                    composition->attributes.erase(composition->attributes.begin() + attributeIndex);
                }
            }
        }

        auto duplicatedAttributes = s_duplicatedAttributes;
        s_duplicatedAttributes.clear();
        for (auto& bundle : duplicatedAttributes) {
            bundle.targetComposition->attributes.push_back(bundle.attribute);
        }
    }
};