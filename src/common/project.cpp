#include "raster.h"
#include "common/common.h"

namespace Raster {
    Project::Project(Json data) {
        this->name = data["Name"];
        this->description = data["Description"];
        this->framerate = data["Framerate"];
        for (auto& composition : data["Compositions"]) {
            compositions.push_back(Composition(composition));
        }
    }

    Project::Project() {
        this->name = "Empty Project";
        this->description = "Nothing...";
        this->framerate = 60;
    }

    Json Project::Serialize() {
        Json data = {};

        data["Name"] = name;
        data["Description"] = description;
        data["Framerate"] = framerate;
        data["Compositions"] = {};
        for (auto& composition : compositions) {
            data["Compositions"].push_back(composition.Serialize());
        }

        return data;
    }
};