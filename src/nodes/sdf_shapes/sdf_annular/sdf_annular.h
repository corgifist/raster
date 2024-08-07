#pragma once
#include "raster.h"
#include "common/common.h"
#include "../../rendering/layer2d/layer2d.h"

namespace Raster {
    struct SDFAnnular : public NodeBase {
        SDFAnnular();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        void TransformShapeUniforms(SDFShape& t_shape, std::string t_uniqueID);
        void TransformShape(SDFShape& t_shape, std::string t_uniqueID);

        std::optional<SDFShape> GetShape(std::string t_attribute);

        SDFShape m_mixedShape;
        int m_firstShapeID;
    };
};