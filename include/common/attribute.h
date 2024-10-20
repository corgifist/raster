#pragma once

#include "raster.h"
#include "typedefs.h"
#include "easing_base.h"

namespace Raster {

    struct Composition;

    struct AttributeKeyframe {
        int id;
        float timestamp;
        std::any value;
        std::optional<AbstractEasing> easing;

        AttributeKeyframe(float t_timestamp, std::any t_value);
        AttributeKeyframe(int t_id, float t_timestamp, std::any t_value);

        Json Serialize(Json t_customData);
    };

    struct AttributeDragDropPayload {
        int attributeID;
    };

    struct AttributeDescription {
        std::string packageName;
        std::string prettyName;
    };

    struct AttributeBase {
        int id;
        std::string name, packageName;
        std::string internalAttributeName;
        std::vector<AttributeKeyframe> keyframes;

        AttributeBase() {}

        std::any Get(float t_frame, Composition* composition);
        virtual void RenderKeyframes() {};
        void RenderLegend(Composition* t_composition);
        void RenderAttributePopup(Composition* t_composition);
        virtual void Load(Json t_data) {};
        
        virtual Json SerializeKeyframeValue(std::any t_value) { return {}; };
        virtual std::any LoadKeyframeValue(Json t_value) { return std::nullopt; };

        virtual void AbstractRenderDetails() {};

        void RenderPopup();

        void SortKeyframes();

        Json Serialize();

        static void ProcessKeyframeShortcuts();

        bool KeyframeExists(float t_timestamp);
        std::optional<AttributeKeyframe*> GetKeyframeByTimestamp(float t_timestamp);
        std::optional<int> GetKeyframeIndexByTimestamp(float t_timestamp);
        std::optional<int> GetKeyframeIndexByID(int t_id);

        protected:

        virtual void AbstractRenderPopup() {}

        virtual std::any AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) { return std::nullopt; };
        virtual std::any AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) { return std::nullopt; };

        void RenderKeyframe(AttributeKeyframe& t_keyframe);

        virtual Json AbstractSerialize() { return {}; };

        void Initialize();

        Composition* composition;

        private:

        void RenderKeyframePopup(AttributeKeyframe& t_keyframe);

        static std::vector<int> m_deletedKeyframes;
    };

    using AbstractAttribute = std::shared_ptr<AttributeBase>;
    using AttributeSpawnProcedure = std::function<AbstractAttribute()>;
};