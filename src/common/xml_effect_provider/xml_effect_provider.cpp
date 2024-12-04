#include "xml_effect_provider.h"
#include "common/attribute_metadata.h"
#include "common/generic_resolution.h"
#include "common/gradient_1d.h"
#include "common/sampler_settings.h"
#include "common/transform2d.h"
#include "compositor/managed_framebuffer.h"
#include "compositor/texture_interoperability.h"
#include "font/IconsFontAwesome5.h"
#include "gpu/gpu.h"
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace Raster {

    static std::vector<std::string> SplitString(std::string s, std::string delimiter) {
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
            token = s.substr (pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back (token);
        }

        res.push_back (s.substr (pos_start));
        return res;
    }

    std::unordered_map<std::string, Pipeline> XMLEffectProvider::s_pipelineCache;


    XMLEffectProvider::XMLEffectProvider(std::string t_xmlPath) {
        NodeBase::Initialize();
        this->m_document = std::make_unique<xml_document>();
        auto parseResult = m_document->load_file(t_xmlPath.c_str());
        if (!parseResult) {
            throw std::runtime_error("could not load xml effect " + t_xmlPath);
        }

        this->m_cachedName = m_document->select_node("/effect/description").node().attribute("name").as_string();
        this->m_cachedIcon = Font::GetIcon(m_document->select_node("/effect/icon").node().attribute("icon").as_string());

        for (auto node : m_document->select_nodes("/effect/attributes/attribute")) {
            auto attribute = node.node();
            SetupAttribute(attribute.attribute("name").as_string(), 
                            MakeDynamicValue(attribute.attribute("type").as_string(), attribute.attribute("value").as_string("")));
        }
        
        for (auto node : m_document->select_nodes("/effect/pins/input")) {
            auto pin = node.node();
            AddInputPin(pin.attribute("name").as_string());
        }

        for (auto node : m_document->select_nodes("/effect/pins/output")) {
            auto pin = node.node();
            AddOutputPin(pin.attribute("name").as_string());
        }

        auto framebuffersNode = m_document->select_node("/effect/framebuffers");
        auto framebuffersAttribute = framebuffersNode.node();
        int framebuffersCount = framebuffersAttribute.attribute("count").as_int();
        m_framebuffers = std::vector<ManagedFramebuffer>(framebuffersCount);
    }

    AbstractPinMap XMLEffectProvider::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        if (m_pipelines.empty()) {
            for (auto node : m_document->select_nodes("/effect/shaders/shader")) {
                auto shader = node.node();
                std::string vertexPath = shader.attribute("vertex").as_string();
                std::string fragmentPath = shader.attribute("fragment").as_string();
                DUMP_VAR(fragmentPath);
                if (s_pipelineCache.find(vertexPath + fragmentPath) != s_pipelineCache.end()) {
                    m_pipelines.push_back(s_pipelineCache[vertexPath + fragmentPath]);
                } else {
                    Pipeline compiledShader = GPU::GeneratePipeline(
                        vertexPath == "basic" ? GPU::s_basicShader : GPU::GenerateShader(ShaderType::Vertex, vertexPath), 
                        GPU::GenerateShader(ShaderType::Fragment, fragmentPath));
                    s_pipelineCache[vertexPath + fragmentPath] = compiledShader;
                    m_pipelines.push_back(compiledShader);
                }
            }
        }

        std::vector<Framebuffer> swappedFramebuffers(m_framebuffers.size());

        auto renderingNode = m_document->select_node("/effect/rendering").node();
        int resultFramebuffer = renderingNode.attribute("result").as_int();

        std::unordered_map<std::string, std::any> attributesCache;

        for (auto pass : renderingNode.children("pass")) {
            int targetFramebuffer = pass.attribute("framebuffer").as_int();
            int targetShader = pass.attribute("shader").as_int();
            std::string baseFramebuffer = pass.attribute("base").as_string();
            std::string clearColorRawString = pass.attribute("clearColor").as_string();
            auto clearColorStr = SplitString(clearColorRawString, ";");
            glm::vec4 targetClearColor = glm::vec4(1);
            if (!clearColorRawString.empty()) {
                targetClearColor = glm::vec4(std::stof(clearColorStr[0]), std::stof(clearColorStr[1]), std::stof(clearColorStr[2]), std::stof(clearColorStr[3]));
            }
            
            std::optional<Framebuffer> baseFramebufferCandidate = std::nullopt;
            if (attributesCache.find(baseFramebuffer) != attributesCache.end()) {
                auto cacheCandidate = attributesCache[baseFramebuffer];
                if (cacheCandidate.type() == typeid(Framebuffer)) {
                    baseFramebufferCandidate = std::any_cast<Framebuffer>(cacheCandidate);
                }
            } else {
                baseFramebufferCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute(baseFramebuffer, t_contextData));
                if (baseFramebufferCandidate) attributesCache[baseFramebuffer] = *baseFramebufferCandidate;
            }
            auto& framebuffer = swappedFramebuffers.at(targetFramebuffer);
            framebuffer = m_framebuffers.at(targetFramebuffer).Get(baseFramebufferCandidate);

            GPU::BindPipeline(m_pipelines.at(targetShader));
            GPU::BindFramebuffer(framebuffer);
            if (!clearColorRawString.empty())
                GPU::ClearFramebuffer(targetClearColor.r, targetClearColor.g, targetClearColor.g, targetClearColor.a);

            for (auto& uniform : pass.children("uniform")) {
                std::string uniformName = uniform.attribute("name").as_string();
                std::string stage = uniform.attribute("stage").as_string();
                Shader& shaderStage = stage == "vertex" ? m_pipelines.at(targetShader).vertex : m_pipelines.at(targetShader).fragment;
                for (auto value : uniform.children("value")) {
                    std::string attributeName = value.attribute("attribute").as_string();
                    std::string type = value.attribute("type").as_string();

                    std::optional<std::any> attributeCandidate = std::nullopt;
                    if (attributesCache.find(attributeName) != attributesCache.end()) {
                        attributeCandidate = attributesCache[attributeName];
                    } else {
                        attributeCandidate = GetDynamicAttribute(attributeName, t_contextData);
                        if (attributeCandidate) attributesCache[attributeName] = *attributeCandidate;
                    }
                    if (attributeCandidate) {
                        auto& attributeValue = *attributeCandidate;

#define UNIFORM_CLAUSE(t_real_type, t_str_type) \
    if (attributeValue.type() == typeid(t_real_type) && t_str_type == type)  { \
        GPU::SetShaderUniform(shaderStage, uniformName, std::any_cast<t_real_type>(attributeValue)); \
    }
                    
                        UNIFORM_CLAUSE(float, "float");
                        UNIFORM_CLAUSE(bool, "bool");
                        UNIFORM_CLAUSE(int, "int");
                        UNIFORM_CLAUSE(glm::vec2, "glm::vec2");
                        UNIFORM_CLAUSE(glm::vec3, "glm::vec3");
                        UNIFORM_CLAUSE(glm::vec4, "glm::vec4");
                    }
                }

                for (auto resolution : uniform.children("resolution")) {
                    int resolutionFramebuffer = resolution.attribute("framebuffer").as_int();
                    auto& targetResolutionFramebuffer = swappedFramebuffers[resolutionFramebuffer];
                    GPU::SetShaderUniform(shaderStage, uniformName, glm::vec2(targetResolutionFramebuffer.width, targetResolutionFramebuffer.height));
                }

                for (auto screenSpaceRendering : uniform.children("screenSpaceRendering")) {
                    std::string framebufferAttributeName = screenSpaceRendering.attribute("attribute").as_string();
                    std::string overrideAttributeName = screenSpaceRendering.attribute("override").as_string();
                    std::optional<Framebuffer> framebufferCandidate = std::nullopt;
                    if (attributesCache.find(framebufferAttributeName) != attributesCache.end()) {
                        auto cacheCandidate = attributesCache[framebufferAttributeName];
                        if (cacheCandidate.type() == typeid(Framebuffer)) {
                            framebufferCandidate = std::any_cast<Framebuffer>(cacheCandidate);
                        }
                    } else {
                        framebufferCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute(framebufferAttributeName, t_contextData));
                        if (framebufferCandidate) attributesCache[framebufferAttributeName] = *framebufferCandidate;
                    }
                    if (!framebufferCandidate) continue;
                    auto& targetScreenSpaceFramebuffer = *framebufferCandidate;
                    bool useScreenSpaceRendering = !(targetScreenSpaceFramebuffer.attachments.size() >= 2);
                    if (attributesCache.find(overrideAttributeName) != attributesCache.end()) {
                        auto overrideCandidate = attributesCache[overrideAttributeName];
                        if (overrideCandidate.type() == typeid(bool)) {
                            if (std::any_cast<bool>(overrideCandidate)) {
                                useScreenSpaceRendering = true;
                            }
                        }
                    } else {
                        auto overrideCandidate = GetAttribute<bool>(overrideAttributeName, t_contextData);
                        if (overrideCandidate) {
                            if (*overrideCandidate) useScreenSpaceRendering = true;
                            attributesCache[overrideAttributeName] = *overrideCandidate;
                        }
                    }
                    GPU::SetShaderUniform(shaderStage, uniformName, useScreenSpaceRendering);
                }

                for (auto attachment : uniform.children("attachment")) {
                    int attachmentIndex = attachment.attribute("index").as_int();
                    int unitIndex = attachment.attribute("unit").as_int();
                    auto attributeName = attachment.attribute("attribute").as_string();
                    std::optional<Framebuffer> attributeCandidate = std::nullopt;
                    if (attributesCache.find(attributeName) != attributesCache.end()) {
                        auto cacheCandidate = attributesCache[attributeName];
                        if (cacheCandidate.type() == typeid(Framebuffer)) {
                            attributeCandidate = std::any_cast<Framebuffer>(cacheCandidate);
                        }
                    } else {
                        attributeCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute(attributeName, t_contextData));
                        if (attributeCandidate) attributesCache[attributeName] = *attributeCandidate;
                    }
                    if (attributeCandidate) {
                        auto& attributeValue = *attributeCandidate;
                        if (attachmentIndex >= 0 && attachmentIndex < attributeValue.attachments.size()) {
                            GPU::BindTextureToShader(shaderStage, uniformName, attributeValue.attachments.at(attachmentIndex), unitIndex);
                        }
                    }
                }
            }

            for (auto& draw : pass.children("draw")) {
                auto count = draw.attribute("count").as_int();
                GPU::DrawArrays(count);
            }
        }

        TryAppendAbstractPinMap(result, renderingNode.attribute("pin").as_string(), swappedFramebuffers.at(resultFramebuffer));

        return result;
    }

    std::any XMLEffectProvider::MakeDynamicValue(std::string t_type, std::string t_value) {
        if (t_type == "float") {
            if (t_value.empty()) return 0.0f;
            return std::stof(t_value.c_str());
        }
        if (t_type == "int") {
            if (t_value.empty()) return 0;
            return std::stoi(t_value.c_str());
        }
        if (t_type == "std::string") {
            return t_value;
        }
        if (t_type == "glm::vec2") {
            auto parts = SplitString(t_value, ";");
            return glm::vec2(std::stof(parts[0].c_str()), std::stof(parts[1].c_str()));
        }
        if (t_type == "glm::vec3") {
            auto parts = SplitString(t_value, ";");
            return glm::vec3(std::stof(parts[0].c_str()), std::stof(parts[1].c_str()), std::stof(parts[2].c_str()));
        }
        if (t_type == "glm::vec4") {
            auto parts = SplitString(t_value, ";");
            return glm::vec4(std::stof(parts[0].c_str()), std::stof(parts[1].c_str()), std::stof(parts[2].c_str()), std::stof(parts[3].c_str()));
        }
        if (t_type == "Transform2D") {
            return Transform2D();
        }
        if (t_type == "Framebuffer") {
            return Framebuffer();
        }
        if (t_type == "SamplerSettings") {
            return SamplerSettings();
        }
        if (t_type == "bool") {
            return t_value == "true";
        }
        if (t_type == "AssetID") {
            return AssetID();
        }
        if (t_type == "GenericResolution") {
            return GenericResolution();
        }
        if (t_type == "Gradient1D") {
            return Gradient1D();
        }
        throw std::runtime_error("could not make dynamic value from " + t_type + " " + t_value);
    }

    void XMLEffectProvider::AbstractRenderProperties() {
        for (auto node : m_document->select_nodes("/effect/properties/property")) {
            auto property = node.node();
            auto propertyName = property.attribute("name").as_string();
            std::vector<std::any> metadata;

            for (auto formatStringAttribute : property.children("formatString")) {
                metadata.push_back(FormatStringMetadata(formatStringAttribute.attribute("format").as_string()));
            }

            for (auto sliderRangeAttribute : property.children("sliderRange")) {
                metadata.push_back(SliderRangeMetadata(
                    sliderRangeAttribute.attribute("min").as_float(),
                    sliderRangeAttribute.attribute("max").as_float()
                ));
            }

            for (auto sliderStepAttribute : property.children("sliderStep")) {
                metadata.push_back(SliderStepMetadata(
                    sliderStepAttribute.attribute("step").as_float()
                ));
            }

            for (auto sliderBaseAttribute : property.children("sliderBase")) {
                metadata.push_back(SliderBaseMetadata(
                    sliderBaseAttribute.attribute("base").as_float()
                ));
            }

            for (auto iconAttribute : property.children("icon")) {
                metadata.push_back(IconMetadata(
                    Font::GetIcon(iconAttribute.attribute("icon").as_string())
                ));
            }

            RenderAttributeProperty(propertyName, metadata);
        }
    }

    void XMLEffectProvider::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json XMLEffectProvider::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool XMLEffectProvider::AbstractDetailsAvailable() {
        return false;
    }

    std::string XMLEffectProvider::AbstractHeader() {
        return m_cachedName;
    }

    std::string XMLEffectProvider::Icon() {
        return m_cachedIcon;
    }

    std::optional<std::string> XMLEffectProvider::Footer() {
        return std::nullopt;
    }
}