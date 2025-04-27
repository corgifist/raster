#include "matchbox_effect_provider.h"
#include "common/attribute_metadata.h"
#include "common/generic_resolution.h"
#include "common/gradient_1d.h"
#include "common/project.h"
#include "common/randomizer.h"
#include "common/sampler_settings.h"
#include "common/transform2d.h"
#include "compositor/managed_framebuffer.h"
#include "compositor/texture_interoperability.h"
#include "font/IconsFontAwesome5.h"
#include "gpu/gpu.h"
#include "common/choice.h"
#include "common/dispatchers.h"
#include "matchbox_effects.h"
#include "raster.h"
#include "../../ImGui/imgui.h"
#include "image/image.h"
#include <filesystem>

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

    std::unordered_map<std::string, Pipeline> MatchboxEffectProvider::s_pipelineCache;
    std::unordered_map<std::string, Texture> MatchboxEffectProvider::s_textureGridsCache;

    MatchboxEffectProvider::MatchboxEffectProvider(std::string t_xmlPath) {
        NodeBase::Initialize();
        this->m_document = std::make_unique<xml_document>();
        auto parseResult = m_document->load_file(t_xmlPath.c_str());
        if (!parseResult) {
            throw std::runtime_error("could not load matchbox effect " + t_xmlPath);
        }

        this->m_data = ParseEffectData(*m_document);
        this->m_baseName = GetBaseName(RemoveExtension(t_xmlPath));

        for (auto& attribute : m_data.attributes) {
            if (attribute.duplicate) continue;
            SetupAttribute(attribute.uniformName, attribute.defaultValue);
            SetAttributeAlias(attribute.uniformName, attribute.attributeName);
            if (attribute.isSampler) {
                AddInputPin(attribute.uniformName);
            }
        }
        AddOutputPin("Output");
    }

    MatchboxEffectProvider::~MatchboxEffectProvider() {
        // empty for now
    }

    static std::string ADSKTypeToString(std::type_index type) {
        if (type == typeid(int)) {
            return "int";
        } else if (type == typeid(float)) {
            return "float";
        } else if (type == typeid(bool)) {
            return "bool";
        } else if (type == typeid(glm::vec2)) {
            return "vec2";
        } else if (type == typeid(glm::vec3)) {
            return "vec3";
        } else if (type == typeid(glm::vec4)) {
            return "vec4";
        } else if (type == typeid(Texture)) {
            return "sampler2D";
        }
        return "void";
    }

    static std::type_index StringToADSKType(std::string type) {
        if (type == "int") {
            return typeid(int);
        } else if (type == "float") {
            return typeid(float);
        } else if (type == "bool") {
            return typeid(bool);
        } else if (type == "vec2") {
            return typeid(glm::vec2);
        } else if (type == "vec3") {
            return typeid(glm::vec3);
        } else if (type == "vec4") {
            return typeid(glm::vec4);
        } else if (type == "sampler2D") {
            return typeid(Texture);
        }
        return typeid(void);
    }

    AbstractPinMap MatchboxEffectProvider::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        m_cachedValues.clear();

        auto& project = Workspace::GetProject();
        auto compositionCandidate = Workspace::GetCompositionByNodeID(nodeID);
        if (!compositionCandidate) return result;
        auto& composition = *compositionCandidate;

        static Texture s_whiteTexture;
        if (!s_whiteTexture.handle) {
            s_whiteTexture = GPU::GenerateTexture(1, 1, 4);
            static uint8_t whitePixels[] = {255, 255, 255, 255};
            GPU::UpdateTexture(s_whiteTexture, 0, 0, 1, 1, 4, whitePixels);
        }

        static Texture s_blackTexture;
        if (!s_blackTexture.handle) {
            s_blackTexture = GPU::GenerateTexture(1, 1, 4);
            static uint8_t blackPixels[] = {0, 0, 0, 255};
            GPU::UpdateTexture(s_blackTexture, 0, 0, 1, 1, 4, blackPixels);
        }

        std::vector<Framebuffer> passBuffers;
        for (auto& pass : m_data.passes) {
            Pipeline pipeline = GetPipelineForIndex(pass.shaderIndex);
            if (!pipeline.handle) continue;
            auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicCachedAttribute(m_data.frontAttributeName, t_contextData));
            if (!baseCandidate) {
                baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicCachedAttribute(m_data.backAttributeName, t_contextData));
            }
            auto referenceFramebuffer = baseCandidate;
            if (pass.outputWidth > 0 && pass.outputHeight > 0) {
                referenceFramebuffer = baseCandidate ? *baseCandidate : Framebuffer();
                if (referenceFramebuffer->width != pass.outputWidth || referenceFramebuffer->height != pass.outputHeight) {
                    referenceFramebuffer->handle = nullptr;
                }
                referenceFramebuffer->width = pass.outputWidth;
                referenceFramebuffer->height = pass.outputHeight;
            }
            if (pass.outputBitDepth != "Output") {
                if (referenceFramebuffer && referenceFramebuffer->attachments.empty()) {
                    referenceFramebuffer->attachments.push_back(Texture());
                }
                if (referenceFramebuffer && pass.outputBitDepth == "Float32") {
                    referenceFramebuffer->attachments[0].precision = TexturePrecision::Full;
                }
                if (referenceFramebuffer && pass.outputBitDepth == "Float16") {
                    referenceFramebuffer->attachments[0].precision = TexturePrecision::Half;
                }
            } else {
                if (referenceFramebuffer && referenceFramebuffer->attachments.empty()) {
                    referenceFramebuffer->attachments.push_back(Texture());
                }
                if (referenceFramebuffer && !referenceFramebuffer->attachments.empty()) {
                    referenceFramebuffer->attachments[0].precision = Compositor::s_colorPrecision;
                }
            }
            auto framebuffer = pass.framebuffer.GetWithoutBlitting(referenceFramebuffer);
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);
            GPU::ClearFramebuffer(0.0f, 0.0f, 0.0f, 0.0f);
            int baseTextureUnit = 0;
            for (auto& buffer : passBuffers) {
                GPU::BindTextureToShader(pipeline.fragment, "adsk_results_pass" + std::to_string(baseTextureUnit + 1), passBuffers[baseTextureUnit].attachments[0], baseTextureUnit);
                // RASTER_LOG("binding texture adsk_results_pass" << baseTextureUnit + 1);
                baseTextureUnit++;
            }
            for (auto& uniform : pass.uniforms) {
                GPU::BindFramebuffer(framebuffer);
                GPU::BindPipeline(pipeline);
                if (uniform.uniformName == "adsk_texture_grid") {
                    auto textureGrid = GetTextureGrid(m_baseName);
                    GPU::BindTextureToShader(pipeline.fragment, "adsk_texture_grid", textureGrid, uniform.textureUnitIndex);
                    continue;
                }
                if (uniform.attributeName.find("adsk_results_pass") != std::string::npos) continue;
                auto valueCandidate = GetDynamicCachedAttribute(uniform.uniformName, t_contextData);
                GPU::BindFramebuffer(framebuffer);
                GPU::BindPipeline(pipeline);
                if (valueCandidate && std::type_index(valueCandidate->type()) != uniform.type) {
                    for (auto& conversionDispatcher : Dispatchers::s_conversionDispatchers) {
                        if (conversionDispatcher.from == std::type_index(valueCandidate->type()) && uniform.type == conversionDispatcher.to) {
                            auto conversionCandidate = conversionDispatcher.function(*valueCandidate);
                            if (conversionCandidate) {
                                valueCandidate = *conversionCandidate;
                                break;
                            }
                        }
                    }
                }
                bool isTextureType = false;
                if (valueCandidate && uniform.stringType == "sampler2D" && valueCandidate->type() == typeid(Texture)) isTextureType = true;
                if (valueCandidate && uniform.stringType == "sampler2D" && valueCandidate->type() == typeid(Framebuffer)) isTextureType = true;
                if (valueCandidate && (std::type_index(valueCandidate->type()) == uniform.type || isTextureType)) {
                    auto& value = *valueCandidate;
                    std::string uiStringTarget = "";
                    if (value.type() == typeid(bool)) {
                        uiStringTarget = std::any_cast<bool>(value) ? "True" : "False";
                    }
                    if (value.type() == typeid(int)) {
                        uiStringTarget = std::to_string(std::any_cast<int>(value));
                    }
                    if (value.type() == typeid(float)) {
                        uiStringTarget = std::to_string(std::any_cast<float>(value));
                    }
                    if (!uiStringTarget.empty()) {
                        m_uiStringCache.Lock();
                        auto& uiStringCache = m_uiStringCache.GetReference();
                        uiStringCache[uniform.uniformName] = uiStringTarget;
                        m_uiStringCache.Unlock(); 
                    }
                    // RASTER_LOG(uniform.stringType << " " << uniform.uniformName);
#define UNIFORM_CLAUSE(TYPE) \
    if (value.type() == typeid(TYPE)) { /* RASTER_LOG(RASTER_STRINGIFY(TYPE) << " " << uniform.uniformName << " setting uniform"); */ GPU::SetShaderUniform(pipeline.fragment, uniform.uniformName, std::any_cast<TYPE>(value)); }
                    UNIFORM_CLAUSE(int);
                    UNIFORM_CLAUSE(float);
                    UNIFORM_CLAUSE(bool);
                    UNIFORM_CLAUSE(glm::vec2);
                    UNIFORM_CLAUSE(glm::vec3);
                    UNIFORM_CLAUSE(glm::vec4);
                    // DUMP_VAR(value.type().name());
                    auto textureCandidate = TextureInteroperability::GetTexture(value);
                    if (textureCandidate) {
                        std::optional<Texture> boundTexture = std::nullopt;
                        // RASTER_LOG("binding texture " << uniform.uniformName);
                        if (textureCandidate) {
                            auto& texture = *textureCandidate;
                            // DUMP_VAR(texture.width);
                            GPU::BindTextureToShader(pipeline.fragment, uniform.uniformName, texture, baseTextureUnit + uniform.textureUnitIndex);
                            // RASTER_LOG("binding " << uniform.uniformName << " to unit " << baseTextureUnit + uniform.textureUnitIndex);
                            boundTexture = texture;
                        } else {
                            if (uniform.noInput == "White") {
                                GPU::BindTextureToShader(pipeline.fragment, uniform.uniformName, s_whiteTexture, baseTextureUnit + uniform.textureUnitIndex); 
                                boundTexture = s_whiteTexture;
                            }
                            if (uniform.noInput == "Black") {
                                GPU::BindTextureToShader(pipeline.fragment, uniform.uniformName, s_blackTexture, baseTextureUnit + uniform.textureUnitIndex);
                                boundTexture = s_blackTexture;
                            }
                        }

                        if (boundTexture) {
                            // DUMP_VAR((uint64_t) boundTexture->handle);
                            // DUMP_VAR(boundTexture->width);
                            GPU::SetShaderUniform(pipeline.fragment, FormatString("adsk_%s_w", uniform.uniformName.c_str()), (float) boundTexture->width);
                            GPU::SetShaderUniform(pipeline.fragment, FormatString("adsk_%s_h", uniform.uniformName.c_str()), (float) boundTexture->height);
                            GPU::SetShaderUniform(pipeline.fragment, FormatString("adsk_%s_frameratio", uniform.uniformName.c_str()), (float) boundTexture->width / (float) boundTexture->height);
                            GPU::SetShaderUniform(pipeline.fragment, FormatString("adsk_%s_pixelratio", uniform.uniformName.c_str()), 1.0f);
                        }
                    }
                }
            }
            GPU::SetShaderUniform(pipeline.fragment, "adsk_result_w", (float) framebuffer.width);
            // RASTER_LOG("adsk_result_w");
            GPU::SetShaderUniform(pipeline.fragment, "adsk_result_h", (float) framebuffer.height);
            // RASTER_LOG("adsK_result_h");
            GPU::SetShaderUniform(pipeline.fragment, "adsk_result_frameratio", (float) framebuffer.width / (float) framebuffer.height);
            // RASTER_LOG("adsk_result_frameratio");
            GPU::SetShaderUniform(pipeline.fragment, "adsk_result_pixelratio", 1.0f);
            // RASTER_LOG("adsk_result_pixelratio");
            
            GPU::SetShaderUniform(pipeline.fragment, "adsk_time", glm::max(1.0f, project.GetCorrectCurrentTime()));
            // RASTER_LOG("adsk_time");
            GPU::SetShaderUniform(pipeline.fragment, "adsk_degrade", false);
            // RASTER_LOG("adsk_degrade");
            GPU::DrawArrays(3);
            passBuffers.push_back(framebuffer); 
        }

        if (passBuffers.size() > 0) TryAppendAbstractPinMap(result, "Output", passBuffers[passBuffers.size() - 1]);

        return result;
    }

    Pipeline MatchboxEffectProvider::GetPipelineForIndex(int t_index) {
        if (m_cachedPipelines.find(t_index) != m_cachedPipelines.end()) {
            return m_cachedPipelines[t_index];
        }
        auto shadersPath = GPU::GetShadersPath();
        static std::vector<std::string> s_probeFormats = {
            "matchbox/%s.glsl",
            "matchbox/%s.%i.glsl",
            "matchbox/%s.%02i.glsl"
        };
        for (auto& format : s_probeFormats) {
            auto formattedPath = shadersPath + FormatString(format, m_baseName.c_str(), t_index);
            // DUMP_VAR(formattedPath);
            if (std::filesystem::exists(formattedPath)) {
                // RASTER_LOG(formattedPath << " found");
                auto shaderContent = ReadFile(formattedPath);
                static std::optional<std::string> s_matchboxPreset;
                if (!s_matchboxPreset) {
                    s_matchboxPreset = ReadFile(shadersPath + "matchbox/matchbox_shader_base.glsl");
                }
                std::string generatedCode = *s_matchboxPreset;
                generatedCode = ReplaceString(generatedCode, "MATCHBOX_CODE_GOES_HERE", shaderContent);
                // DUMP_VAR(generatedCode);
                int id = Randomizer::GetRandomInteger();
                std::string newShaderPath = shadersPath + "matchbox/.matchbox" + std::to_string(id) + ".frag";
                std::string clearPath = "matchbox/.matchbox" + std::to_string(id);
                WriteFile(newShaderPath, generatedCode);
                try {
                    auto pipeline = GetCachedPipeline(clearPath);
                    std::filesystem::remove(newShaderPath);
                    m_cachedPipelines[t_index] = pipeline;
                    return pipeline;
                } catch (std::runtime_error e) {
                    RASTER_LOG("failed to generate shader " << formattedPath);
                    RASTER_LOG("\t" << e.what());
                    throw std::runtime_error("terminating");
                }
            }
        }
        return Pipeline();
    }

    MatchboxEffectData MatchboxEffectProvider::ParseEffectData(xml_document &t_document) {
        MatchboxEffectData result;
        result.metadata = ParseMetadata(t_document);
        result.attributes = ParseAttributes(t_document);
        result.passes = ParsePasses(t_document);
        for (auto& attribute : result.attributes) {
            if (attribute.inputType == "Front") {
                result.frontAttributeName = attribute.uniformName;
            }
            if (attribute.inputType == "Back") {
                result.backAttributeName = attribute.uniformName;
            }
        }
        return result;
    }

    std::vector<MatchboxEffectPass> MatchboxEffectProvider::ParsePasses(xml_document &t_document) {
        std::vector<MatchboxEffectPass> result;
        for (auto& pass : t_document.child("ShaderNodePreset").children("Shader")) {
            MatchboxEffectPass p;
            p.shaderIndex = pass.attribute("Index").as_int();
            p.uniforms = ParseUniforms(pass);
            p.outputWidth = pass.attribute("OutputWidth").as_int(0);
            p.outputHeight = pass.attribute("OutputHeight").as_int(0);
            p.outputBitDepth = pass.attribute("OutputBitDepth").as_string("Output");
            result.push_back(p);
        }
        return result;
    }

    std::vector<MatchboxEffectUniform> MatchboxEffectProvider::ParseUniforms(xml_node &t_node) {
        std::vector<MatchboxEffectUniform> result;
        for (auto& uniform : t_node.children("Uniform")) {
            std::string uniformName = uniform.attribute("Name").as_string();
            std::string attributeName = uniform.attribute("DisplayName").as_string();
            std::string typeString = uniform.attribute("Type").as_string();
            
            MatchboxEffectUniform u;
            u.uniformName = uniformName;
            u.attributeName = attributeName;
            u.type = StringToADSKType(typeString);
            u.stringType = typeString;
            u.textureUnitIndex = uniform.attribute("Index").as_int();
            u.noInput = uniform.attribute("NoInput").as_string();
            
            result.push_back(u);
        }
        return result;
    }

    std::vector<MatchboxEffectAttribute> MatchboxEffectProvider::ParseAttributes(xml_document &t_document) {
        std::vector<MatchboxEffectAttribute> result;
        std::vector<std::string> uniformNames;
        for (auto& pass : t_document.child("ShaderNodePreset").children("Shader")) {
            for (auto& uniform : pass.children("Uniform")) {
                std::string uniformName = uniform.attribute("Name").as_string("ADSKUniform");
                bool duplicate = false;
                for (auto& duplicateChild : uniform.children("Duplicate")) {
                    if (std::find(uniformNames.begin(), uniformNames.end(), uniformName) != uniformNames.end()) {
                        duplicate = true;
                    }
                    break;
                }
                if (duplicate) continue;
                std::string attributeName = uniform.attribute("DisplayName").as_string(uniformName.c_str());
                if (uniformName.find("adsk_results_pass") != std::string::npos) continue;
                if (uniformName == "adsk_texture_grid") continue;
                auto min = uniform.attribute("Min").as_float();
                auto max = uniform.attribute("Max").as_float();
                auto inc = uniform.attribute("Inc").as_float(1);

                std::string inputType = uniform.attribute("InputType").as_string();
                bool isSampler = (inputType == "Front") || (inputType == "Back") || (inputType == "Matte") || (std::string(uniform.attribute("Type").as_string()) == "sampler2D");

                std::string type = uniform.attribute("Type").as_string();
                std::any defaultValue = MakeDynamicValue(uniform);
                MatchboxEffectAttribute attribute;
                attribute.attributeName = attributeName;
                attribute.min = min;
                attribute.max = max;
                attribute.inc = inc;
                attribute.inputType = inputType;
                attribute.isSampler = isSampler;
                attribute.defaultValue = defaultValue;
                attribute.uiConditionSource = uniform.attribute("UIConditionSource").as_string();
                attribute.uiConditionValue = uniform.attribute("UIConditionValue").as_string();
                attribute.uiConditionType = uniform.attribute("UIConditionType").as_string();
                attribute.uniformName = uniformName;
                attribute.duplicate = duplicate;
                
                if (type == "vec2" || type == "vec3" || type == "vec4") {
                    float minCandidate = FLT_MAX;
                    float maxCandidate = FLT_MIN;
                    float incCandidate = FLT_MAX;
                    for (auto& subuniform : uniform.children("SubUniform")) {
                        float subMin = subuniform.attribute("Min").as_float(0);
                        float subMax = subuniform.attribute("Max").as_float(1);
                        float subInc = subuniform.attribute("Inc").as_float(1);
                        if (minCandidate > subMin) minCandidate = subMin;
                        if (maxCandidate < subMax) maxCandidate = subMax;
                        if (incCandidate > subInc) incCandidate = subInc;
                    }

                    attribute.max = maxCandidate;
                    attribute.min = minCandidate;
                    attribute.inc = incCandidate;
                }

                result.push_back(attribute);
                uniformNames.push_back(uniformName);
            }
        }
        return result;
    }

    std::any MatchboxEffectProvider::MakeDynamicValue(xml_node& t_node) {
        std::string attributeType = t_node.attribute("Type").as_string();
        if (attributeType == "sampler2D") {
            return Framebuffer();
        }
        if (attributeType == "int" && t_node.attribute("ValueType").as_string() == (std::string) "Popup") {
            std::vector<std::string> choices;
            for (auto& entry : t_node.children("PopupEntry")) {
                choices.push_back(entry.attribute("Title").as_string());
            }
            return Choice(choices);
        }
        if (attributeType == "int") {
            return t_node.attribute("Default").as_int();
        } else if (attributeType == "float") {
            return t_node.attribute("Default").as_float();
        } else if (attributeType == "bool") {
            return t_node.attribute("Default").as_bool();
        } else if (attributeType == "vec2" || attributeType == "vec3" || attributeType == "vec4") {
            std::vector<float> values;
            for (auto& subuniform : t_node.children("SubUniform")) {
                values.push_back(subuniform.attribute("Default").as_float());
            }
            if (values.size() == 2) {
                return glm::vec2(values[0], values[1]);
            } else if (values.size() == 3) {
                return glm::vec3(values[0], values[1], values[2]);
            } else if (values.size() == 4) {
                return glm::vec4(values[0], values[1], values[2], values[3]);
            }
        }
        throw std::runtime_error("unsupported attribute type '" + attributeType + "'");
    }

    MatchboxEffectMetadata MatchboxEffectProvider::ParseMetadata(xml_document& t_document) {
        auto mainNode = t_document.child("ShaderNodePreset");
        MatchboxEffectMetadata result;
        result.commercialUsePermitted = mainNode.attribute("CommercialUsePermitted").as_bool(false);
        result.name = mainNode.attribute("Name").as_string("Matchbox Effect");
        result.description = mainNode.attribute("Description").as_string("");
        result.shaderType = mainNode.attribute("ShaderType").as_string("Matchbox");
        result.version = mainNode.attribute("Version").as_string();
        return result;
    }

    Pipeline MatchboxEffectProvider::GetCachedPipeline(std::string t_fragmentPath) {
        if (s_pipelineCache.find(t_fragmentPath) != s_pipelineCache.end()) {
            return s_pipelineCache[t_fragmentPath];
        }
        auto pipeline = GPU::GeneratePipeline(GPU::s_basicShader, GPU::GenerateShader(ShaderType::Fragment, t_fragmentPath));
        if (pipeline.handle) {
            s_pipelineCache[t_fragmentPath] = pipeline;
        }
        return pipeline;
    }


    Texture MatchboxEffectProvider::GetTextureGrid(std::string t_baseName) {
        if (s_textureGridsCache.find(t_baseName) != s_textureGridsCache.end()) {
            return s_textureGridsCache[t_baseName];
        }

        static std::vector<std::string> textureGridProbes = {
            "textureGrids/%s.textureGrid.jpg",  
            "textureGrids/%s.textureGrid.exr"
        };
        for (auto& probePath : textureGridProbes) {
            std::string path = FormatString(probePath, t_baseName.c_str());
            if (std::filesystem::exists(path)) {
                auto textureGridCandidate = ImageLoader::Load(path);
                if (textureGridCandidate) {
                    auto& textureGrid = *textureGridCandidate;
                    TexturePrecision targetPrecision = TexturePrecision::Usual;
                    if (textureGrid.precision == ImagePrecision::Half) targetPrecision = TexturePrecision::Half;
                    if (textureGrid.precision == ImagePrecision::Full) targetPrecision = TexturePrecision::Full;
                    auto texture = GPU::GenerateTexture(textureGrid.width, textureGrid.height, textureGrid.channels, targetPrecision, true);
                    GPU::UpdateTexture(texture, 0, 0, textureGrid.width, textureGrid.height, textureGrid.channels, textureGrid.data.data());
                    s_textureGridsCache[t_baseName] = texture;
                    return texture;
                }
            }
        }
        return Texture();
    }

    void MatchboxEffectProvider::AbstractRenderProperties() {
        for (auto& attribute : m_data.attributes) {
            if (attribute.isSampler) continue;
            bool wasDisabled = false;
            if (!attribute.uiConditionType.empty()) {
                m_uiStringCache.Lock();
                auto& uiStringCache = m_uiStringCache.GetReference();
                if (uiStringCache.find(attribute.uiConditionSource) != uiStringCache.end()) {
                    auto comparasionValue = uiStringCache[attribute.uiConditionSource];
                    if (comparasionValue != attribute.uiConditionValue) {
                        if (attribute.uiConditionType == "Hide") {
                            m_uiStringCache.Unlock();
                            continue;
                        } else if (attribute.uiConditionType == "Disable") {
                            wasDisabled = true;
                        }
                    }
                }
                m_uiStringCache.Unlock();
            }
            std::vector<std::any> metadata;
            if (attribute.max != 0.0f && attribute.min != 0.0f) {
                metadata.push_back(SliderRangeMetadata(attribute.min, attribute.max));
            }
            metadata.push_back(SliderStepMetadata(attribute.inc));
            if (wasDisabled) ImGui::BeginDisabled();
            RenderAttributeProperty(attribute.uniformName, metadata);
            if (wasDisabled) ImGui::EndDisabled();
        }
    }

    void MatchboxEffectProvider::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json MatchboxEffectProvider::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool MatchboxEffectProvider::AbstractDetailsAvailable() {
        return false;
    }

    std::string MatchboxEffectProvider::AbstractHeader() {
        return m_data.metadata.name;
    }

    std::string MatchboxEffectProvider::Icon() {
        return ICON_FA_SPLOTCH;
    }

    std::optional<std::string> MatchboxEffectProvider::Footer() {
        return std::nullopt;
    }
}
