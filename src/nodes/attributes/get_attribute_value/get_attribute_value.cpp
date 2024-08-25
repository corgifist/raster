#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "get_attribute_value.h"
#include "../../../ImGui/imgui.h"

namespace Raster {

    GetAttributeValue::GetAttributeValue() {
        NodeBase::Initialize();

        SetupAttribute("AttributeName", std::string(""));
        SetupAttribute("AttributeID", 0);

        AddOutputPin("Value");
    }

    AbstractPinMap GetAttributeValue::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto attributeCandidate = GetCompositionAttribute();
        if (attributeCandidate.has_value()) {
            auto& attribute = attributeCandidate.value();
            auto& project = Workspace::s_project.value();
            auto parentComposition = Workspace::GetCompositionByNodeID(nodeID).value();
            TryAppendAbstractPinMap(result, "Value", attribute->Get(project.GetCurrentTime() - parentComposition->beginFrame, parentComposition));
        }
        return result;
    }

    std::optional<AbstractAttribute> GetAttributeValue::GetCompositionAttribute() {
        auto attributeIDCandidate = GetAttribute<int>("AttributeID");
        if (attributeIDCandidate.has_value()) {
            auto& attributeID = attributeIDCandidate.value();
            auto attributeCandidate = Workspace::GetAttributeByAttributeID(attributeID);
            if (attributeCandidate.has_value()) {
                return attributeCandidate.value();
            }
        }
        auto attributeNameCandidate = GetAttribute<std::string>("AttributeName");
        if (attributeNameCandidate.has_value()) {
            std::string attributeName = attributeNameCandidate.value();
            auto parentComposition = Workspace::GetCompositionByNodeID(nodeID).value();
            return Workspace::GetAttributeByName(
                parentComposition,
                attributeName
            );
        }
        return std::nullopt;
    }

    void GetAttributeValue::AbstractLoadSerialized(Json t_data) {
        SetAttributeValue("AttributeName", t_data["AttributeName"].get<std::string>());
        SetAttributeValue("AttributeID", t_data["AttributeID"].get<int>());
    }

    Json GetAttributeValue::AbstractSerialize() {
        return {
            {"AttributeName", std::any_cast<std::string>(m_attributes["AttributeName"])},
            {"AttributeID", std::any_cast<int>(m_attributes["AttributeID"])}
        };
    }

    void GetAttributeValue::AbstractRenderProperties() {
        ImGui::Text(ICON_FA_CIRCLE_INFO " You Can Access Attribute by Name or ID");
        ImGui::Spacing();
        RenderAttributeProperty("AttributeName");
        RenderAttributeProperty("AttributeID");
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
        auto attributeCandidate = GetCompositionAttribute();
        if (attributeCandidate.has_value()) {
            return attributeCandidate.value()->name;
        }
        return base;
    }

    std::string GetAttributeValue::Icon() {
        return ICON_FA_LINK;
    }

    std::optional<std::string> GetAttributeValue::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::GetAttributeValue>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Get Attribute Value",
            .packageName = RASTER_PACKAGED "get_attribute_value",
            .category = Raster::DefaultNodeCategories::s_attributes
        };
    }
}