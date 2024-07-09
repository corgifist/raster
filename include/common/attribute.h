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
        AttributeKeyframe(int t_id, float t_timestamp, std::any t_value);
    };

    struct AttributeDragDropPayload {
        int attributeID;
    };

    struct AttributeBase {
        int id;
        std::string name, packageName;
        std::vector<AttributeKeyframe> keyframes;

        AttributeBase();

        std::any Get(float t_frame, Composition* composition);
        virtual void RenderKeyframes() = 0;
        void RenderLegend(Composition* t_composition);
        virtual void Load(Json t_data) = 0;

        virtual void AbstractRenderDetails() {};

        void RenderPopup();

        void SortKeyframes();

        Json Serialize();

        static void ProcessKeyframeShortcuts();

        protected:

        virtual void AbstractRenderPopup() {}

        virtual std::any AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) = 0;
        virtual std::any AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) = 0;

        void RenderKeyframe(AttributeKeyframe t_keyframe);

        bool KeyframeExists(float t_timestamp);
        std::optional<AttributeKeyframe*> GetKeyframeByTimestamp(float t_timestamp);
        std::optional<int> GetKeyframeIndexByTimestamp(float t_timestamp);

        virtual Json AbstractSerialize() = 0;

        void Initialize();

        Composition* composition;

        private:
        static std::vector<int> m_deletedKeyframes;
    };

    using AbstractAttribute = std::shared_ptr<AttributeBase>;
    using AttributeSpawnProcedure = std::function<AbstractAttribute()>;
};