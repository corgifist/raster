#include "common/attribute.h"
#include "common/randomizer.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_drag.h"
#include "common/ui_shared.h"
#include "common/composition.h"
#include "common/workspace.h"

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
            {"ID", id}
        };
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
        drags.resize(keyframes.size());

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
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                s_selectedKeyframes = {t_keyframe.id};
            }

            keyframeDrag.Activate();

            float keyframeDragDistance;
            if (keyframeDrag.GetDragDistance(keyframeDragDistance)) {
                for (auto& keyframeID : s_selectedKeyframes) {
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
        }


        keyframeColor.w = 1.0f;
        DrawRect(keyframeBounds, keyframeColor);

        PopClipRect();
    }
};