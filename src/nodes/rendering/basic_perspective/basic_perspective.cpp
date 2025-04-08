#include "basic_perspective.h"
#include "common/attribute_metadata.h"
#include "common/ui_helpers.h"
#include "compositor/texture_interoperability.h"
#include "font/IconsFontAwesome5.h"
#include "gpu/gpu.h"

namespace Raster {

    std::optional<Pipeline> BasicPerspective::s_pipeline;

    BasicPerspective::BasicPerspective() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Texture", Texture());
        SetupAttribute("Position", glm::vec3(0, 0, 0));
        SetupAttribute("Size", glm::vec2(1, 1));
        SetupAttribute("Scale", 1.0f);
        SetupAttribute("Rotation", glm::vec3(0, 0, 0));
        SetupAttribute("Anchor", glm::vec3(0, 0, 0));
        SetupAttribute("FOV", 45.0f);
        SetupAttribute("RespectAspectRatio", true);

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    BasicPerspective::~BasicPerspective() {
    }

    AbstractPinMap BasicPerspective::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto& project = Workspace::GetProject();
        
        if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "RENDERING_PASS", bool)) {
            return {};
        }

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::GenerateShader(ShaderType::Vertex, "basic_perspective/shader"),
                GPU::GenerateShader(ShaderType::Fragment, "basic_perspective/shader")
            );
        }

        auto baseCandidate = GetAttribute<Framebuffer>("Base", t_contextData);
        auto textureCandidate = GetAttribute<Texture>("Texture", t_contextData);
        auto positionCandidate = GetAttribute<glm::vec3>("Position", t_contextData);
        auto sizeCandidate = GetAttribute<glm::vec2>("Size", t_contextData);
        auto scaleCandidate = GetAttribute<float>("Scale", t_contextData);
        auto rotationCandidate = GetAttribute<glm::vec3>("Rotation", t_contextData);
        auto anchorCandidate = GetAttribute<glm::vec3>("Anchor", t_contextData);
        auto respectAspectRatioCandidate = GetAttribute<bool>("RespectAspectRatio", t_contextData);
        auto fovCandidate = GetAttribute<float>("FOV", t_contextData);
        auto& framebuffer = m_framebuffer.Get(baseCandidate);

        if (s_pipeline && textureCandidate && positionCandidate && sizeCandidate && scaleCandidate && rotationCandidate && anchorCandidate && fovCandidate && respectAspectRatioCandidate) {
            auto& pipeline = s_pipeline.value();
            GPU::BindPipeline(pipeline);
            GPU::BindFramebuffer(framebuffer);
            auto& texture = *textureCandidate;
            auto position = *positionCandidate;
            auto& size = *sizeCandidate;
            auto& scale = *scaleCandidate;
            auto& rotation = *rotationCandidate;
            auto& anchor = *anchorCandidate;
            auto& respectAspectRation = *respectAspectRatioCandidate;
            auto& fov = *fovCandidate;

            auto projectionMatrix = glm::perspective(glm::radians(-fov), (float) framebuffer.width / (float) framebuffer.height, 0.001f, 100.0f);
            auto aspectRatio = texture.handle ? (float) texture.width / (float) texture.height : 1.0f;
            if (texture.handle) {
                size *= glm::vec2((float) texture.width / (float) texture.height, 1);
            }
            position.x *= -1;
            size.x *= -1;
            position.z -= 2.42f;
            auto transform = glm::identity<glm::mat4>();
            transform = glm::translate(transform, position);
            transform = glm::translate(transform, anchor);
            transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(1, 0, 0));
            transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(0, 1, 0));
            transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0, 0, 1));
            transform = glm::translate(transform, -anchor); 
            transform = glm::scale(transform, glm::vec3(size * scale, 1.0)); 

            GPU::SetShaderUniform(pipeline.vertex, "uMatrix", projectionMatrix * transform);
            GPU::SetShaderUniform(pipeline.fragment, "uTextureAvailable", texture.handle ? true : false);
            if (texture.handle) {
                GPU::BindTextureToShader(pipeline.fragment, "uTexture", texture, 0);
            }
            GPU::SetShaderUniform(pipeline.fragment, "uAspectRatio", aspectRatio);
            GPU::DrawArrays(6);
            TryAppendAbstractPinMap(result, "Output", framebuffer);

        }

        return result;
    }

    void BasicPerspective::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json BasicPerspective::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void BasicPerspective::AbstractRenderProperties() {
        RenderAttributeProperty("Position", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT),
            SliderStepMetadata(0.05)
        });
        RenderAttributeProperty("Size", {
            IconMetadata(ICON_FA_EXPAND),
            SliderStepMetadata(0.05)
        });
        RenderAttributeProperty("Scale", {
            IconMetadata(ICON_FA_EXPAND),
            SliderStepMetadata(0.05)
        });
        RenderAttributeProperty("Rotation", {
            IconMetadata(ICON_FA_ROTATE),
            SliderStepMetadata(0.05)
        });
        RenderAttributeProperty("Anchor", {
            IconMetadata(ICON_FA_ANCHOR),
            SliderStepMetadata(0.05)
        });
        RenderAttributeProperty("FOV", {
            IconMetadata(ICON_FA_CAMERA),
            SliderRangeMetadata(1, 360)
        });
        RenderAttributeProperty("RespectAspectRatio", {
            IconMetadata(ICON_FA_IMAGE)
        });
    }

    bool BasicPerspective::AbstractDetailsAvailable() {
        return false;
    }

    std::string BasicPerspective::AbstractHeader() {
        return "Basic Perspective";
    }

    std::string BasicPerspective::Icon() {
        return ICON_FA_IMAGE;
    }

    std::optional<std::string> BasicPerspective::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::BasicPerspective>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Basic Perspective",
            .packageName = RASTER_PACKAGED "basic_perspective",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}