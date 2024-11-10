#include "raster.h"
#include "common/common.h"

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
    }

    float Composition::GetOpacity(bool* attributeOpacityUsed, bool* correctOpacityTypeUsed) {
        auto& project = Workspace::s_project.value();
        if (opacityAttributeID > 0) {
            for (auto& attribute : attributes) {
                if (attribute->id == opacityAttributeID) {
                    std::any opacityCandidate = attribute->Get(project.currentFrame - beginFrame, this);
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
            if (IsInBounds(project.currentFrame, beginFrame, endFrame + 1)) {
                node->ClearAttributesCache();
                if (!audioMixing) node->ClearImmediateFooters();
            }
        }
        if (audioMixing && !audioEnabled) return;
        if (!IsInBounds(project.currentFrame, beginFrame, endFrame + 1)) return;
        if (!enabled) return;
        AbstractPinMap accumulator;
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
                    accumulator = node->Execute(accumulator, t_data);
                }
            }
        }
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
        data["Nodes"] = {};
        for (auto& pair : nodes) {
            data["Nodes"].push_back(pair.second->Serialize());
        }
        data["Attributes"] = {};
        for (auto& attribute : attributes) {
            data["Attributes"].push_back(attribute->Serialize());
        }
        return data;
    }
};