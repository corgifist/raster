#include "common/examples.h"

namespace Raster {

    std::vector<Example> Examples::s_examples;

    Example::Example(Json t_data) {
        this->name = t_data["Name"];
        this->icon = t_data["Icon"];
        this->path = t_data["Path"];
    }

    void Examples::Initialize() {
        RASTER_LOG("loading examples.json");
        try {
            auto examplesJson = ReadJson("examples.json");
            for (auto& example : examplesJson["Examples"]) {
                RASTER_LOG("loading '" << example["Name"].get<std::string>() << "' example (" << example["Path"].get<std::string>() << ")");
                s_examples.push_back(Example(example));
            }
        } catch (...) {
            RASTER_LOG("failed to load examples.json");
        }
    }
};