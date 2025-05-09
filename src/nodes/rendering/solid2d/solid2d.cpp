#include "solid2d.h"

#include "../../../ImGui/imgui.h"
#include "common/dispatchers.h"



namespace Raster {

    std::optional<Pipeline> Solid2D::s_pipeline;

    Solid2D::Solid2D() {
        NodeBase::Initialize();

        AddInputPin("Base");
        AddOutputPin("Framebuffer");

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Color", glm::vec4(1));
        SetupAttribute("Transform", Transform2D());

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::GenerateShader(ShaderType::Vertex, "solid2d/shader"),
                GPU::GenerateShader(ShaderType::Fragment, "solid2d/shader")
            );
        }
    }

    AbstractPinMap Solid2D::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto& project = Workspace::s_project.value();

        auto& framebuffer = m_managedFramebuffer.Get(GetAttribute<Framebuffer>("Base", t_contextData));
        auto transformCandidate = GetAttribute<Transform2D>("Transform", t_contextData);
        auto colorCandidate = GetAttribute<glm::vec4>("Color", t_contextData);

        if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "RENDERING_PASS", bool)) {
            return {};
        }
        if (s_pipeline.has_value() && transformCandidate.has_value() && colorCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& transform = transformCandidate.value();
            auto& color = colorCandidate.value();

            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);

            GPU::SetShaderUniform(pipeline.vertex, "uMatrix", project.GetProjectionMatrix() * transform.GetTransformationMatrix());
            GPU::SetShaderUniform(pipeline.fragment, "uColor", color);

            GPU::DrawArrays(6);

            TryAppendAbstractPinMap(result, "Framebuffer", framebuffer);
        }

        return result;
    }

    void Solid2D::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Solid2D::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void Solid2D::AbstractRenderProperties() {
        RenderAttributeProperty("Transform", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT)
        });
        RenderAttributeProperty("Color", {
            IconMetadata(ICON_FA_DROPLET)
        });
    }

    bool Solid2D::AbstractDetailsAvailable() {
        return false;
    }

    std::string Solid2D::AbstractHeader() {
        return "Solid2D";
    }

    std::string Solid2D::Icon() {
        return ICON_FA_SQUARE " " ICON_FA_UP_DOWN_LEFT_RIGHT;
    }

    std::optional<std::string> Solid2D::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Solid2D>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Solid2D",
            .packageName = RASTER_PACKAGED "solid2d",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}