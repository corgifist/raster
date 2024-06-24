#pragma once

#include "raster.h"
#include "typedefs.h"

namespace Raster {

    struct Composition;

    struct AttributeKeyframe {
        int id;
        float timestamp;
        std::any value;

        AttributeKeyframe(float t_timestamp, std::any t_value);
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

        void SortKeyframes();

        Json Serialize();

        protected:

        void RenderKeyframe(AttributeKeyframe t_keyframe);

        bool KeyframeExists(float t_timestamp);
        std::optional<AttributeKeyframe*> GetKeyframeByTimestamp(float t_timestamp);
        std::optional<int> GetKeyframeIndexByTimestamp(float t_timestamp);

        virtual Json AbstractSerialize() = 0;

        void Initialize();

        protected:
        Composition* composition;
    };

    using AbstractAttribute = std::shared_ptr<AttributeBase>;
    using AttributeSpawnProcedure = std::function<AbstractAttribute()>;
};