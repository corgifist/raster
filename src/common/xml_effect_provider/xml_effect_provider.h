#pragma once

#include "xml.hpp"

#include "common/typedefs.h"
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

    struct AttributesCache {
    
    private:

    };

    struct XMLEffectProvider : public NodeBase {
        XMLEffectProvider(std::string t_xmlPath);
        ~XMLEffectProvider();
        
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
        
        template<typename T>
        std::optional<T> GetCachedAttribute(std::string t_attribute, ContextData& t_contextData) {
            if (m_cachedValues.find(t_attribute) != m_cachedValues.end()) {
                auto& candidate = m_cachedValues[t_attribute];
                if (candidate.type() == typeid(T)) {
                    return std::any_cast<T>(candidate);
                } 
                return std::nullopt;
            }
            auto valueCandidate = GetAttribute<T>(t_attribute, t_contextData);
            if (valueCandidate) {
                m_cachedValues[t_attribute] = *valueCandidate;
                return *valueCandidate;
            }
            return std::nullopt;
        };

        std::optional<std::any> GetDynamicCachedAttribute(std::string t_attribute, ContextData& t_contextData) {
            if (m_cachedValues.find(t_attribute) != m_cachedValues.end()) {
                return m_cachedValues[t_attribute];
            }
            auto valueCandidate = GetDynamicAttribute(t_attribute, t_contextData);
            if (valueCandidate) {
                m_cachedValues[t_attribute] = *valueCandidate;
                return *valueCandidate;
            }
            return std::nullopt;
        };

        std::vector<Pipeline> m_pipelines;
        std::vector<ManagedFramebuffer> m_framebuffers;
        std::vector<std::optional<ArrayBuffer>> m_gradientBuffers;
        std::unique_ptr<xml_document> m_document;
        std::unordered_map<std::string, std::any> m_cachedValues;

        std::string m_cachedName, m_cachedIcon;
        static std::unordered_map<std::string, Pipeline> s_pipelineCache;
    };
};