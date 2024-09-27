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
        for (auto& node : data["Nodes"]) {
            auto nodeCandidate = Workspace::InstantiateSerializedNode(node);
            if (nodeCandidate.has_value()) {
                auto node = nodeCandidate.value();
                nodes.push_back(node);
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
        bool audioMixing = t_data.find("AUDIO_PASS") != t_data.end();
        for (auto& node : nodes) {
            if (audioMixing) break;
            node->executionsPerFrame = 0;
            node->ClearAttributesCache();
        }
        if (!IsInBounds(project.currentFrame, beginFrame, endFrame + 1)) return;
        if (!enabled) return;
        AbstractPinMap accumulator;
        for (auto& node : nodes) {
            if (node->flowInputPin.has_value()) {
                auto& flowInputPin = node->flowInputPin.value();
                bool anyPinConnected = false;
                for (auto& nodeCandidate : nodes) {
                    if (nodeCandidate->flowOutputPin.has_value() && nodeCandidate->flowOutputPin.value().connectedPinID == flowInputPin.pinID) {
                        anyPinConnected = true;
                        break;
                    }
                }
                if (!anyPinConnected) {
                    if (audioMixing && !node->DoesAudioMixing()) continue;
                    accumulator = node->Execute(accumulator, t_data);
                }
            }
        }
    }

    void Composition::OnTimelineSeek() {
        for (auto& node : nodes) {
            node->OnTimelineSeek();
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
        data["Nodes"] = {};
        for (auto& node : nodes) {
            data["Nodes"].push_back(node->Serialize());
        }
        data["Attributes"] = {};
        for (auto& attribute : attributes) {
            data["Attributes"].push_back(attribute->Serialize());
        }
        return data;
    }
};