#include "raster.h"
#include "common/common.h"

namespace Raster {

    Composition::Composition() {
        this->id = Randomizer::GetRandomInteger();
        this->beginFrame = 0;
        this->endFrame = 60;
        this->name = "New Composition";
        this->description = "Empty Composition";
    }

    Composition::Composition(Json data) {
        this->id = data["ID"];
        this->beginFrame = data["BeginFrame"];
        this->endFrame = data["EndFrame"];
        this->name = data["Name"];
        this->description = data["Description"];
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

    Json Composition::Serialize() {
        Json data = {};
        data["ID"] = id;
        data["BeginFrame"] = beginFrame;
        data["EndFrame"] = endFrame;
        data["Name"] = name;
        data["Description"] = description;
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