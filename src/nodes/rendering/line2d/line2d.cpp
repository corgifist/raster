#include "line2d.h"
#include "common/attribute_metadata.h"
#include "common/line2d.h"

#include "../../../ImGui/imgui.h"
#include "common/dispatchers.h"



namespace Raster {
    Line2DNode::Line2DNode() {
        NodeBase::Initialize();

        AddInputPin("Base");
        AddOutputPin("Framebuffer");

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Line", Line2D());
        SetupAttribute("Color", glm::vec4(1));
        SetupAttribute("Width", 1.0f);
        SetupAttribute("Antialiasing", 1);
    }

    AbstractPinMap Line2DNode::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto& project = Workspace::s_project.value();

        auto& framebuffer = m_managedFramebuffer.Get(GetAttribute<Framebuffer>("Base", t_contextData));
        auto lineCandidate = GetAttribute<Line2D>("Line", t_contextData);
        auto colorCandidate = GetAttribute<glm::vec4>("Color", t_contextData);
        auto antialiasingCandidate = GetAttribute<int>("Antialiasing", t_contextData);
        auto widthCandidate = GetAttribute<float>("Width", t_contextData);

        auto projectionMatrix = project.GetProjectionMatrix();

        if (lineCandidate.has_value() && colorCandidate.has_value()) {
            auto& line = *lineCandidate;
            auto& color = colorCandidate.value();
            auto& antialiasing = *antialiasingCandidate;
            auto& width = *widthCandidate;

            GPU::BindFramebuffer(framebuffer);

            auto transformedBegin = (projectionMatrix * glm::vec4(line.begin, 0, 1));
            auto transformedEnd = (projectionMatrix * glm::vec4(line.end, 0, 1));

            std::vector<glm::vec2> points = {
                glm::vec2(transformedBegin.x, transformedBegin.y), glm::vec2(transformedEnd.x, transformedEnd.y)
            };
            std::vector<glm::vec4> colors = {
                line.beginColor * color, line.endColor * color
            };
            GPU::DrawLines(points, colors, width, glm::vec2(framebuffer.width, framebuffer.height), antialiasing);

            TryAppendAbstractPinMap(result, "Framebuffer", framebuffer);
        }

        return result;
    }

    void Line2DNode::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Line2DNode::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void Line2DNode::AbstractRenderProperties() {
        RenderAttributeProperty("Line", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT)
        });
        RenderAttributeProperty("Width", {
            SliderStepMetadata(0.1f),
            IconMetadata(ICON_FA_LEFT_RIGHT)
        });
        RenderAttributeProperty("Color", {
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("Antialiasing", {
            SliderRangeMetadata(0, 10),
            IconMetadata(ICON_FA_GEARS)
        });
    }

    bool Line2DNode::AbstractDetailsAvailable() {
        return false;
    }

    std::string Line2DNode::AbstractHeader() {
        return "Line2D";
    }

    std::string Line2DNode::Icon() {
        return ICON_FA_SQUARE " " ICON_FA_UP_DOWN_LEFT_RIGHT;
    }

    std::optional<std::string> Line2DNode::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Line2DNode>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Line2D",
            .packageName = RASTER_PACKAGED "line2d",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}