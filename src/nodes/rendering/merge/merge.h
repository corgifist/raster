#pragma once
#include "raster.h"
#include "common/common.h"
#include "compositor/compositor.h"
#include "compositor/texture_interoperability.h"
#include "compositor/double_buffered_framebuffer.h"
#include "font/font.h"
#include "../../../ImGui/imgui.h"
#include "../../../ImGui/imgui_stdlib.h"

namespace Raster {
    struct Merge : public NodeBase {
    public:
        Merge();
        ~Merge();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        static void DispatchStringAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    
    private:
        DoubleBufferedFramebuffer m_framebuffer;
    };
};