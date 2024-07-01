#include "common/common.h"
#include "gpu/gpu.h"
#include "app/app.h"
#include "font/font.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"

namespace Raster {

    void NodeBase::DispatchValueAttribute(std::any& t_attribute) {
        for (auto& dispatcher : Dispatchers::s_stringDispatchers) {
            if (std::type_index(t_attribute.type()) == dispatcher.first) {
                dispatcher.second(t_attribute);
            } 
        }
    }

    void NodeBase::Initialize() {
        this->enabled = true;
        this->bypassed = false;
    }

    AbstractPinMap NodeBase::Execute(AbstractPinMap t_accumulator) {
        this->m_accumulator = t_accumulator;
        this->m_attributesCache = {};
        if (!enabled) return {};
        if (bypassed) {
            auto outputPin = flowOutputPin.value();
            if (outputPin.connectedPinID > 0) {
                auto connectedNode = Workspace::GetNodeByPinID(outputPin.connectedPinID);
                if (connectedNode.has_value()) {
                    connectedNode.value()->Execute({});
                    return {};
                }
            }
            return {};
        }
        Workspace::UpdatePinCache(t_accumulator);
        auto pinMap = AbstractExecute(t_accumulator);
        Workspace::UpdatePinCache(pinMap);
        auto outputPin = flowOutputPin.value_or(GenericPin());
        if (outputPin.connectedPinID > 0) {
            auto connectedNode = Workspace::GetNodeByPinID(outputPin.connectedPinID);
            if (connectedNode.has_value() && connectedNode.value()->enabled) {
                auto newPinMap = connectedNode.value()->Execute(pinMap);
                Workspace::UpdatePinCache(newPinMap);
                return newPinMap;
            }
        }
        return pinMap;
    }

    std::string NodeBase::Header() {
        if (!overridenHeader.empty()) {
            return overridenHeader;
        }
        return AbstractHeader();
    }

    Json NodeBase::Serialize() {
        Json data = {};
        data["NodeID"] = nodeID;
        if (flowInputPin.has_value()) {
            data["FlowInputPin"] = flowInputPin.value().Serialize();
        }
        if (flowOutputPin.has_value()) {
            data["FlowOutputPin"] = flowOutputPin.value().Serialize();
        }
        data["InputPins"] = {};
        for (auto& pin : inputPins) {
            data["InputPins"].push_back(pin.Serialize());
        }
        data["OutputPins"] = {};
        for (auto& pin : outputPins) {
            data["OutputPins"].push_back(pin.Serialize());
        }

        data["NodeData"] = AbstractSerialize();
        auto nodeImplementation = Workspace::GetNodeImplementationByLibraryName(this->libraryName);
        if (nodeImplementation.has_value()) {
            data["PackageName"] = nodeImplementation.value().description.packageName;
        }

        data["Enabled"] = enabled;
        data["Bypassed"] = bypassed;

        return data;
    }

    std::optional<std::any> NodeBase::GetDynamicAttribute(std::string t_attribute) {
        if (!enabled || bypassed) return std::nullopt;
        auto attributePin = GenericPin();
        for (auto& pin : inputPins) {
            if (pin.linkedAttribute == t_attribute) {
                attributePin = pin;
                break;
            }
        }
        if (this->m_accumulator.find(attributePin.connectedPinID) != this->m_accumulator.end()) {
            auto dynamicAttribute = this->m_accumulator.at(attributePin.connectedPinID);
            return dynamicAttribute;
        }
        auto targetNode = Workspace::GetNodeByPinID(attributePin.connectedPinID);
        if (targetNode.has_value() && targetNode.value()->enabled) {
            auto dynamicAttribute = targetNode.value()->AbstractExecute()[attributePin.connectedPinID];
            return dynamicAttribute;
        }

        if (m_attributes.find(t_attribute) != m_attributes.end()) {
            auto dynamicAttribute = m_attributes[t_attribute];
            return dynamicAttribute;
        }

        return std::nullopt;
    }

    template<typename T>
    std::optional<T> NodeBase::GetAttribute(std::string t_attribute) {
        auto dynamicAttributeCandidate = GetDynamicAttribute(t_attribute);
        if (dynamicAttributeCandidate.has_value()) {
            auto& dynamicAttribute = dynamicAttributeCandidate.value();
            if (dynamicAttribute.type() == typeid(T)) {
                return std::any_cast<T>(dynamicAttribute);
            }
        }
        return std::nullopt;
    }

    void NodeBase::RenderAttributeProperty(std::string t_attribute) {
        static std::any placeholder = nullptr;
        std::any& dynamicCandidate = placeholder;
        bool candidateWasFound = false;
        if (m_attributesCache.find(t_attribute) != m_attributesCache.end()) {
            dynamicCandidate = m_attributesCache[t_attribute];
            candidateWasFound = true;
        }
        if (m_attributes.find(t_attribute) != m_attributes.end() && !candidateWasFound) {
            dynamicCandidate = m_attributes[t_attribute];
            candidateWasFound = true;
        }
        bool isAttributeExposed = false;
        if (candidateWasFound) {
            for (auto& pin : inputPins) {
                if (pin.linkedAttribute == t_attribute) {
                    isAttributeExposed = true;
                }
            }
            std::string exposeButtonTest = 
                !isAttributeExposed ? FormatString("%s %s", ICON_FA_LINK, Localization::GetString("EXPOSE").c_str()) :
                                     FormatString("%s %s", ICON_FA_LINK_SLASH, Localization::GetString("HIDE").c_str());
            ImGui::PushFont(Font::s_denseFont);
                ImGui::SetWindowFontScale(1.2f);
                    bool attributeTreeExpanded = ImGui::TreeNodeEx(FormatString("%s %s", ICON_FA_LIST, t_attribute.c_str()).c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SetWindowFontScale(1.0f);
            ImGui::PopFont();
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button) * ImVec4(isAttributeExposed ? 1.2f : 1.0f));
            if (ImGui::Button(exposeButtonTest.c_str())) {
                if (isAttributeExposed) {
                    int attributeIndex = 0;
                    for (auto& pin : inputPins) {
                        if (pin.linkedAttribute == t_attribute) {
                            break;
                        }
                        attributeIndex++;
                    }
                    inputPins.erase(inputPins.begin() + attributeIndex);
                } else {
                    AddInputPin(t_attribute);
                }
            }
            ImGui::PopStyleColor();
            if (attributeTreeExpanded) {
                bool dispatcherWasFound = false;
                for (auto& dispatcher : Dispatchers::s_propertyDispatchers) {
                    if (std::type_index(dynamicCandidate.type()) == dispatcher.first) {
                        dispatcherWasFound = true;
                        dispatcher.second(this, t_attribute, dynamicCandidate, isAttributeExposed);
                        m_attributes[t_attribute] = dynamicCandidate;
                    } 
                }
                if (!dispatcherWasFound) {
                    ImGui::Text("%s %s '%s'", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("NO_DISPATCHER_WAS_FOUND").c_str(), dynamicCandidate.type().name());
                }
                ImGui::TreePop();
            }
        }
    }

    void NodeBase::RenderDetails() {
        AbstractRenderDetails();
    }

    bool NodeBase::DetailsAvailable() {
        return AbstractDetailsAvailable();
    }

    std::set<std::string> NodeBase::GetAttributesList() {
        std::set<std::string> result;
        for (auto& attribute : m_attributes) {
            result.insert(attribute.first);
        }
        return result;
    }

    void NodeBase::GenerateFlowPins() {
        this->flowInputPin = GenericPin("", PinType::Input, true);
        this->flowOutputPin = GenericPin("", PinType::Output, true);
    }

    std::optional<GenericPin> NodeBase::GetAttributePin(std::string t_attribute) {
        for (auto& pin : inputPins) {
            if (pin.linkedAttribute == t_attribute) return pin;
        }

        for (auto& pin : outputPins) {
            if (pin.linkedAttribute == t_attribute) return pin;
        }

        return std::nullopt;
    }

    void NodeBase::TryAppendAbstractPinMap(AbstractPinMap& t_map, std::string t_attribute, std::any t_value) {
        auto targetPin = GetAttributePin(t_attribute);
        if (targetPin.has_value()) {
            auto pin = targetPin.value();
            t_map[pin.pinID] = t_value;
        }
    }

    void NodeBase::AddInputPin(std::string t_attribute) {
        this->inputPins.push_back(GenericPin(t_attribute, PinType::Input));
    }

    void NodeBase::AddOutputPin(std::string t_attribute) {
        this->outputPins.push_back(GenericPin(t_attribute, PinType::Output));
    }

    INSTANTIATE_ATTRIBUTE_TEMPLATE(std::string);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(float);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(std::any);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(Texture);
};