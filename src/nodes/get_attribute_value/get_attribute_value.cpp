#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "get_attribute_value.h"
#include "../../ImGui/imgui.h"

namespace Raster {

    GetAttributeValue::GetAttributeValue() {
        NodeBase::Initialize();
        NodeBase::GenerateFlowPins();
        this->m_attributes["AttributeName"] = std::string("");

        AddOutputPin("Value");
    }

    AbstractPinMap GetAttributeValue::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto attributeCandidate = GetCompositionAttribute();
        if (attributeCandidate.has_value()) {
            auto& attribute = attributeCandidate.value();
            auto& project = Workspace::s_project.value();
            auto parentComposition = Workspace::GetCompositionByNodeID(nodeID).value();
            TryAppendAbstractPinMap(result, "Value", attribute->Get(project.currentFrame - parentComposition->beginFrame, parentComposition));
        }
        return result;
    }

    std::optional<AbstractAttribute> GetAttributeValue::GetCompositionAttribute() {
        auto attributeNameCandidate = GetAttribute<std::string>("AttributeName");
        if (attributeNameCandidate.has_value()) {
            std::string attributeName = attributeNameCandidate.value();
            auto parentComposition = Workspace::GetCompositionByNodeID(nodeID).value();
            auto attributeCandidate = Workspace::GetAttributeByName(
                parentComposition,
                attributeName
            );
            if (attributeCandidate.has_value()) {
                auto& attribute = attributeCandidate.value();
                auto& project = Workspace::s_project.value();
                return attribute;
            }
        }
        return std::nullopt;
    }

    void GetAttributeValue::AbstractRenderProperties() {
        RenderAttributeProperty("AttributeName");
    }

    bool GetAttributeValue::AbstractDetailsAvailable() {
        return GetCompositionAttribute().has_value();
    }

    void GetAttributeValue::AbstractRenderDetails() {
        auto attributeCandidate = GetCompositionAttribute();
        if (attributeCandidate.has_value()) {
            auto& attribute = attributeCandidate.value();
            attribute->AbstractRenderDetails();
        }
    }

    std::string GetAttributeValue::AbstractHeader() {
        std::string base = "Get Attribute Value";
        auto attributeNameCandidate = GetAttribute<std::string>("AttributeName");
        if (!attributeNameCandidate.value().empty()) {
            return base + " (" + attributeNameCandidate.value() + ")";
        }
        return base;
    }

    std::string GetAttributeValue::Icon() {
        return ICON_FA_STOPWATCH;
    }

    std::optional<std::string> GetAttributeValue::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::GetAttributeValue>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Get Attribute Value",
            .packageName = RASTER_PACKAGED_PACKAGE "get_attribute_value",
            .category = Raster::NodeCategory::Attributes
        };
    }
}