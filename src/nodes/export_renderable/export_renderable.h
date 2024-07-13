#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"

namespace Raster {
    struct ExportRenderable : public NodeBase {
        std::optional<std::type_index> lastExportedType;

        ExportRenderable();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};