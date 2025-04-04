#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "compositor/texture_interoperability.h"

namespace Raster {
    struct ExportRenderable : public NodeBase {
        std::optional<std::type_index> lastExportedType;

        ExportRenderable();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();
        bool AbstractDoesRendering();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};