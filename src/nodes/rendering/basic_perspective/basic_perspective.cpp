#include "basic_perspective.h"
#include "common/attribute_metadata.h"
#include "common/ui_helpers.h"
#include "compositor/texture_interoperability.h"
#include "font/IconsFontAwesome5.h"
#include "gpu/gpu.h"
#include "common/transform3d.h"

namespace Raster {

    std::optional<Pipeline> BasicPerspective::s_pipeline;
    std::optional<Pipeline> BasicPerspective::s_geometryPipeline;
    std::optional<Pipeline> BasicPerspective::s_combinerPipeline;

    BasicPerspective::BasicPerspective() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("InputGeometryBuffer", Framebuffer());
        SetupAttribute("Texture", Texture());
        SetupAttribute("Transform", Transform3D());

        AddInputPin("Base");
        AddOutputPin("Output");
        AddOutputPin("GeometryBuffer");

        m_hasCamera = false;
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
            auto sh = GPU::GenerateShader(ShaderType::Vertex, "basic_perspective/shader");
            s_pipeline = GPU::GeneratePipeline(
                sh,
                GPU::GenerateShader(ShaderType::Fragment, "basic_perspective/shader")
            );
            s_geometryPipeline = GPU::GeneratePipeline(
                sh,
                GPU::GenerateShader(ShaderType::Fragment, "basic_perspective/geometry")
            );
            s_combinerPipeline = GPU::GeneratePipeline(
                sh,
                GPU::GenerateShader(ShaderType::Fragment, "basic_perspective/geometry_combiner")
            );
        }

        auto cameraCandidate = project.GetCamera();
        if (!cameraCandidate) {
            m_hasCamera = false;
            return {};
        }
        m_hasCamera = true;
        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base", t_contextData));
        auto inputGeometryBufferCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("InputGeometryBuffer", t_contextData));
        auto textureCandidate = TextureInteroperability::GetTexture(GetDynamicAttribute("Texture", t_contextData));
        auto transformCandidate = GetAttribute<Transform3D>("Transform", t_contextData);
        auto& framebuffer = m_framebuffer.Get(baseCandidate);
        auto& geometryBuffer = m_geometryBuffer.Get(baseCandidate);
        auto& temporaryBuffer = m_temporaryBuffer.Get(baseCandidate);

        if (s_pipeline && inputGeometryBufferCandidate && textureCandidate && transformCandidate) {
            auto& pipeline = s_pipeline.value();
            auto& combinerPipeline = *s_combinerPipeline;
            auto& geometryPipeline = *s_geometryPipeline;
            auto& inputGeometryBuffer = *inputGeometryBufferCandidate;
            GPU::BindPipeline(pipeline);
            GPU::BindFramebuffer(framebuffer);
            auto& texture = *textureCandidate;
            auto& transformObject = *transformCandidate;
            auto& camera = *cameraCandidate;

            auto aspectRatio = texture.handle ? (float) texture.width / (float) texture.height : 1.0f;
            auto projectionMatrix = camera.GetProjectionMatrix();
            auto transform = transformObject.GetTransformationMatrix();
            transform = camera.GetTransformationMatrix() * transform;

            GPU::SetShaderUniform(pipeline.vertex, "uMatrix", projectionMatrix * transform);
            GPU::SetShaderUniform(pipeline.fragment, "uTextureAvailable", texture.handle ? true : false);
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            if (texture.handle) {
                GPU::BindTextureToShader(pipeline.fragment, "uTexture", texture, 0);
            }
            GPU::SetShaderUniform(pipeline.fragment, "uDepthAvailable", inputGeometryBuffer.handle ? true : false);
            if (inputGeometryBuffer.handle) {
                GPU::BindTextureToShader(pipeline.fragment, "uDepth", inputGeometryBuffer.attachments.at(0), 1);
            }
            GPU::SetShaderUniform(pipeline.fragment, "uAspectRatio", aspectRatio);
            GPU::DrawArrays(6);

            GPU::BindFramebuffer(temporaryBuffer);
            GPU::BindPipeline(geometryPipeline);
            GPU::ClearFramebuffer(0, 0, 0, 0);
            GPU::SetShaderUniform(geometryPipeline.vertex, "uMatrix", projectionMatrix * transform);
            GPU::DrawArrays(6);

            if (inputGeometryBuffer.handle) {
                GPU::BindFramebuffer(geometryBuffer);
                GPU::BindPipeline(combinerPipeline);
                GPU::ClearFramebuffer(0, 0, 0, 0);
                GPU::SetShaderUniform(combinerPipeline.vertex, "uMatrix", glm::scale(glm::vec3(100, 100, 100)));
                GPU::SetShaderUniform(combinerPipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
                GPU::BindTextureToShader(combinerPipeline.fragment, "uA", temporaryBuffer.attachments.at(0), 0);
                GPU::BindTextureToShader(combinerPipeline.fragment, "uB", inputGeometryBuffer.attachments.at(0), 1);
                GPU::DrawArrays(6);
            }

            TryAppendAbstractPinMap(result, "Output", framebuffer);
            TryAppendAbstractPinMap(result, "GeometryBuffer", inputGeometryBuffer.handle ? geometryBuffer : temporaryBuffer);
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
        RenderAttributeProperty("Transform", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT)
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
        return m_hasCamera ? std::nullopt : std::optional<std::string>(FormatString("%s %s", ICON_FA_CAMERA, Localization::GetString("NO_ACTIVE_CAMERA").c_str()));
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