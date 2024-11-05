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

    AbstractPinMap GetAttributeValue::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto attributeCandidate = GetCompositionAttribute(t_contextData);
        if (attributeCandidate.has_value()) {
            auto& attribute = attributeCandidate.value();
            auto& project = Workspace::s_project.value();
            auto parentComposition = Workspace::GetCompositionByNodeID(nodeID).value();
            TryAppendAbstractPinMap(result, "Value", attribute->Get(project.GetCorrectCurrentTime() - parentComposition->beginFrame, parentComposition));
        }
        return result;
    }

    std::optional<AbstractAttribute> GetAttributeValue::GetCompositionAttribute(ContextData& t_contextData) {
        auto attributeIDCandidate = GetAttribute<int>("AttributeID", t_contextData);
        if (attributeIDCandidate.has_value()) {
            auto& attributeID = attributeIDCandidate.value();
            m_lastAttributeID = attributeID;
            auto attributeCandidate = Workspace::GetAttributeByAttributeID(attributeID);
            if (attributeCandidate.has_value()) {
                return attributeCandidate.value();
            }
        }
        auto attributeNameCandidate = GetAttribute<std::string>("AttributeName", t_contextData);
        if (attributeNameCandidate.has_value()) {
            std::string attributeName = attributeNameCandidate.value();
            auto parentComposition = Workspace::GetCompositionByNodeID(nodeID).value();
            auto attributeCandidate = Workspace::GetAttributeByName(
                parentComposition,
                attributeName
            );
            if (attributeCandidate.has_value()) {
                m_lastAttributeID = attributeCandidate.value()->id;
            }
            return attributeCandidate;
        }
        return std::nullopt;
    }

    void GetAttributeValue::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json GetAttributeValue::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void GetAttributeValue::AbstractRenderProperties() {
        ImGui::Text(ICON_FA_CIRCLE_INFO " You Can Access Attribute by Name or ID");
        ImGui::Spacing();
        RenderAttributeProperty("AttributeName");
        RenderAttributeProperty("AttributeID");
    }

    bool GetAttributeValue::AbstractDetailsAvailable() {
        if (m_lastAttributeID.has_value()) {
            if (Workspace::GetAttributeByAttributeID(m_lastAttributeID.value()).has_value()) return true;
        }
        return false;
    }

    void GetAttributeValue::AbstractRenderDetails() {
        if (m_lastAttributeID.has_value()) {
            auto attributeCandidate = Workspace::GetAttributeByAttributeID(m_lastAttributeID.value());
            if (attributeCandidate.has_value()) {
                auto& attribute = attributeCandidate.value();
                attribute->AbstractRenderDetails();
            }
        }
    }

    std::string GetAttributeValue::AbstractHeader() {
        std::string base = "Get Attribute Value";
        if (m_lastAttributeID.has_value()) {
            auto attributeCandidate = Workspace::GetAttributeByAttributeID(m_lastAttributeID.value());
            if (attributeCandidate.has_value()) {
                return attributeCandidate.value()->name;
            }
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