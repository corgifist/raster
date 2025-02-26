#pragma once

#include "raster.h"
#include "common/typedefs.h"
#include "common/user_interface.h"

namespace Raster {
    struct UserInterfaces {
        static std::vector<UserInterfaceImplementation> s_implementations;

        static std::optional<UserInterfaceImplementation> GetUserInterfaceImplementationByPackageName(std::string t_packageName);
        static std::optional<AbstractUserInterface> InstantiateUserInterface(std::string t_packageName);
        static std::optional<AbstractUserInterface> InstantiateSerializedUserInterface(Json t_data);
    };
};