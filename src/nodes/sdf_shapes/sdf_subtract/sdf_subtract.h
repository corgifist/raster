#pragma once
#include "raster.h"
#include "common/common.h"
#include "../../rendering/layer2d/layer2d.h"

namespace Raster {
    struct SDFSubtract : public NodeBase {
        SDFSubtract();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

    private:
        void TransformShapeUniforms(SDFShape& t_shape, std::string t_uniqueID);
        void TransformShape(SDFShape& t_shape, std::string t_uniqueID);

        std::optional<SDFShape> GetShape(std::string t_attribute, ContextData& t_contextData);

        SDFShape m_mixedShape;
        int m_firstShapeID, m_secondShapeID;
    };
};