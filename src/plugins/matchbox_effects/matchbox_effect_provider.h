#pragma once

#include "xml.hpp"


#include <typeindex>
#include <unordered_map>
#include "common/synchronized_value.h"
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

    struct MatchboxEffectMetadata {
        std::string shaderType; // "Matchbox" or "Lightbox"
        std::string version;
        std::string description;
        std::string name;
        bool commercialUsePermitted;

        MatchboxEffectMetadata() {}
    };

    struct MatchboxEffectUniform {
        std::string uniformName;
        std::string attributeName;
        std::string stringType;
        std::type_index type;
        int textureUnitIndex;
        std::string noInput;

        MatchboxEffectUniform() : type(typeid(void)), textureUnitIndex(0) {}
    };

    struct MatchboxEffectAttribute {
        std::string attributeName, uniformName;
        float min, max, inc;
        bool isSampler;
        bool duplicate;
        std::string inputType;
        std::string uiConditionSource;
        std::string uiConditionValue;
        std::string uiConditionType;

        std::any defaultValue;
    };

    struct MatchboxEffectPass {
        int shaderIndex;
        int outputWidth, outputHeight;
        ManagedFramebuffer framebuffer;
        std::string outputBitDepth;
        std::vector<MatchboxEffectUniform> uniforms;  
    };

    struct MatchboxEffectData {
        MatchboxEffectMetadata metadata;
        std::vector<MatchboxEffectAttribute> attributes;
        std::vector<MatchboxEffectPass> passes;
        std::string frontAttributeName, backAttributeName;

        MatchboxEffectData() {}
    };

    struct MatchboxEffectProvider : public NodeBase {
        MatchboxEffectProvider(std::string t_xmlPath);
        ~MatchboxEffectProvider();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        Pipeline GetPipelineForIndex(int t_index);

        static MatchboxEffectMetadata ParseMetadata(xml_document& t_document);
        static MatchboxEffectData ParseEffectData(xml_document& t_document);
        static std::vector<MatchboxEffectAttribute> ParseAttributes(xml_document& t_document);
        static std::vector<MatchboxEffectPass> ParsePasses(xml_document& t_document);
        static std::vector<MatchboxEffectUniform> ParseUniforms(xml_node& t_node);

    private:
        static std::any MakeDynamicValue(xml_node& t_node);
        static Pipeline GetCachedPipeline(std::string t_fragmentPath);
        static Texture GetTextureGrid(std::string t_baseName);

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

        std::unordered_map<int, Pipeline> m_cachedPipelines;

        std::optional<std::vector<Pipeline>> m_pipelines;
        std::optional<std::vector<ManagedFramebuffer>> m_framebuffers;
        std::unique_ptr<xml_document> m_document;
        std::unordered_map<std::string, std::any> m_cachedValues;

        SynchronizedValue<std::unordered_map<std::string, std::string>> m_uiStringCache;

        MatchboxEffectData m_data;
        std::string m_baseName;
        static std::unordered_map<std::string, Pipeline> s_pipelineCache;
        static std::unordered_map<std::string, Texture> s_textureGridsCache;
    };
};