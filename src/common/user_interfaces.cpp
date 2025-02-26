#include "common/user_interfaces.h"
#include "common/randomizer.h"
#include "common/libraries.h"
#include "common/easings.h"
#include "common/user_interface.h"

namespace Raster {
    std::vector<UserInterfaceImplementation> UserInterfaces::s_implementations;

    std::optional<UserInterfaceImplementation> UserInterfaces::GetUserInterfaceImplementationByPackageName(std::string t_packageName) {
        for (auto& entry : s_implementations) {
            if (entry.description.packageName == t_packageName) return entry;
        }
        return std::nullopt;
    }

    std::optional<AbstractUserInterface> UserInterfaces::InstantiateUserInterface(std::string t_packageName) {
        auto description = GetUserInterfaceImplementationByPackageName(t_packageName);
        if (description.has_value()) {
            auto userInterface = description.value().spawn();
            userInterface->packageName = t_packageName;
            return userInterface;
        }
        return std::nullopt;
    }

    std::optional<AbstractUserInterface> UserInterfaces::InstantiateSerializedUserInterface(Json t_data) {
        if (t_data.contains("PackageName")) {
            auto userInterfaceCandidate = InstantiateUserInterface(t_data["PackageName"]);
            if (userInterfaceCandidate.has_value()) {
                (*userInterfaceCandidate)->Load(t_data);
                return userInterfaceCandidate;
            }
        }
        return std::nullopt;
    }
};