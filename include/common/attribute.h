#pragma once

#include "raster.h"
#include "typedefs.h"

namespace Raster {

    struct Composition;

    struct AttributeKeyframe {
        float timestamp;
        std::any value;
    };

    struct AttributeBase {
        int id;
        std::string name, packageName;
        std::vector<AttributeKeyframe> keyframes;

        AttributeBase();

        virtual std::any Get(float t_frame, Composition* composition) = 0;
        virtual void RenderKeyframes() = 0;
        virtual void RenderLegend(Composition* t_composition) = 0;
        virtual void Load(Json t_data) = 0;

        Json Serialize();

        protected:

        void RenderKeyframe(AttributeKeyframe keyframe);

        virtual Json AbstractSerialize() = 0;

        void Initialize();
    };

    using AbstractAttribute = std::shared_ptr<AttributeBase>;
    using AttributeSpawnProcedure = std::function<AbstractAttribute()>;
};