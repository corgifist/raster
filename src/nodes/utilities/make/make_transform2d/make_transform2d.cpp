#include "make_transform2d.h"

namespace Raster {

    MakeTransform2D::MakeTransform2D() {
        NodeBase::Initialize();

        this->m_attributes["Position"] = glm::vec2(0, 0);
        this->m_attributes["Size"] = glm::vec2(1, 1);
        this->m_attributes["Anchor"] = glm::vec2(0, 0);
        this->m_attributes["Angle"] = 0.0f;
        this->m_attributes["ParentTransform"] = Transform2D();

        AddOutputPin("Output");
    }

    AbstractPinMap MakeTransform2D::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto positionCandidate = GetAttribute<glm::vec2>("Position");
        auto sizeCandidate = GetAttribute<glm::vec2>("Size");
        auto anchorCandidate = GetAttribute<glm::vec2>("Anchor");
        auto angleCandidate = GetAttribute<float>("Angle");
        auto parentTransformCandidate = GetAttribute<Transform2D>("ParentTransform");
        if (positionCandidate.has_value() && sizeCandidate.has_value() && anchorCandidate.has_value() && angleCandidate.has_value() && parentTransformCandidate.has_value()) {
            auto& position = positionCandidate.value();
            auto& size = sizeCandidate.value();
            auto& anchor = anchorCandidate.value();
            auto& angle = angleCandidate.value();
            auto& parentTransform = parentTransformCandidate.value();

            Transform2D resultTransform;
            resultTransform.position = position;
            resultTransform.size = size;
            resultTransform.anchor = anchor;
            resultTransform.angle = angle;
            resultTransform.parentMatrix = parentTransform.GetTransformationMatrix();
            TryAppendAbstractPinMap(result, "Output", resultTransform);
        }
        return result;
    }

    void MakeTransform2D::AbstractRenderProperties() {
    }

    bool MakeTransform2D::AbstractDetailsAvailable() {
        return false;
    }

    std::string MakeTransform2D::AbstractHeader() {
        return "Make Transform2D";
    }

    std::string MakeTransform2D::Icon() {
        return ICON_FA_UP_DOWN_LEFT_RIGHT;
    }

    std::optional<std::string> MakeTransform2D::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MakeTransform2D>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Make Transform2D",
            .packageName = RASTER_PACKAGED "make_transform2d",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}