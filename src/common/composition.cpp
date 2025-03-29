#include "common/composition.h"
#include "common/attribute.h"
#include "common/composition_mask.h"
#include "raster.h"
#include "common/common.h"
#include <algorithm>
#include <cmath>

namespace Raster {

    Composition::Composition() {
        this->id = Randomizer::GetRandomInteger();
        this->beginFrame = 0;
        this->endFrame = 60;
        this->name = "New Composition";
        this->description = "Empty Composition";
        this->blendMode = "";
        this->opacity = 1.0f;
        this->opacityAttributeID = -1;
        this->enabled = true;
        this->colorMark = Workspace::s_colorMarks[Workspace::s_defaultColorMark];
        this->audioEnabled = true;
        this->lockedCompositionID = -1;
        this->masks = {};
        this->cutTimeOffset = 0;
        this->identityState = false;
        this->speedAttributeID = this->pitchAttributeID = -1;
        this->lockPitchToSpeed = true;
        this->speed = this->pitch = 1.0f;
    }

    Composition::Composition(Json data) {
        this->id = data["ID"];
        this->beginFrame = data["BeginFrame"];
        this->endFrame = data["EndFrame"];
        this->name = data["Name"];
        this->description = data["Description"];
        this->blendMode = data["BlendMode"];
        this->opacity = data["Opacity"];
        this->opacityAttributeID = data["OpacityAttributeID"];
        this->enabled = data["Enabled"];
        this->colorMark = data["ColorMark"];
        this->audioEnabled = data.contains("AudioEnabled") ? data["AudioEnabled"].get<bool>() : true;
        this->lockedCompositionID = data.contains("LockedCompositionID") ? data["LockedCompositionID"].get<int>() : -1;
        this->cutTimeOffset = data.contains("CutTimeOffset") ? data["CutTimeOffset"].get<float>() : 0.0f;
        this->identityState = false;
        this->speedAttributeID = data.contains("SpeedAttributeID") ? data["SpeedAttributeID"].get<int>() : -1;
        this->pitchAttributeID = data.contains("PitchAttributeID") ? data["PitchAttributeID"].get<int>() : -1;
        this->lockPitchToSpeed = data.contains("LockPitchToSpeed") ? data["LockPitchToSpeed"].get<bool>() : true;
        this->speed = data.contains("Speed") ? data["Speed"].get<float>() : 1.0f;
        this->pitch = data.contains("Pitch") ? data["Pitch"].get<float>() : 1.0f;
        for (auto& node : data["Nodes"]) {
            auto nodeCandidate = Workspace::InstantiateSerializedNode(node);
            if (nodeCandidate.has_value()) {
                auto node = nodeCandidate.value();
                nodes[node->nodeID] = node;
            }
        }
        for (auto& composition : data["Attributes"]) {
            auto attributeCandidate = Attributes::InstantiateSerializedAttribute(composition);
            if (attributeCandidate.has_value()) {
                attributes.push_back(attributeCandidate.value());
            }
        }
        if (data.contains("Masks")) {
            for (auto& mask : data["Masks"]) {
                masks.push_back(CompositionMask(mask));
            }
        }
    }

    float Composition::GetOpacity(bool* attributeOpacityUsed, bool* correctOpacityTypeUsed) {
        auto& project = Workspace::s_project.value();
        if (opacityAttributeID > 0) {
            for (auto& attribute : attributes) {
                if (attribute->id == opacityAttributeID) {
                    std::any opacityCandidate = attribute->Get(project.GetCorrectCurrentTime() - beginFrame, this);
                    if (attributeOpacityUsed) *attributeOpacityUsed = true;
                    if (opacityCandidate.type() == typeid(float)) {
                        if (correctOpacityTypeUsed) *correctOpacityTypeUsed = true;
                        return std::any_cast<float>(opacityCandidate);
                    } else {
                        if (correctOpacityTypeUsed) *correctOpacityTypeUsed = false;
                    }
                }
            }
        }
        return opacity;
    }

    void Composition::Traverse(ContextData t_data) {
        auto& project = Workspace::GetProject();
        bool audioMixing = RASTER_GET_CONTEXT_VALUE(t_data, "AUDIO_PASS", bool);
        bool resetWorkspaceState = RASTER_GET_CONTEXT_VALUE(t_data, "RESET_WORKSPACE_STATE", bool);
        bool onlyAudioNodes = RASTER_GET_CONTEXT_VALUE(t_data, "ONLY_AUDIO_NODES", bool);
        bool onlyRenderingNodes = RASTER_GET_CONTEXT_VALUE(t_data, "ONLY_RENDERING_NODES", bool);
        // RASTER_SYNCHRONIZED(Workspace::s_nodesMutex);
        for (auto& pair : nodes) {
            if (!resetWorkspaceState) break;
            auto& node = pair.second;
            node->executionsPerFrame.SetBackValue(0);
            if (IsInBounds(project.GetCorrectCurrentTime(), GetBeginFrame() - 1, GetEndFrame() + 1)) {
                node->ClearAttributesCache();
            }
        }
        if (audioMixing && !audioEnabled) return;
        if (!IsInBounds(project.GetCorrectCurrentTime(), GetBeginFrame() - 1, GetEndFrame() + 1)) return;
        if (!enabled) return;
        AbstractPinMap accumulator;
        project.TimeTravel(cutTimeOffset);
        if (!RASTER_GET_CONTEXT_VALUE(t_data, "MANUAL_SPEED_CONTROL", bool)) project.SetFakeTime(beginFrame + MapTime(project.GetCorrectCurrentTime() - beginFrame));
        for (auto& pair : nodes) {
            auto& node = pair.second;
            if (node->flowInputPin.has_value()) {
                auto& flowInputPin = node->flowInputPin.value();
                bool anyPinConnected = false;
                for (auto& nodeCandidatePair : nodes) {
                    auto& nodeCandidate = nodeCandidatePair.second;
                    if (nodeCandidate->flowOutputPin.has_value() && nodeCandidate->flowOutputPin.value().connectedPinID == flowInputPin.pinID) {
                        anyPinConnected = true;
                        break;
                    }
                }
                if (!anyPinConnected) {
                    if (onlyAudioNodes && !node->DoesAudioMixing()) continue;
                    if (!onlyAudioNodes && node->DoesAudioMixing()) continue;
                    node->Execute(t_data);
                }
            }
        }
        if (!RASTER_GET_CONTEXT_VALUE(t_data, "MANUAL_SPEED_CONTROL", bool)) project.ResetFakeTime();
        project.ResetTimeTravel();
    }

    bool Composition::DoesAudioMixing() {
        for (auto& node : nodes) {
            if (!node.second->enabled || node.second->bypassed) continue;
            if (node.second->DoesAudioMixing()) return true;
        }
        return false;
    }

    bool Composition::DoesRendering() {
        for (auto& node : nodes) {
            if (!node.second->enabled || node.second->bypassed) continue;
            if (node.second->DoesRendering()) return true;
        }
        return false;
    }

    std::vector<int> Composition::GetUsedAudioBuses() {
        std::vector<int> result;
        for (auto& node : nodes) {
            auto ids = node.second->GetUsedAudioBuses();
            for (auto& id : ids) {
                if (std::find(result.begin(), result.end(), id) == result.end()) {
                    result.push_back(id);
                }
            }
        }
        return result;
     }

    void Composition::OnTimelineSeek() {
        for (auto& pair : nodes) {
            pair.second->OnTimelineSeek();
        }
    }

    static float InternalMapTime(Composition* t_composition, float t_time, bool t_inverseSpeed) {
        if (t_composition->speedAttributeID < 0) return t_time * (t_inverseSpeed ? 1.0f / t_composition->speed : t_composition->speed);
        auto lockedCompositionCandidate = Workspace::GetCompositionByID(t_composition->lockedCompositionID);
        int speedAttributeID = t_composition->speedAttributeID;
        if (lockedCompositionCandidate) speedAttributeID = (*lockedCompositionCandidate)->speedAttributeID;
        auto attributeCandidate = Workspace::GetAttributeByAttributeID(speedAttributeID);
        if (!attributeCandidate || (attributeCandidate && (*attributeCandidate)->packageName != RASTER_PACKAGED "float_attribute")) return t_time * (t_inverseSpeed ? 1.0f * t_composition->speed : t_composition->speed);
        auto& attribute = *attributeCandidate;
        if (attribute->keyframes.size() == 1) return t_time * (t_inverseSpeed ? 1.0f / std::any_cast<float>(attribute->keyframes[0].value) : std::any_cast<float>(attribute->keyframes[0].value));
        auto keyframes = attribute->keyframes;
        if (keyframes[keyframes.size() - 1].timestamp != t_composition->endFrame - t_composition->endFrame) {
            keyframes.push_back(AttributeKeyframe(t_composition->endFrame - t_composition->beginFrame, keyframes[keyframes.size() - 1].value));
        }
        float result = 0;
        for (int i = 0; i < keyframes.size() - 1; i++) {
            bool mustBreak = false;
            auto keyframe = keyframes[i];
            auto nextKeyframe = keyframes[i + 1];
            if (nextKeyframe.timestamp > t_time) {
                mustBreak = true;
                auto speed = std::any_cast<float>(keyframe.value);
                auto nextSpeed = std::any_cast<float>(nextKeyframe.value);
                auto percentage = GetPercentageInBounds(t_time, keyframe.timestamp, nextKeyframe.timestamp);
                nextKeyframe.value = glm::mix(speed, nextSpeed, percentage);
                nextKeyframe.timestamp = t_time;
            }
            auto speed = std::any_cast<float>(keyframe.value);
            auto nextSpeed = std::any_cast<float>(nextKeyframe.value);
            speed = glm::max(speed, 0.1f);
            nextSpeed = glm::max(nextSpeed, 0.1f);
            if (t_inverseSpeed) {
                speed = 1.0f / speed;
                nextSpeed = 1.0f / nextSpeed;
            }
            float width = (nextKeyframe.timestamp - keyframe.timestamp);
            float innerHeight = std::max(speed, nextSpeed) - std::min(speed, nextSpeed);
            float upperWidth = std::sqrt(width*width+innerHeight*innerHeight);
            float area = (upperWidth + width) / 2.0f * std::max(speed, nextSpeed);
            result += area;
            if (mustBreak) break;
        }
        return result;
    }

    float Composition::GetSpeed() {
        int attributeID = speedAttributeID;
        if (lockedCompositionID > 0) {
            auto lockedCompositionCandidate = Workspace::GetCompositionByID(lockedCompositionID);
            if (lockedCompositionCandidate) {
                attributeID = (*lockedCompositionCandidate)->speedAttributeID;
            }
        }
        auto attributeCandidate = Workspace::GetAttributeByAttributeID(attributeID);
        if (attributeCandidate) {
            auto& attribute = *attributeCandidate;
            auto speedCandidate = attribute->Get(Workspace::GetProject().GetCorrectCurrentTime() - beginFrame, this);
            if (speedCandidate.type() == typeid(float)) return glm::max(0.1f, std::any_cast<float>(speedCandidate));
        }
        return glm::max(speed, 0.1f);
    }

    float Composition::GetPitch() {
        int attributeID = pitchAttributeID;
        if (lockedCompositionID > 0) {
            auto lockedCompositionCandidate = Workspace::GetCompositionByID(lockedCompositionID);
            if (lockedCompositionCandidate) {
                attributeID = (*lockedCompositionCandidate)->pitchAttributeID;
            }
        }
        auto attributeCandidate = Workspace::GetAttributeByAttributeID(attributeID);
        if (attributeCandidate) {
            auto& attribute = *attributeCandidate;
            auto pitchCandidate = attribute->Get(Workspace::GetProject().GetCorrectCurrentTime() - beginFrame, this);
            if (pitchCandidate.type() == typeid(float)) return glm::max(0.1f, std::any_cast<float>(pitchCandidate));
        }
        return glm::max(pitch, 0.1f);
    }

    float Composition::GetBeginFrame() {
        return beginFrame;
    }

    float Composition::GetEndFrame() {
        return beginFrame + GetLength();
    }

    float Composition::MapTime(float t_time) {
        return InternalMapTime(this, t_time, false);
    }

    float Composition::GetLength() {
        return InternalMapTime(this, endFrame - beginFrame, true);
    }

    Json Composition::Serialize() {
        Json data = {};
        data["ID"] = id;
        data["BeginFrame"] = beginFrame;
        data["EndFrame"] = endFrame;
        data["Name"] = name;
        data["Description"] = description;
        data["BlendMode"] = blendMode;
        data["Opacity"] = opacity;
        data["OpacityAttributeID"] = opacityAttributeID;
        data["Enabled"] = enabled;
        data["ColorMark"] = colorMark;
        data["AudioEnabled"] = audioEnabled;
        data["LockedCompositionID"] = lockedCompositionID;
        data["CutTimeOffset"] = cutTimeOffset;
        data["SpeedAttributeID"] = speedAttributeID;
        data["PitchAttributeID"] = pitchAttributeID;
        data["LockPitchToSpeed"] = lockPitchToSpeed;
        data["Speed"] = speed;
        data["Pitch"] = pitch;
        data["Nodes"] = {};
        for (auto& pair : nodes) {
            data["Nodes"].push_back(pair.second->Serialize());
        }
        data["Attributes"] = {};
        for (auto& attribute : attributes) {
            data["Attributes"].push_back(attribute->Serialize());
        }
        data["Masks"] = {};
        for (auto& mask : masks) {
            data["Masks"].push_back(mask.Serialize());
        }
        return data;
    }
};