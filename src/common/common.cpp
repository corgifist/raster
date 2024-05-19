#include "common/common.h"

namespace Raster {
    std::random_device Randomizer::s_random_device;
    std::mt19937 Randomizer::s_random(Randomizer::s_random_device());
    std::uniform_int_distribution<std::mt19937::result_type> Randomizer::s_distribution(1, INT32_MAX - 50);

    int Randomizer::GetRandomInteger() {
        return std::abs(int(s_distribution(s_random)));
    }

    GenericPin::GenericPin(std::string t_linkedAttribute, PinType t_type) {
        this->linkID = Randomizer::GetRandomInteger();
        this->pinID = Randomizer::GetRandomInteger();
        this->connectedPinID = -1;
        this->linkedAttribute = t_linkedAttribute;
        this->type = t_type;
    }

    GenericPin::GenericPin() {
        this->linkID = this->pinID = this->connectedPinID = -1;
    }

    AbstractPinMap NodeBase::Execute(AbstractPinMap t_accumulator) {
        this->m_accumulator = t_accumulator;
        auto pinMap = AbstractExecute(t_accumulator);
        auto outputPin = flowOutputPin.value_or(GenericPin());
        if (outputPin.connectedPinID > 0) {
            auto connectedNode = Workspace::GetNodeByPinID(outputPin.connectedPinID);
            if (connectedNode.has_value()) {
                return connectedNode.value()->AbstractExecute(pinMap);
            }
        }
        return pinMap;
    }

    std::optional<std::string> NodeBase::GetStringAttribute(std::string t_attribute) {
        auto attributePin = GenericPin();
        for (auto& pin : inputPins) {
            if (pin.linkedAttribute == t_attribute) {
                attributePin = pin;
                break;
            }
        }
        if (this->m_accumulator.find(attributePin.connectedPinID) != this->m_accumulator.end()) {
            auto dynamicAttribute = this->m_accumulator.at(attributePin.connectedPinID);
            if (dynamicAttribute.type() == typeid(std::string)) {
                return std::optional(std::any_cast<std::string>(dynamicAttribute));
            }
        }
        auto targetNode = Workspace::GetNodeByPinID(attributePin.connectedPinID);
        if (targetNode.has_value()) {
            auto dynamicAttribute = targetNode.value()->Execute()[attributePin.connectedPinID];
            if (dynamicAttribute.type() == typeid(std::string)) {
                return std::any_cast<std::string>(dynamicAttribute);
            }
        }

        return std::nullopt;
    }

    void NodeBase::GenerateFlowPins() {
        this->flowInputPin = GenericPin("", PinType::Input);
        this->flowOutputPin = GenericPin("", PinType::Output);
    }

    std::unordered_map<std::string, internalDylib> Libraries::s_registry;

    std::vector<AbstractNode> Workspace::s_nodes;
    std::vector<std::string> Workspace::s_initializedNodes;

    void Workspace::Initialize() {
        if (!std::filesystem::exists("nodes/")) {
            std::filesystem::create_directory("nodes");
        }
        auto iterator = std::filesystem::directory_iterator("nodes");
        for (auto &entry : iterator) {
            std::string transformedPath = std::regex_replace(
                GetBaseName(entry.path().string()), std::regex(".dll|.so|lib"), "");
            Libraries::LoadLibrary("nodes", transformedPath);
            s_initializedNodes.push_back(transformedPath);
            std::cout << transformedPath << std::endl;
        }
    }

    std::optional<AbstractNode> Workspace::InstantiateNode(std::string t_nodeName) {
        try {
            return std::optional<AbstractNode>(PopulateNode(Libraries::GetFunction<AbstractNode()>(t_nodeName, "SpawnNode")()));
        } catch (...) {
            return std::nullopt;
        }
    }

    AbstractNode Workspace::PopulateNode(AbstractNode node) {
        node->nodeID = Randomizer::GetRandomInteger();
        return node;
    }

    std::optional<AbstractNode> Workspace::GetNodeByPinID(int pinID) {
        for (auto& node : s_nodes) {
            if (node->flowInputPin.has_value()) {
                if (node->flowInputPin.value().pinID == pinID) {
                    return std::optional{node};
                }
            }
            if (node->flowOutputPin.has_value()) {
                if (node->flowOutputPin.value().pinID == pinID) {
                    return std::optional{node};
                }
            }
            for (auto& inputPin : node->inputPins) {
                if (inputPin.pinID == pinID) return std::optional{node};
            }
            for (auto& outputPin : node->outputPins) {
                if (outputPin.pinID == pinID) return std::optional{node};
            }
        }
        return std::nullopt;
    }

    std::optional<GenericPin> Workspace::GetPinByPinID(int pinID) {
        for (auto& node : s_nodes) {
            if (node->flowInputPin.has_value()) {
                if (node->flowInputPin.value().pinID == pinID) {
                    return node->flowInputPin;
                }
            }
            if (node->flowOutputPin.has_value()) {
                if (node->flowOutputPin.value().pinID == pinID) {
                    return node->flowOutputPin;
                }
            }
            for (auto& inputPin : node->inputPins) {
                if (inputPin.pinID == pinID) return std::optional{inputPin};
            }
            for (auto& outputPin : node->outputPins) {
                if (outputPin.pinID == pinID) return std::optional{outputPin};
            }
        }
        return std::nullopt;
    }

    void Workspace::UpdatePinByID(GenericPin pin, int pinID) {
        for (auto& node : s_nodes) {
            if (node->flowInputPin.has_value()) {
                if (node->flowInputPin.value().pinID == pinID) {
                    node->flowInputPin = pin;
                }
            }
            if (node->flowOutputPin.has_value()) {
                if (node->flowOutputPin.value().pinID == pinID) {
                    node->flowOutputPin = pin;
                }
            }
            for (auto& inputPin : node->inputPins) {
                if (inputPin.pinID == pinID) {
                    inputPin = pin;
                }
            }
            for (auto& outputPin : node->outputPins) {
                if (outputPin.pinID == pinID) {
                    outputPin = pin;
                }
            }
        }
    }

}