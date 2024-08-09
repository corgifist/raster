#include "tracking_motion_blur.h"

namespace Raster {

    std::optional<Pipeline> TrackingMotionBlur::s_pipeline;

    TrackingMotionBlur::TrackingMotionBlur() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Transform", Transform2D());
        SetupAttribute("BlurIntensity", 1.0f);
        SetupAttribute("Samples", 50);

        AddInputPin("Base");
        AddOutputPin("Framebuffer");

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::GenerateShader(ShaderType::Vertex, "tracking_motion_blur/shader"),
                GPU::GenerateShader(ShaderType::Fragment, "tracking_motion_blur/shader")
            );
        }
    }

    AbstractPinMap TrackingMotionBlur::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto& project = Workspace::GetProject();

        auto baseCandidate = GetAttribute<Framebuffer>("Base");
        auto baseTransformCandidate = GetAttribute<Transform2D>("Transform");
        auto blurIntensityCandidate = GetAttribute<float>("BlurIntensity");
        auto samplesCandidate = GetAttribute<int>("Samples");
        if (s_pipeline.has_value() && baseCandidate.has_value() && baseTransformCandidate.has_value() && blurIntensityCandidate.has_value() && samplesCandidate.has_value() && baseCandidate.value().attachments.size() > 0) {
            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);
            Compositor::EnsureResolutionConstraintsForFramebuffer(m_temporalFramebuffer);
            float aspect = (float) m_framebuffer.width / (float) m_framebuffer.height;
            auto& base = baseCandidate.value();
            auto& baseTransform = baseTransformCandidate.value();
            auto& blurIntensity = blurIntensityCandidate.value();
            auto& pipeline = s_pipeline.value();
            auto& samples = samplesCandidate.value();
            
            project.TimeTravel(-1);
            
            ClearAttributesCache();
            auto previousTransformCandidate = GetAttribute<Transform2D>("Transform");
            if (previousTransformCandidate.has_value()) {
                auto& previousTransform = previousTransformCandidate.value();

                auto resolution = glm::vec2(m_framebuffer.width, m_framebuffer.height);
                auto positionDifference = NDCToScreen(baseTransform.DecomposePosition(), resolution) - NDCToScreen(previousTransform.DecomposePosition(), resolution);

                float angleDifference = std::fmod(baseTransform.DecomposeRotation(), 179) - std::fmod(previousTransform.DecomposeRotation(), 179);

                glm::vec4 centerPointNDC4 = project.GetProjectionMatrix() * baseTransform.GetTransformationMatrix() * glm::vec4(0, 0, 0, 1);
                glm::vec2 centerPointNDC(centerPointNDC4.x, centerPointNDC4.y);

                glm::vec2 centerPointUV = NDCToScreen(centerPointNDC, resolution) / resolution;

                glm::vec2 currentSize = baseTransform.DecomposeSize();
                currentSize.x = (currentSize.x / aspect) * m_framebuffer.width;
                currentSize.y = (currentSize.y) * m_framebuffer.height;

                glm::vec2 previousSize = previousTransform.DecomposeSize();
                previousSize.x = (previousSize.x / aspect) * m_framebuffer.width;
                previousSize.y = (previousSize.y) * m_framebuffer.height;

                glm::vec2 sizeDifference = currentSize - previousSize;
                positionDifference *= blurIntensity;
                angleDifference *= blurIntensity;
                sizeDifference *= blurIntensity;

                GPU::BindFramebuffer(m_framebuffer);
                GPU::BindPipeline(pipeline);

                GPU::ClearFramebuffer(0, 0, 0, 0);

                GPU::SetShaderUniform(pipeline.fragment, "uResolution", resolution);
                GPU::SetShaderUniform(pipeline.fragment, "uLinearBlurIntensity", positionDifference);
                GPU::SetShaderUniform(pipeline.fragment, "uAngularBlurAngle", glm::radians(-angleDifference));
                GPU::SetShaderUniform(pipeline.fragment, "uRadialBlurIntensity", sizeDifference);
                GPU::SetShaderUniform(pipeline.fragment, "uCenter", centerPointUV);
                GPU::SetShaderUniform(pipeline.fragment, "uSamples", (float) samples);

                GPU::SetShaderUniform(pipeline.fragment, "uStage", 2);

                GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);
                GPU::DrawArrays(3);

                GPU::BindFramebuffer(m_temporalFramebuffer);
                GPU::ClearFramebuffer(0, 0, 0, 0);

                GPU::SetShaderUniform(pipeline.fragment, "uStage", 1);
                GPU::BindTextureToShader(pipeline.fragment, "uTexture", m_framebuffer.attachments.at(0), 0);

                GPU::DrawArrays(3);

                GPU::BindFramebuffer(m_framebuffer);
                GPU::ClearFramebuffer(0, 0, 0, 0);

                GPU::SetShaderUniform(pipeline.fragment, "uStage", 0);
                GPU::BindTextureToShader(pipeline.fragment, "uTexture", m_temporalFramebuffer.attachments.at(0), 0);

                GPU::DrawArrays(3);

                TryAppendAbstractPinMap(result, "Framebuffer", m_framebuffer);

            }

            project.ResetTimeTravel();
        }

        return result;
    }

    void TrackingMotionBlur::AbstractRenderProperties() {
        RenderAttributeProperty("BlurIntensity");
        RenderAttributeProperty("Samples");
    }

    bool TrackingMotionBlur::AbstractDetailsAvailable() {
        return false;
    }

    std::string TrackingMotionBlur::AbstractHeader() {
        return "Tracking Motion Blur";
    }

    std::string TrackingMotionBlur::Icon() {
        return ICON_FA_VECTOR_SQUARE;
    }

    std::optional<std::string> TrackingMotionBlur::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::TrackingMotionBlur>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Tracking Motion Blur",
            .packageName = RASTER_PACKAGED "tracking_motion_blur",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}