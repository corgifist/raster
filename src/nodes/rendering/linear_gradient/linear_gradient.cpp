#include "linear_gradient.h"
#include "common/gradient_1d.h"

namespace Raster {

    std::optional<Pipeline> LinearGradient::s_pipeline;

    LinearGradient::LinearGradient() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Gradient", Gradient1D());
        SetupAttribute("UseYPlane", false);
        SetupAttribute("Opacity", 1.0f);
        SetupAttribute("OnlyScreenSpaceRendering", false);
        this->m_useYPlane = false;

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    LinearGradient::~LinearGradient() {
        if (m_gradientBuffer.has_value()) {
            GPU::DestroyBuffer(m_gradientBuffer.value());
        }
    }

    AbstractPinMap LinearGradient::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "linear_gradient/shader")
            );
        }
        
        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base", t_contextData));
        auto framebuffer = m_framebuffer.Get(baseCandidate);
        auto useYPlaneCandidate = GetAttribute<bool>("UseYPlane", t_contextData);
        auto gradientCandidate = GetAttribute<Gradient1D>("Gradient", t_contextData);
        auto opacityCandidate = GetAttribute<float>("Opacity", t_contextData);
        auto onlySpaceScreenRenderingCandidate = GetAttribute<bool>("OnlyScreenSpaceRendering", t_contextData);
        if (s_pipeline.has_value() && baseCandidate.has_value() && gradientCandidate.has_value() && useYPlaneCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto& useYPlane = useYPlaneCandidate.value();
            auto& opacity = opacityCandidate.value();
            auto& gradient = gradientCandidate.value();
            auto& onlySpaceScreenRendering = onlySpaceScreenRenderingCandidate.value();
            m_useYPlane = useYPlane;
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);

            bool screenSpaceRendering = !(base.attachments.size() >= 2);
            if (onlySpaceScreenRendering) screenSpaceRendering = true;

            GPU::SetShaderUniform(pipeline.fragment, "uUseYPlane", useYPlane);
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

    void LinearGradient::AbstractRenderProperties() {
        RenderAttributeProperty("UseYPlane", {
            IconMetadata(ICON_FA_UP_DOWN)
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

    void LinearGradient::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json LinearGradient::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool LinearGradient::AbstractDetailsAvailable() {
        return false;
    }

    std::string LinearGradient::AbstractHeader() {
        return "Linear Gradient";
    }

    std::string LinearGradient::Icon() {
        return m_useYPlane ? ICON_FA_UP_DOWN : ICON_FA_LEFT_RIGHT;
    }

    std::optional<std::string> LinearGradient::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::LinearGradient>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Linear Gradient",
            .packageName = RASTER_PACKAGED "linear_gradient",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}