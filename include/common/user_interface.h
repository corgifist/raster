#pragma once

#include "common/randomizer.h"
#include "font/IconsFontAwesome5.h"
#include "raster.h"
#include "typedefs.h"

namespace Raster {
    struct UserInterfaceBase {
    public:
        int id;
        std::string packageName;

        UserInterfaceBase() {
            this->id = Randomizer::GetRandomInteger();
        }

        void Render();
        Json Serialize();
        void Load(Json t_data);

    private:
        virtual void AbstractRender() {};

        virtual Json AbstractSerialize() { return {}; };
        virtual void AbstractLoad(Json t_Data) {};
    };

    using AbstractUserInterface = std::shared_ptr<UserInterfaceBase>;

    using UserInterfaceSpawnProcedure = std::function<AbstractUserInterface()>;

    struct UserInterfaceDescription {
        std::string prettyName;
        std::string packageName;
        std::string icon;
    };

    struct UserInterfaceImplementation {
        std::string libraryName;
        UserInterfaceDescription description;    
        UserInterfaceSpawnProcedure spawn;
    };
}