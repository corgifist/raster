#include "bezier2d.h"
#include "common/attribute_metadata.h"
#include "common/bezier_curve.h"
#include "common/gradient_1d.h"
#include "../../../ImGui/imgui.h"
#include "common/dispatchers.h"
#include "font/IconsFontAwesome5.h"



namespace Raster {
    Bezier2D::Bezier2D() {
        NodeBase::Initialize();

        AddInputPin("Base");
        AddOutputPin("Framebuffer");

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Bezier", BezierCurve());
        SetupAttribute("Gradient", Gradient1D());
        SetupAttribute("Width", 1.0f);
        SetupAttribute("Quality", 6);
        SetupAttribute("Antialiasing", 1);
    }

    AbstractPinMap Bezier2D::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto& project = Workspace::s_project.value();

        auto& framebuffer = m_managedFramebuffer.Get(GetAttribute<Framebuffer>("Base", t_contextData));
        auto bezierCandidate = GetAttribute<BezierCurve>("Bezier", t_contextData);
        auto gradientCandidate = GetAttribute<Gradient1D>("Gradient", t_contextData);
        auto antialiasingCandidate = GetAttribute<int>("Antialiasing", t_contextData);
        auto widthCandidate = GetAttribute<float>("Width", t_contextData);
        auto qualityCandidate = GetAttribute<int>("Quality", t_contextData);

        auto projectionMatrix = project.GetProjectionMatrix();

        if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "RENDERING_PASS", bool)) {
            return {};
        }

        if (bezierCandidate.has_value() && gradientCandidate.has_value() && antialiasingCandidate && widthCandidate && qualityCandidate) {
            auto bezier = *bezierCandidate;
            auto& gradient = gradientCandidate.value();
            auto& antialiasing = *antialiasingCandidate;
            auto width = *widthCandidate * Compositor::previewResolutionScale;
            auto& quality = *qualityCandidate;
            auto iterations = std::pow(2, quality);

            GPU::BindFramebuffer(framebuffer);

            std::vector<glm::vec2> points;
            for (auto& point : bezier.points) {
                auto point4 = projectionMatrix * glm::vec4(point.x, point.y, 0, 1);
                point = glm::vec2(point4.x, point4.y);
            }
            for (int i = 0; i < iterations; i++) {
                points.push_back(bezier.Get((float) i / (float) iterations));
                points.push_back(bezier.Get((float) (i + 1) / (float) iterations));
            }
            std::vector<glm::vec4> colors;
            for (int i = 0; i < iterations; i++) {
                colors.push_back(gradient.Get((float) i / (float) iterations));
                colors.push_back(gradient.Get((float) (i + 1) / (float) iterations));
            }
            GPU::DrawLines(points, colors, width, glm::vec2(framebuffer.width, framebuffer.height), antialiasing);

            TryAppendAbstractPinMap(result, "Framebuffer", framebuffer);
        }

        return result;
    }

    void Bezier2D::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Bezier2D::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void Bezier2D::AbstractRenderProperties() {
        RenderAttributeProperty("Bezier", {
            IconMetadata(ICON_FA_BEZIER_CURVE)
        });
        RenderAttributeProperty("Width", {
            SliderStepMetadata(0.1f),
            IconMetadata(ICON_FA_LEFT_RIGHT)
        });
        RenderAttributeProperty("Quality", {
            IconMetadata(ICON_FA_GEARS),
            SliderRangeMetadata(1, 12)
        });
        RenderAttributeProperty("Gradient", {
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("Antialiasing", {
            SliderRangeMetadata(0, 10),
            IconMetadata(ICON_FA_GEARS)
        });
    }

    bool Bezier2D::AbstractDetailsAvailable() {
        return false;
    }

    std::string Bezier2D::AbstractHeader() {
        return "Bezier2D";
    }

    std::string Bezier2D::Icon() {
        return ICON_FA_BEZIER_CURVE;
    }

    std::optional<std::string> Bezier2D::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Bezier2D>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Bezier2D",
            .packageName = RASTER_PACKAGED "bezier2D",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}