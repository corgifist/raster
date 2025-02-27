#include "common/layout.h"
#include "common/localization.h"
#include "common/randomizer.h"
#include "common/user_interfaces.h"
#include "raster.h"

namespace Raster {

    Layout::Layout() {
        this->name = "New Layout";
        this->id = Randomizer::GetRandomInteger();
    }

    Layout::Layout(std::string t_name) {
        this->name = t_name;
        this->id = Randomizer::GetRandomInteger();
    }

    Layout::Layout(Json t_data) {
        this->id = t_data["ID"];
        this->name = t_data["Name"];
        for (auto& interface : t_data["UserInterfaces"]) {
            // DUMP_VAR(interface.dump());
            auto userInterfaceCandidate = UserInterfaces::InstantiateSerializedUserInterface(interface);
            if (userInterfaceCandidate) {
                // RASTER_LOG("pushing interface");
                windows.push_back(*userInterfaceCandidate);
            }
        }
    }

    Json Layout::Serialize() {
        Json result = Json::object();
        result["ID"] = id;
        result["Name"] = name;
        result["UserInterfaces"] = Json::array();
        for (auto& window : windows) {
            result["UserInterfaces"].push_back(window->Serialize());
        }
        return result;
    }

};