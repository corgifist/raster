#include "common/common.h"
#include "gpu/gpu.h"
#include "app/app.h"
#include "font/font.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"

namespace Raster {

    PropertyDispatchersCollection NodeBase::s_dispatchers = {
        {ATTRIBUTE_TYPE(std::string), NodeBase::DispatchStringAttribute}
    };

    StringDispatchersCollection NodeBase::s_stringDispatchers = {
        {ATTRIBUTE_TYPE(std::string), NodeBase::DispatchStringValue},
        {ATTRIBUTE_TYPE(Texture), NodeBase::DispatchTextureValue}
    };

    static ImVec2 FitRectInRect(ImVec2 screen, ImVec2 rectangle) {
        ImVec2 dst = screen;
        ImVec2 src = rectangle;
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return ImVec2{src.x * scale, src.y * scale};
    }

    void NodeBase::DispatchValueAttribute(std::any& t_attribute) {
        for (auto& dispatcher : s_stringDispatchers) {
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

    template <typename T>
    std::optional<T> NodeBase::GetAttribute(std::string t_attribute) {
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
            if (dynamicAttribute.type() == typeid(T)) {
                m_attributesCache[t_attribute] = dynamicAttribute;
                return std::optional(std::any_cast<T>(dynamicAttribute));
            }
        }
        auto targetNode = Workspace::GetNodeByPinID(attributePin.connectedPinID);
        if (targetNode.has_value() && targetNode.value()->enabled) {
            auto dynamicAttribute = targetNode.value()->AbstractExecute()[attributePin.connectedPinID];
            if (dynamicAttribute.type() == typeid(T)) {
                m_attributesCache[t_attribute] = dynamicAttribute;
                return std::any_cast<T>(dynamicAttribute);
            }
        }

        if (m_attributes.find(t_attribute) != m_attributes.end()) {
            auto dynamicAttribute = m_attributes[t_attribute];
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
                for (auto& dispatcher : s_dispatchers) {
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

    // Attribute Dispatchers

    void NodeBase::DispatchStringAttribute(NodeBase* t_owner, std::string t_attrbute, std::any& t_value, bool t_isAttributeExposed) {
        std::string string = std::any_cast<std::string>(t_value);
        if (t_isAttributeExposed) {
            float originalCursorX = ImGui::GetCursorPosX();
            ImGui::SetCursorPosX(originalCursorX - ImGui::CalcTextSize(ICON_FA_LINK).x - 5);
            ImGui::Text("%s ", ICON_FA_LINK);
            ImGui::SameLine();
            ImGui::SetCursorPosX(originalCursorX);
        }
        ImGui::Text("%s", t_attrbute.c_str());
        ImGui::SameLine();
        ImGui::InputText(FormatString("##%s", t_attrbute.c_str()).c_str(), &string, t_isAttributeExposed ? ImGuiInputTextFlags_ReadOnly : 0);
        t_value = string;
    }

    // Value Dispatchers

    void NodeBase::DispatchStringValue(std::any& t_attribute) {
        ImGui::Text("%s %s: '%s'", ICON_FA_QUOTE_LEFT, Localization::GetString("VALUE").c_str(), std::any_cast<std::string>(t_attribute).c_str());
    }

    void NodeBase::DispatchTextureValue(std::any& t_attribute) {
        auto texture = std::any_cast<Texture>(t_attribute);
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - 64);
        ImGui::Image(texture.handle, FitRectInRect(ImVec2(128, 128), ImVec2(texture.width, texture.height)));
        
        auto footerText = FormatString("%ix%i; %s", (int) texture.width, (int) texture.height, texture.PrecisionToString().c_str());
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(footerText.c_str()).x / 2.0f);
        ImGui::Text(footerText.c_str());
    }

    INSTANTIATE_ATTRIBUTE_TEMPLATE(std::string);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(Texture);
};