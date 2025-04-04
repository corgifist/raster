#include "tracking_motion_blur.h"

namespace Raster {

    std::optional<Pipeline> TrackingMotionBlur::s_pipeline;
    std::optional<Sampler> TrackingMotionBlur::s_sampler;

    TrackingMotionBlur::TrackingMotionBlur() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Transform", Transform2D());
        SetupAttribute("BlurIntensity", 1.0f);
        SetupAttribute("Samples", 50);

        AddInputPin("Base");
        AddOutputPin("Framebuffer");
    }

    TrackingMotionBlur::~TrackingMotionBlur() {
        if (m_framebuffer.Get().handle) {
            m_framebuffer.Destroy();
        }

        if (m_temporalFramebuffer.Get().handle) {
            m_temporalFramebuffer.Destroy();
        }
    }

    AbstractPinMap TrackingMotionBlur::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto& project = Workspace::GetProject();

        auto baseCandidate = GetAttribute<Framebuffer>("Base", t_contextData);
        auto baseTransformCandidate = GetAttribute<Transform2D>("Transform", t_contextData);
        auto blurIntensityCandidate = GetAttribute<float>("BlurIntensity", t_contextData);
        auto samplesCandidate = GetAttribute<int>("Samples", t_contextData);

        if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "RENDERING_PASS", bool)) {
            return {};
        }
        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "tracking_motion_blur/shader")
            );

            s_sampler = GPU::GenerateSampler();
            GPU::SetSamplerTextureWrappingMode(s_sampler.value(), TextureWrappingAxis::S, TextureWrappingMode::MirroredRepeat);
            GPU::SetSamplerTextureWrappingMode(s_sampler.value(), TextureWrappingAxis::T, TextureWrappingMode::MirroredRepeat);
        }
        if (s_pipeline.has_value() && s_sampler.has_value() && baseCandidate.has_value() && baseTransformCandidate.has_value() && blurIntensityCandidate.has_value() && samplesCandidate.has_value() && baseCandidate.value().attachments.size() > 0) {
            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);
            Compositor::EnsureResolutionConstraintsForFramebuffer(m_temporalFramebuffer);
            float aspect = (float) m_framebuffer.width / (float) m_framebuffer.height;
            auto& base = baseCandidate.value();
            auto& baseTransform = baseTransformCandidate.value();
            auto& blurIntensity = blurIntensityCandidate.value();
            auto& pipeline = s_pipeline.value();
            auto& sampler = s_sampler.value();
            auto& samples = samplesCandidate.value();
            
            project.TimeTravel(-1);
            
            auto previousTransformCandidate = GetAttribute<Transform2D>("Transform", t_contextData);
            if (previousTransformCandidate.has_value()) {
                auto& previousTransform = previousTransformCandidate.value();

                auto resolution = glm::vec2(m_framebuffer.width, m_framebuffer.height);
                auto positionDifference = NDCToScreen(baseTransform.DecomposePosition(), resolution) - NDCToScreen(previousTransform.DecomposePosition(), resolution);

                float angleDifference = baseTransform.DecomposeRotation() - previousTransform.DecomposeRotation();

                glm::vec4 centerPointNDC4 = project.GetProjectionMatrix() * baseTransform.GetTransformationMatrix() * glm::vec4(0, 0, 0, 1);
                glm::vec2 centerPointNDC(centerPointNDC4.x, centerPointNDC4.y);

                glm::vec2 centerPointUV = NDCToScreen(centerPointNDC, resolution) / resolution;
                centerPointUV.y = 1 - centerPointUV.y;

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

                auto& framebuffer = m_framebuffer.GetFrontFramebuffer();
                auto& temporalFramebuffer = m_temporalFramebuffer.GetFrontFramebuffer(); 

                GPU::BindFramebuffer(framebuffer);
                GPU::BindPipeline(pipeline);
                GPU::BindSampler(sampler, 0);

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

                GPU::BindFramebuffer(temporalFramebuffer);
                GPU::ClearFramebuffer(0, 0, 0, 0);

                GPU::SetShaderUniform(pipeline.fragment, "uStage", 1);
                GPU::BindTextureToShader(pipeline.fragment, "uTexture", framebuffer.attachments.at(0), 0);

                GPU::DrawArrays(3);

                GPU::BindFramebuffer(framebuffer);
                GPU::ClearFramebuffer(0, 0, 0, 0);

                GPU::SetShaderUniform(pipeline.fragment, "uStage", 0);
                GPU::BindTextureToShader(pipeline.fragment, "uTexture", temporalFramebuffer.attachments.at(0), 0);

                GPU::DrawArrays(3);

                GPU::BindSampler(std::nullopt, 0);

                TryAppendAbstractPinMap(result, "Framebuffer", framebuffer);

            }

            project.ResetTimeTravel();
        }

        return result;
    }

    void TrackingMotionBlur::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json TrackingMotionBlur::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void TrackingMotionBlur::AbstractRenderProperties() {
        RenderAttributeProperty("BlurIntensity", {
            IconMetadata(ICON_FA_PERCENT)
        });
        RenderAttributeProperty("Samples", {
            IconMetadata(ICON_FA_GEARS)
        });
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
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::TrackingMotionBlur>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Tracking Motion Blur",
            .packageName = RASTER_PACKAGED "tracking_motion_blur",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}