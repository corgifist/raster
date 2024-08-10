#pragma once

#include "raster.h"
#include "typedefs.h"
#include "dylib.hpp"
#include "randomizer.h"

namespace Raster {
    struct EasingBase {
    public:
        int id;
        std::string prettyName, icon;
        std::string packageName;

        void Initialize();
        void RenderDetails();

        virtual float Get(float t_percentage) = 0;

        Json Serialize();
        void Load(Json t_data);
    protected:
        virtual void AbstractLoad(Json t_data) = 0;
        virtual Json AbstractSerialize() = 0;

        virtual void AbstractRenderDetails() {};
    };

    using AbstractEasing = std::shared_ptr<EasingBase>;
    using EasingSpawnProcedure = std::function<AbstractEasing()>;
}