#include "common/common.h"
#include "common/transform2d.h"
#include "gpu/gpu.h"
#include "app/app.h"
#include "font/font.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"
#include "common/dispatchers.h"
#include "common/audio_samples.h"

#define TYPE_NAME(icon, type) icon " " #type
namespace Raster {

    static std::vector<std::pair<std::string, std::any>> s_defaultValues = {
        {TYPE_NAME(ICON_FA_DIVIDE, int), 0},
        {TYPE_NAME(ICON_FA_DIVIDE, float), 0.0f},
        {TYPE_NAME(ICON_FA_QUOTE_LEFT, std::string), std::string("")},
        {TYPE_NAME(ICON_FA_LEFT_RIGHT, glm::vec2), glm::vec2()},
        {TYPE_NAME(ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER, glm::vec3), glm::vec3()},
        {TYPE_NAME(ICON_FA_EXPAND, glm::vec4), glm::vec4()},
        {TYPE_NAME(ICON_FA_UP_DOWN_LEFT_RIGHT, Transform2D), Transform2D()},
        {TYPE_NAME(ICON_FA_IMAGE, SamplerSettings), SamplerSettings()}
    };

    void NodeBase::SetAttributeValue(std::string t_attribute, std::any t_value) {
        this->m_attributes[t_attribute] = t_value;
    }

    void NodeBase::SetupAttribute(std::string t_attribute, std::any t_value) {
        SetAttributeValue(t_attribute, t_value);
        this->m_attributesOrder.push_back(t_attribute);
    }

    void NodeBase::Initialize() {
        this->enabled = true;
        this->bypassed = false;
        this->executionsPerFrame = 0;
        this->m_contextMutex = std::make_unique<std::mutex>();
        this->m_attributesCacheMutex = std::make_unique<std::mutex>();
    }

    AbstractPinMap NodeBase::Execute(AbstractPinMap t_accumulator, ContextData t_contextData) {
        this->m_accumulator = t_accumulator;
        if (!enabled) return {};
        MergeContextDatas(t_contextData);
        if (bypassed) {
            auto outputPin = flowOutputPin.value();
            if (outputPin.connectedPinID > 0) {
                auto connectedNode = Workspace::GetNodeByPinID(outputPin.connectedPinID);
                if (connectedNode.has_value()) {
                    connectedNode.value()->Execute({}, t_contextData);
                    return {};
                }
            }
            return {};
        }
        Workspace::UpdatePinCache(t_accumulator);
        executionsPerFrame++;
        auto pinMap = AbstractExecute(t_accumulator);
        Workspace::UpdatePinCache(pinMap);
        auto outputPin = flowOutputPin.value_or(GenericPin());
        if (outputPin.connectedPinID > 0) {
            auto connectedNode = Workspace::GetNodeByPinID(outputPin.connectedPinID);
            if (connectedNode.has_value() && connectedNode.value()->enabled) {
                auto newPinMap = connectedNode.value()->Execute(pinMap, t_contextData);
                Workspace::UpdatePinCache(newPinMap);
                return newPinMap;
            }
        }
        return pinMap;
    }

    void NodeBase::RenderAttributeProperty(std::string t_attribute) {
        static std::any placeholder = nullptr;
        std::any& dynamicCandidate = placeholder;
        bool candidateWasFound = false;
        bool usingCachedAttribute = false;
        m_attributesCacheMutex->lock();
        if (m_attributesCache.find(t_attribute) != m_attributesCache.end()) {
            dynamicCandidate = m_attributesCache[t_attribute];
            candidateWasFound = true;
            usingCachedAttribute = true;
        }
        m_attributesCacheMutex->unlock();
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
            std::string typeText = FormatString("%s %s: %s", ICON_FA_CIRCLE_INFO, Localization::GetString("VALUE_TYPE").c_str(), Workspace::GetTypeName(dynamicCandidate).c_str());
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
            ImGui::SameLine();
            std::string valueTypePopupID = FormatString("##valueTypePopup%s%i", t_attribute.c_str(), nodeID);
            if (ImGui::Button(typeText.c_str())) {
                ImGui::OpenPopup(valueTypePopupID.c_str());
            }
            if (ImGui::BeginPopup(valueTypePopupID.c_str())) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_CIRCLE_INFO, Localization::GetString("VALUE_TYPE").c_str()).c_str());
                for (auto& defaultValue : s_defaultValues) {
                    if (ImGui::MenuItem(FormatString("%s", defaultValue.first.c_str()).c_str())) {
                        m_attributes[t_attribute] = defaultValue.second;
                        dynamicCandidate = defaultValue.second;
                    }
                    if (ImGui::BeginItemTooltip()) {
                        Dispatchers::DispatchString(defaultValue.second);
                        ImGui::EndTooltip();
                    }
                }
                ImGui::EndPopup();
            }
            if (attributeTreeExpanded) {
                bool dispatcherWasFound = false;
                for (auto& dispatcher : Dispatchers::s_propertyDispatchers) {
                    if (std::type_index(dynamicCandidate.type()) == dispatcher.first) {
                        if (isAttributeExposed) {
                            float originalCursorX = ImGui::GetCursorPosX();
                            ImGui::SetCursorPosX(originalCursorX - ImGui::CalcTextSize(ICON_FA_LINK).x - 5);
                            ImGui::Text("%s ", ICON_FA_LINK);
                            ImGui::SameLine();
                            ImGui::SetCursorPosX(originalCursorX);
                        }
                        dispatcherWasFound = true;
                        dispatcher.second(this, t_attribute, dynamicCandidate, isAttributeExposed);
                        if (!usingCachedAttribute) m_attributes[t_attribute] = dynamicCandidate;
                    } 
                }
                if (!dispatcherWasFound) {
                    ImGui::Text("%s %s '%s'", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("NO_DISPATCHER_WAS_FOUND").c_str(), dynamicCandidate.type().name());
                }
                ImGui::TreePop();
            }
        }
    }

    void NodeBase::SerializeAttribute(Json& t_data, std::string t_attribute) {
        if (m_attributes.find(t_attribute) != m_attributes.end()) {
            auto& attribute = m_attributes[t_attribute];
            auto serializedAttribute = DynamicSerialization::Serialize(attribute);
            if (serializedAttribute.has_value()) {
                t_data[t_attribute] = serializedAttribute.value();
            }
        }
    }

    Json NodeBase::SerializeAttributes(std::vector<std::string> t_attributes) {
        Json result;
        for (auto& attribute : t_attributes) {
            SerializeAttribute(result, attribute);
        }
        return result;
    }

    void NodeBase::DeserializeAttribute(Json& t_data, std::string t_attribute) {
        if (t_data.contains(t_attribute)) {
            auto serializedAttribute = t_data[t_attribute];
            auto deserializedAttribute = DynamicSerialization::Deserialize(serializedAttribute);
            if (deserializedAttribute.has_value()) {
                m_attributes[t_attribute] = deserializedAttribute.value();
            }
        }
    }

    Json NodeBase::SerializeAllAttributes() {
        Json result;
        for (auto& attribute : m_attributes) {
            SerializeAttribute(result, attribute.first);
        }
        return result;
    }

    void NodeBase::DeserializeAllAttributes(Json& t_data) {
        for (auto [attribute, serializedData] : t_data.items()) {
            DeserializeAttribute(t_data, attribute);
        }
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
        auto attributePinCandidate = GetAttributePin(t_attribute);
        auto attributePin = attributePinCandidate.has_value() ? attributePinCandidate.value() : GenericPin();
        std::string exposedPinAttributeName = FormatString("<%i>.%s", nodeID, t_attribute.c_str());
        auto compositionCandidate = Workspace::GetCompositionByNodeID(nodeID);
        if (compositionCandidate.has_value()) {
            auto& project = Workspace::GetProject();
            auto& composition = compositionCandidate.value();
            for (auto& attribute : composition->attributes) {
                if (attribute->internalAttributeName.find(exposedPinAttributeName) != std::string::npos) {
                    auto attributeValue = attribute->Get(project.GetCorrectCurrentTime() - composition->beginFrame, composition);
                    m_attributesCacheMutex->lock();
                    m_attributesCache[t_attribute] = attributeValue;
                    m_attributesCacheMutex->unlock();
                    return attributeValue;
                }
            }
        }

        auto targetNode = Workspace::GetNodeByPinID(attributePin.connectedPinID);
        if (targetNode.has_value() && targetNode.value()->enabled) {
            targetNode.value()->MergeContextDatas(GetContextData());
            auto pinMap = targetNode.value()->AbstractExecute();
            Workspace::UpdatePinCache(pinMap);
            auto dynamicAttribute = pinMap[attributePin.connectedPinID];
            targetNode.value()->executionsPerFrame++;
            m_attributesCacheMutex->lock();
            m_attributesCache[t_attribute] = dynamicAttribute;
            m_attributesCacheMutex->unlock();
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
            if (typeid(T) == typeid(glm::vec4) && dynamicAttribute.type() == typeid(glm::vec3)) {
                glm::vec3 vec3 = std::any_cast<glm::vec3>(dynamicAttribute);
                glm::vec4 vec4 = glm::vec4(vec3, 1.0f);
                dynamicAttribute = vec4;
            }
            if (typeid(T) == typeid(float) && dynamicAttribute.type() == typeid(int)) {
                dynamicAttribute = (float) std::any_cast<int>(dynamicAttribute);
            }
            if (typeid(T) == typeid(int) && dynamicAttribute.type() == typeid(float)) {
                dynamicAttribute = (int) std::any_cast<float>(dynamicAttribute);
            }
            if (dynamicAttribute.type() == typeid(T)) {
                return std::any_cast<T>(dynamicAttribute);
            }
        }
        return std::nullopt;
    }

    void NodeBase::RenderDetails() {
        AbstractRenderDetails();
    }

    bool NodeBase::DetailsAvailable() {
        return AbstractDetailsAvailable();
    }

    void NodeBase::ClearAttributesCache() {
        m_attributesCacheMutex->lock();
            this->m_attributesCache.clear();
            this->m_accumulator.clear();
        m_attributesCacheMutex->unlock();
    }

    std::vector<std::string> NodeBase::GetAttributesList() {
        return m_attributesOrder;
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
        for (auto& pin : inputPins) {
            if (pin.linkedAttribute == t_attribute) return;
        }
        this->inputPins.push_back(GenericPin(t_attribute, PinType::Input));
    }

    void NodeBase::AddOutputPin(std::string t_attribute) {
        for (auto& pin : outputPins) {
            if (pin.linkedAttribute == t_attribute) return;
        }
        this->outputPins.push_back(GenericPin(t_attribute, PinType::Output));
    }

    ContextData NodeBase::GetContextData() {
        m_contextMutex->lock();
            auto threadID = std::this_thread::get_id();
            if (m_contextDatas.find(threadID) == m_contextDatas.end()) {
                m_contextDatas[threadID] = {};
            }
            auto contextData = m_contextDatas[threadID];
        m_contextMutex->unlock();
        return contextData;
    }

    void NodeBase::UpdateContextData(std::string t_key, std::any t_value) {
        auto oldContextData = GetContextData();
        oldContextData[t_key] = t_value;
        m_contextMutex->lock();
            m_contextDatas[std::this_thread::get_id()] = oldContextData;
        m_contextMutex->unlock();
    }

    void NodeBase::MergeContextDatas(ContextData t_data) {
        auto currentContextData = GetContextData();
        auto id = std::this_thread::get_id();
        m_contextMutex->lock();
            for (auto& pair : t_data) {
                m_contextDatas[id][pair.first] = pair.second;
            }
        m_contextMutex->unlock();
    }

    bool NodeBase::DoesAudioMixing() {
        return AbstractDoesAudioMixing();
    }

    void NodeBase::OnTimelineSeek() {
        return AbstractOnTimelineSeek();
    }

    std::optional<float> NodeBase::GetContentDuration() {
        return AbstractGetContentDuration();
    }

    INSTANTIATE_ATTRIBUTE_TEMPLATE(std::string);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(float);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(int);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(std::any);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(Texture);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(glm::vec4);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(glm::vec3);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(glm::vec2);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(Framebuffer);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(Transform2D);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(SamplerSettings);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(bool);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(AudioSamples);
};