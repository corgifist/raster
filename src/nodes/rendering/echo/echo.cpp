#include "echo.h"

namespace Raster {

    std::optional<Pipeline> Echo::s_echoPipeline;

    Echo::Echo() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Steps", 5);
        SetupAttribute("FrameStep", 5);

        AddInputPin("Base");
        AddOutputPin("Framebuffer");
    }

    Echo::~Echo() {
        if (m_framebuffer.Get().handle) {
            m_framebuffer.Destroy();
        }
    }

    AbstractPinMap Echo::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        if (!s_echoPipeline.has_value()) {
            s_echoPipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "echo/shader")
            );
        }

        auto& project = Workspace::GetProject();
        auto requiredResolution = Compositor::GetRequiredResolution();
        
        auto stepsCandidate = GetAttribute<int>("Steps", t_contextData);
        auto frameStepCandidate = GetAttribute<int>("FrameStep", t_contextData);
        if (stepsCandidate.has_value() && s_echoPipeline.has_value()) {
            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);
            auto framebuffer = m_framebuffer.GetFrontFramebuffer();
            GPU::BindFramebuffer(framebuffer);
            GPU::ClearFramebuffer(0, 0, 0, 0);
            auto steps = stepsCandidate.value();
            auto& frameStep = frameStepCandidate.value();
            float opacityStep = 1.0f / ((float) steps + 1);

            project.TimeTravel(-steps * frameStep);

            auto& pipeline = s_echoPipeline.value();

            for (int i = 0; i < steps + 1; i++) {
                auto baseCandidate = GetAttribute<Framebuffer>("Base", t_contextData);
                if (baseCandidate.has_value()) {
                    if (project.GetCorrectCurrentTime() < 0) {
                        project.TimeTravel(frameStep);
                        continue;
                    }
                    auto& base = baseCandidate.value();
                    if (base.attachments.size() <= 2 || !base.handle) {
                        GPU::BindPipeline(pipeline);
                        GPU::BindFramebuffer(framebuffer);
                        GPU::SetShaderUniform(pipeline.fragment, "uResolution", requiredResolution);
                        GPU::SetShaderUniform(pipeline.fragment, "uOpacity", std::clamp(opacityStep * (i + 1), 0.0f, 1.0f));
                        GPU::BindTextureToShader(pipeline.fragment, "uColorTexture", base.attachments.at(0), 0);
                        GPU::BindTextureToShader(pipeline.fragment, "uUVTexture", base.attachments.at(1), 1);

                        GPU::DrawArrays(3);
                    }

                    project.TimeTravel(frameStep);
                }
            }
            TryAppendAbstractPinMap(result, "Framebuffer", framebuffer);
            project.ResetTimeTravel();
        }


        return result;
    }

    void Echo::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Echo::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void Echo::AbstractRenderProperties() {
        RenderAttributeProperty("Steps");
        RenderAttributeProperty("FrameStep");
    }

    bool Echo::AbstractDetailsAvailable() {
        return false;
    }

    std::string Echo::AbstractHeader() {
        return "Echo";
    }

    std::string Echo::Icon() {
        return ICON_FA_IMAGES;
    }

    std::optional<std::string> Echo::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Echo>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Echo",
            .packageName = RASTER_PACKAGED "echo",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}