#pragma once

#include "xml.hpp"

#include "compositor/managed_framebuffer.h"
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "compositor/texture_interoperability.h"
#include "common/asset_id.h"
#include "common/gradient_1d.h"
#include "common/generic_resolution.h"
#include "font/font.h"

namespace Raster {

    using namespace pugi;

    struct ShaderPair {
        std::string vertexPath, fragmentPath;
        Pipeline pipeline;

        ShaderPair() {}
        ShaderPair(std::string t_vertexPath, std::string t_fragmentPath, Pipeline t_pipeline) : 
                    vertexPath(t_vertexPath), fragmentPath(t_fragmentPath), pipeline(t_pipeline) {}
    };

    struct XMLEffectProvider : public NodeBase {
        XMLEffectProvider(std::string t_xmlPath);
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        std::any MakeDynamicValue(std::string t_type, std::string t_value = "");
        
        std::vector<Pipeline> m_pipelines;
        std::vector<ManagedFramebuffer> m_framebuffers;
        std::unique_ptr<xml_document> m_document;

        std::string m_cachedName, m_cachedIcon;
        static std::unordered_map<std::string, Pipeline> s_pipelineCache;
    };
};