#include "raster.h"
#include "common/common.h"

namespace Raster {
    Project::Project(Json data) {
        this->name = data["Name"];
        this->description = data["Description"];
        this->framerate = data["Framerate"];
        this->currentFrame = data["CurrentFrame"];
        for (auto& composition : data["Compositions"]) {
            compositions.push_back(Composition(composition));
        }
    }

    Project::Project() {
        this->name = "Empty Project";
        this->description = "Nothing...";
        this->framerate = 60;
        this->currentFrame = 0;
    }

    uint64_t Project::GetProjectLength() {
        uint64_t candidate = UINT64_MAX;
        for (auto& composition : compositions) {
            if (composition.endFrame < candidate) {
                candidate = composition.endFrame;
            }
        }
        return candidate;
    }

    std::string Project::FormatFrameToTime(uint64_t frame) {
        auto transformedFrame = frame / framerate;
        float minutes = std::floor(transformedFrame / 60);
        float seconds = std::floor(remainder(transformedFrame, 60.0f));
        return FormatString("%02i:%02i", (int) minutes, (int) seconds);
    }

    Json Project::Serialize() {
        Json data = {};

        data["Name"] = name;
        data["Description"] = description;
        data["Framerate"] = framerate;
        data["CurrentFrame"] = currentFrame;
        data["Compositions"] = {};
        for (auto& composition : compositions) {
            data["Compositions"].push_back(composition.Serialize());
        }

        return data;
    }
};