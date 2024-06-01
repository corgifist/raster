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
            auto nodeCandidate = Workspace::InstantiateSerializedNode(data);
            if (nodeCandidate.has_value()) {
                auto node = nodeCandidate.value();
                nodes.push_back(node);
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
        return data;
    }
};