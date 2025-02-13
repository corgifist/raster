#pragma once

#include "raster.h"

namespace Raster {
    enum class MaskOperation {
        Normal = 0, Add = 1, Subtract = 2, Multiply = 3, Divide = 4
    };

    struct CompositionMask {
        int compositionID;
        MaskOperation op;
        bool precompose;

        CompositionMask() : compositionID(-1), op(MaskOperation::Normal), precompose(false) {}
        CompositionMask(int t_compositionID, MaskOperation t_op) : compositionID(t_compositionID), op(t_op), precompose(false) {}
        CompositionMask(int t_compositionID, MaskOperation t_op, bool t_precompose) : compositionID(t_compositionID), op(t_op), precompose(t_precompose) {}

        CompositionMask(Json t_data);

        Json Serialize();
    };
};