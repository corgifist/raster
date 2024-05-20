#include "common/common.h"

namespace Raster {
        AbstractPinMap NodeBase::Execute(AbstractPinMap t_accumulator) {
        this->m_accumulator = t_accumulator;
        auto pinMap = AbstractExecute(t_accumulator);
        auto outputPin = flowOutputPin.value_or(GenericPin());
        if (outputPin.connectedPinID > 0) {
            auto connectedNode = Workspace::GetNodeByPinID(outputPin.connectedPinID);
            if (connectedNode.has_value()) {
                return connectedNode.value()->Execute(pinMap);
            }
        }
        return pinMap;
    }

    template <typename T>
    std::optional<T> NodeBase::GetAttribute(std::string t_attribute) {
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
                return std::optional(std::any_cast<T>(dynamicAttribute));
            }
        }
        auto targetNode = Workspace::GetNodeByPinID(attributePin.connectedPinID);
        if (targetNode.has_value()) {
            auto dynamicAttribute = targetNode.value()->AbstractExecute()[attributePin.connectedPinID];
            if (dynamicAttribute.type() == typeid(T)) {
                return std::any_cast<T>(dynamicAttribute);
            }
        }

        return std::nullopt;
    }

    void NodeBase::GenerateFlowPins() {
        this->flowInputPin = GenericPin("", PinType::Input);
        this->flowOutputPin = GenericPin("", PinType::Output);
    }

    INSTANTIATE_ATTRIBUTE_TEMPLATE(std::string);
};