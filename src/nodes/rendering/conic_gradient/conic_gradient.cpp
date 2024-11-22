#include "conic_gradient.h"
#include "common/gradient_1d.h"

namespace Raster {

    std::optional<Pipeline> ConicGradient::s_pipeline;

    ConicGradient::ConicGradient() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Position", glm::vec2(0.0));
        SetupAttribute("Angle", 0.0f);
        SetupAttribute("Gradient", Gradient1D());
        SetupAttribute("Opacity", 1.0f);
        SetupAttribute("OnlyScreenSpaceRendering", false);

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    ConicGradient::~ConicGradient() {
        if (m_gradientBuffer.has_value()) {
            GPU::DestroyBuffer(m_gradientBuffer.value());
        }
    }

    AbstractPinMap ConicGradient::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "conic_gradient/shader")
            );
        }
        
        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base", t_contextData));
        auto framebuffer = m_framebuffer.Get(baseCandidate);
        auto positionCandidate = GetAttribute<glm::vec2>("Position", t_contextData);
        auto gradientCandidate = GetAttribute<Gradient1D>("Gradient", t_contextData);
        auto angleCandidate = GetAttribute<float>("Angle", t_contextData);
        auto opacityCandidate = GetAttribute<float>("Opacity", t_contextData);
        auto onlySpaceScreenRenderingCandidate = GetAttribute<bool>("OnlyScreenSpaceRendering", t_contextData);
        if (s_pipeline.has_value() && baseCandidate.has_value() && positionCandidate.has_value() && gradientCandidate.has_value() && angleCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto& position = positionCandidate.value();
            auto& opacity = opacityCandidate.value();
            auto& gradient = gradientCandidate.value();
            auto& angle = angleCandidate.value();
            auto& onlySpaceScreenRendering = onlySpaceScreenRenderingCandidate.value();
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);

            bool screenSpaceRendering = !(base.attachments.size() >= 2);
            if (onlySpaceScreenRendering) screenSpaceRendering = true;

            GPU::SetShaderUniform(pipeline.fragment, "uPosition", position);
            GPU::SetShaderUniform(pipeline.fragment, "uAngle", glm::radians(-angle));
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            GPU::SetShaderUniform(pipeline.fragment, "uScreenSpaceRendering", screenSpaceRendering);
            GPU::SetShaderUniform(pipeline.fragment, "uOpacity", opacity);
            if (!screenSpaceRendering) {
                GPU::BindTextureToShader(pipeline.fragment, "uColorTexture", base.attachments.at(0), 0);
                GPU::BindTextureToShader(pipeline.fragment, "uUVTexture", base.attachments.at(1), 1);
            }

            int gradientBufferSize = sizeof(float) + sizeof(float) * 5 * gradient.stops.size();
            char* gradientBufferArray = new char[gradientBufferSize];
            gradient.FillToBuffer(gradientBufferArray);
            if (!m_gradientBuffer.has_value()) {
                m_gradientBuffer = GPU::GenerateBuffer(gradientBufferSize, ArrayBufferType::ShaderStorageBuffer, ArrayBufferUsage::Dynamic);
            }
            auto& gradientBuffer = m_gradientBuffer.value();
            if (gradientBuffer.size != gradientBufferSize) {
                GPU::DestroyBuffer(gradientBuffer);
                gradientBuffer = GPU::GenerateBuffer(gradientBufferSize, ArrayBufferType::ShaderStorageBuffer, ArrayBufferUsage::Dynamic);
            }
            GPU::FillBuffer(gradientBuffer, 0, gradientBufferSize, gradientBufferArray);
            GPU::BindBufferBase(gradientBuffer, 0);
            delete[] gradientBufferArray;

            GPU::DrawArrays(3);
            
            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }

        return result;
    }

    void ConicGradient::AbstractRenderProperties() {
        RenderAttributeProperty("Position", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT),
            SliderStepMetadata(0.05f)
        });
        RenderAttributeProperty("Angle", {
            IconMetadata(ICON_FA_ROTATE),
            SliderStepMetadata(0.05f)
        }); 
        RenderAttributeProperty("Gradient", {
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("Opacity", {
            SliderRangeMetadata(0, 100),
            SliderBaseMetadata(100),
            FormatStringMetadata("%"),
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("OnlyScreenSpaceRendering", {
            IconMetadata(ICON_FA_IMAGE)
        });
    }

    void ConicGradient::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json ConicGradient::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool ConicGradient::AbstractDetailsAvailable() {
        return false;
    }

    std::string ConicGradient::AbstractHeader() {
        return "Conic Gradient";
    }

    std::string ConicGradient::Icon() {
        return ICON_FA_DROPLET;
    }

    std::optional<std::string> ConicGradient::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::ConicGradient>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Conic Gradient",
            .packageName = RASTER_PACKAGED "conic_gradient",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}