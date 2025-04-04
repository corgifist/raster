#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "raster.h"

namespace Raster {
    struct SamplerConstantsBase : public NodeBase {
    public:
        SamplerConstantsBase(std::any t_constant);
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        std::any m_constant;
    };
};