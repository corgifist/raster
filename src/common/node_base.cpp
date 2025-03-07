#include "common/common.h"
#include "common/transform2d.h"
#include "font/IconsFontAwesome5.h"
#include "gpu/gpu.h"
#include "app/app.h"
#include "font/font.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"
#include "common/dispatchers.h"
#include "common/audio_samples.h"
#include "common/asset_id.h"
#include "common/generic_audio_decoder.h"
#include "common/generic_resolution.h"
#include "common/gradient_1d.h"
#include "raster.h"
#include "common/choice.h"
#include "common/rendering.h"
#include "common/line2d.h"

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
        {TYPE_NAME(ICON_FA_IMAGE, SamplerSettings), SamplerSettings()},
        {TYPE_NAME(ICON_FA_FOLDER_OPEN, AssetID), AssetID()},
        {TYPE_NAME(ICON_FA_VOLUME_HIGH " " ICON_FA_GEARS, GenericAudioDecoder), GenericAudioDecoder()},
        {TYPE_NAME(ICON_FA_WAVE_SQUARE, AudioSamples), AudioSamples()},
        {TYPE_NAME(ICON_FA_IMAGE " " ICON_FA_EXPAND, GenericResolution), GenericResolution()},
        {TYPE_NAME(ICON_FA_DROPLET, Gradient1D), Gradient1D()},
        {TYPE_NAME(ICON_FA_QUESTION, Choice), Choice()},
        {TYPE_NAME(ICON_FA_LINES_LEANING, Line2D), Line2D()}
    };

    NodeBase::~NodeBase() {
        m_attributes.Lock();
        auto& attributes = m_attributes.GetReference();
        for (auto& attributePair : attributes) {
            if (attributePair.second.type() == typeid(GenericAudioDecoder)) {
                std::any_cast<GenericAudioDecoder>(attributePair.second).Destroy();
            }
        }
        m_attributes.Unlock();
    }

    void NodeBase::SetAttributeValue(std::string t_attribute, std::any t_value) {
        m_attributes.Lock();
            m_attributes.GetReference()[t_attribute] = t_value;
        m_attributes.Unlock();
    }

    void NodeBase::SetupAttribute(std::string t_attribute, std::any t_value) {
        SetAttributeValue(t_attribute, t_value);
        this->m_attributesOrder.push_back(t_attribute);
    }

    void NodeBase::Initialize() {
        this->enabled = true;
        this->bypassed = false;
        this->executionsPerFrame.Set(0);
    }

    AbstractPinMap NodeBase::Execute(ContextData& t_contextData) {
        if (!enabled || !Workspace::IsProjectLoaded()) return {};
        if (bypassed) {
            auto outputPin = flowOutputPin.value();
            if (outputPin.connectedPinID > 0) {
                auto connectedNode = Workspace::GetNodeByPinID(outputPin.connectedPinID);
                if (connectedNode.has_value()) {
                    connectedNode.value()->Execute(t_contextData);
                    return {};
                }
            }
            return {};
        }
        // if (!ExecutingInAudioContext(t_contextData)) Workspace::UpdatePinCache(t_accumulator); 
        if (RASTER_GET_CONTEXT_VALUE(t_contextData, "INCREMENT_EPF", bool)) executionsPerFrame.SetBackValue(executionsPerFrame.Get() + 1); 
        auto pinMap = AbstractExecute(t_contextData);
        if (!ExecutingInAudioContext(t_contextData)) Workspace::UpdatePinCache(pinMap);
        auto outputPin = flowOutputPin.value_or(GenericPin());
        if (outputPin.connectedPinID > 0) {
            auto connectedNode = Workspace::GetNodeByPinID(outputPin.connectedPinID);
            if (connectedNode.has_value() && connectedNode.value()->enabled) {
                auto newPinMap = connectedNode.value()->Execute(t_contextData);
                if (!ExecutingInAudioContext(t_contextData)) Workspace::UpdatePinCache(newPinMap);
                return newPinMap;
            }
        }
        return pinMap;
    }

    void NodeBase::RenderAttributeProperty(std::string t_attribute, std::vector<std::any> t_metadata) {
        static std::any placeholder = nullptr;
        std::any& dynamicCandidate = placeholder;
        bool candidateWasFound = false;
        bool usingCachedAttribute = false;
        auto& frontAttributesCache = m_attributesCache.GetFrontValue();
        frontAttributesCache.Lock();
        if (frontAttributesCache.GetReference().find(t_attribute) != frontAttributesCache.GetReference().end()) {
            dynamicCandidate = frontAttributesCache.GetReference()[t_attribute];
            candidateWasFound = true;
            usingCachedAttribute = true;
        }
        frontAttributesCache.Unlock();
        m_attributes.Lock();
        auto& attributes = m_attributes.GetReference();
        if (attributes.find(t_attribute) != attributes.end() && !candidateWasFound) {
            dynamicCandidate = attributes[t_attribute];
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
                    std::string icon = ICON_FA_LIST;
                    for (auto& metadata : t_metadata) {
                        if (metadata.type() == typeid(IconMetadata)) {
                            icon = std::any_cast<IconMetadata>(metadata).icon;
                        }
                    }
                    bool attributeTreeExpanded = ImGui::TreeNodeEx(FormatString("%s %s##%s", icon.c_str(), GetAttributeName(t_attribute).c_str(), t_attribute.c_str()).c_str(), ImGuiTreeNodeFlags_DefaultOpen);
                ImGui::SetWindowFontScale(1.0f);
            ImGui::PopFont();
            ImGui::SameLine();
            static std::optional<Json> s_copiedAttributeData;
            auto serializedContentCandidate = DynamicSerialization::Serialize(dynamicCandidate);
            if (!serializedContentCandidate.has_value()) ImGui::BeginDisabled();
                if (ImGui::Button(ICON_FA_COPY)) {
                    s_copiedAttributeData = serializedContentCandidate.value();
                }
                ImGui::SetItemTooltip("%s %s", ICON_FA_COPY, Localization::GetString("COPY_ATTRIBUTE_VALUE").c_str());
            if (!serializedContentCandidate.has_value()) ImGui::EndDisabled();
            ImGui::SameLine();
            bool clipboardTextValid = s_copiedAttributeData.has_value();
            if (!clipboardTextValid) ImGui::BeginDisabled();
            if (ImGui::Button(ICON_FA_PASTE)) {
                auto deserializedValueCandidate = DynamicSerialization::Deserialize(s_copiedAttributeData.value());
                if (deserializedValueCandidate.has_value()) {
                    dynamicCandidate = deserializedValueCandidate.value();
                }
            }
            ImGui::SetItemTooltip("%s %s", ICON_FA_PASTE, Localization::GetString("PASTE_ATTRIBUTE_VALUE").c_str());
            if (!clipboardTextValid) ImGui::EndDisabled(); 
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
                        attributes[t_attribute] = defaultValue.second;
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
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("%s ", ICON_FA_LINK);
                            ImGui::SameLine();
                            ImGui::SetCursorPosX(originalCursorX);
                        }
                        dispatcherWasFound = true;
                        dispatcher.second(this, GetAttributeName(t_attribute), dynamicCandidate, isAttributeExposed, t_metadata);
                        if (!usingCachedAttribute) attributes[t_attribute] = dynamicCandidate;
                    } 
                }
                if (!dispatcherWasFound) {
                    ImGui::Text("%s %s '%s'", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("NO_DISPATCHER_WAS_FOUND").c_str(), dynamicCandidate.type().name());
                }
                ImGui::TreePop();
            }
        }
        m_attributes.Unlock();
    }

    void NodeBase::SerializeAttribute(Json& t_data, std::string t_attribute) {
        m_attributes.Lock();
        auto& attributes = m_attributes.GetReference();
        if (attributes.find(t_attribute) != attributes.end()) {
            auto& attribute = attributes[t_attribute];
            auto serializedAttribute = DynamicSerialization::Serialize(attribute);
            if (serializedAttribute.has_value()) {
                t_data[t_attribute] = serializedAttribute.value();
            }
        }
        m_attributes.Unlock();
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
                m_attributes.Lock();
                m_attributes.GetReference()[t_attribute] = deserializedAttribute.value();
                m_attributes.Unlock();
            }
        }
    }

    Json NodeBase::SerializeAllAttributes() {
        Json result;
        auto attributes = m_attributes.Get();
        for (auto& attribute : attributes) {
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

        data["NodePosition"] = nullptr;
        if (nodePosition) {
            data["NodePosition"] = {nodePosition->x, nodePosition->y};
        }

        data["Enabled"] = enabled;
        data["Bypassed"] = bypassed;
        data["OverridenHeader"] = overridenHeader;

        return data;
    }

    std::optional<std::any> NodeBase::GetDynamicAttribute(std::string t_attribute, ContextData& t_contextData) {
        if (!enabled || bypassed || !Workspace::IsProjectLoaded()) return std::nullopt;
        auto attributePinCandidate = GetAttributePin(t_attribute);
        auto attributePin = attributePinCandidate.has_value() ? attributePinCandidate.value() : GenericPin();
        std::string exposedPinAttributeName = FormatString("<%i>.%s", nodeID, t_attribute.c_str());
        auto compositionCandidate = Workspace::GetCompositionByNodeID(nodeID);
        auto& backAttributesCache = m_attributesCache.Get();
        backAttributesCache.Lock();
        if (compositionCandidate.has_value()) {
            auto& project = Workspace::GetProject();
            auto& composition = compositionCandidate.value();
            std::function<std::optional<std::any>(std::vector<AbstractAttribute>*)> evalAttribute = [&](std::vector<AbstractAttribute>* t_attributes) -> std::optional<std::any> {
                for (auto& attribute : *t_attributes) {
                    auto childAttributesCandidate = attribute->GetChildAttributes();
                    if (childAttributesCandidate) {
                        auto resultCandidate = evalAttribute(*childAttributesCandidate);
                        if (resultCandidate) return *resultCandidate;
                    }
                    if (attribute->internalAttributeName.find(exposedPinAttributeName) != std::string::npos) {
                        auto attributeValue = attribute->Get(project.GetCorrectCurrentTime() - composition->beginFrame, composition);
                        backAttributesCache.GetReference()[t_attribute] = attributeValue;
                        backAttributesCache.Unlock();
                        return attributeValue;
                    }
                }
                return std::nullopt;
            };
            auto evalCandidate = evalAttribute(&composition->attributes);
            if (evalCandidate) return *evalCandidate;
        }

        auto targetNode = Workspace::GetNodeByPinID(attributePin.connectedPinID);
        if (targetNode.has_value() && targetNode.value()->enabled) {
            auto pinMap = targetNode.value()->AbstractExecute(t_contextData);
            if (!ExecutingInAudioContext(t_contextData)) Workspace::UpdatePinCache(pinMap);
            auto dynamicAttribute = pinMap[attributePin.connectedPinID];
            if (RASTER_GET_CONTEXT_VALUE(t_contextData, "INCREMENT_EPF", bool)) targetNode.value()->executionsPerFrame.SetBackValue(targetNode.value()->executionsPerFrame.Get() + 1); 
            if (!ExecutingInAudioContext(t_contextData)) {
                backAttributesCache.GetReference()[t_attribute] = dynamicAttribute;
            }
            backAttributesCache.Unlock();
            return dynamicAttribute;
        }
        backAttributesCache.Unlock();

        if (m_attributes.Get().find(t_attribute) != m_attributes.Get().end()) {
            auto dynamicAttribute = m_attributes.Get()[t_attribute];
            return dynamicAttribute;
        }

        return std::nullopt;
    }

    template<typename T>
    std::optional<T> NodeBase::GetAttribute(std::string t_attribute, ContextData& t_contextData) {
        auto dynamicAttributeCandidate = GetDynamicAttribute(t_attribute, t_contextData);
        if (dynamicAttributeCandidate.has_value()) {
            auto& dynamicAttribute = dynamicAttributeCandidate.value();
            if (dynamicAttribute.type() != typeid(T)) {
                if (dynamicAttribute.type() == typeid(GenericAudioDecoder)) {
                    auto& project = Workspace::GetProject();
                    auto compositionCandidate = Workspace::GetCompositionByNodeID(nodeID);
                    if (!compositionCandidate) return std::nullopt;
                    auto& composition = *compositionCandidate;
                    *std::any_cast<GenericAudioDecoder>(dynamicAttribute).seekTarget = (project.currentFrame - composition->beginFrame) / project.framerate;
                    if (typeid(T) == typeid(AudioSamples)) {
                        auto decoder = std::any_cast<GenericAudioDecoder>(dynamicAttribute);
                        bool isAudioPass = RASTER_GET_CONTEXT_VALUE(t_contextData, "AUDIO_PASS", bool);
                        int audioPassID = RASTER_GET_CONTEXT_VALUE(t_contextData, "AUDIO_PASS_ID", int);
                        auto samplesCandidate = decoder.DecodeSamples(isAudioPass ? audioPassID : AudioInfo::s_audioPassID, t_contextData);
                        if (samplesCandidate.has_value()) {
                            dynamicAttribute = samplesCandidate.value();
                        }
                    }
                }
                if (RASTER_GET_CONTEXT_VALUE(t_contextData, "ALLOW_MEDIA_DECODING", bool)) {
                    auto conversionCandidate = Dispatchers::DispatchConversion(dynamicAttribute, typeid(T));
                    if (conversionCandidate.has_value()) {
                        return std::any_cast<T>(conversionCandidate.value());
                    }
                } else if (dynamicAttribute.type() == typeid(GenericAudioDecoder)) {
                    auto cachedSamplesCandidate = std::any_cast<GenericAudioDecoder>(dynamicAttribute).GetCachedSamples();
                    if (cachedSamplesCandidate.has_value()) {
                        dynamicAttribute = cachedSamplesCandidate.value();
                    } else {
                        dynamicAttribute = std::nullopt;
                    }
                }
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
        auto& backAttributesCache = m_attributesCache.Get();
        backAttributesCache.Lock();
        backAttributesCache.GetReference().clear();
        backAttributesCache.Unlock();
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

    void NodeBase::SetAttributeAlias(std::string t_attributeName, std::string t_attributeAlias) {
        m_attributeAliases[t_attributeName] = t_attributeAlias;
    }

    std::string NodeBase::GetAttributeName(std::string t_attributeName) {
        if (m_attributeAliases.find(t_attributeName) != m_attributeAliases.end()) {
            return m_attributeAliases[t_attributeName];
        }
        return t_attributeName;
    }

    bool NodeBase::DoesAudioMixing() {
        return AbstractDoesAudioMixing();
    }

    bool NodeBase::DoesRendering() {
        return AbstractDoesRendering();
    }

    void NodeBase::OnTimelineSeek() {
        m_attributes.Lock();
            for (auto& attribute : m_attributes.GetReference()) {
                if (attribute.second.type() == typeid(GenericAudioDecoder)) {
                    auto decoder = std::any_cast<GenericAudioDecoder>(attribute.second);
                    auto compositionCandidate = Workspace::GetCompositionByNodeID(nodeID);
                    if (!compositionCandidate) continue;
                    auto& composition = *compositionCandidate;
                    auto& project = Workspace::GetProject();
                    decoder.Seek((project.currentFrame - composition->beginFrame) / project.framerate);
                    attribute.second = decoder;
                }
            }
        m_attributes.Unlock();
        AbstractOnTimelineSeek();
    }

    std::vector<int> NodeBase::GetUsedAudioBuses() {
        return AbstractGetUsedAudioBuses();
    }

    std::optional<float> NodeBase::GetContentDuration() {
        return AbstractGetContentDuration();
    }

    bool NodeBase::ExecutingInAudioContext(ContextData& t_data) {
        return t_data.find("AUDIO_PASS") != t_data.end();
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
    INSTANTIATE_ATTRIBUTE_TEMPLATE(Gradient1D);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(Choice);
    INSTANTIATE_ATTRIBUTE_TEMPLATE(Line2D);
};