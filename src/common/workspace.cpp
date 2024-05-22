#include "common/common.h"

namespace Raster {

    std::vector<NodeCategory> Workspace::s_categories = {
        NodeCategory::Resources,
        NodeCategory::Utilities,
        NodeCategory::Other
    };

    std::vector<AbstractNode> Workspace::s_nodes;
    std::vector<NodeImplementation> Workspace::s_nodeImplementations;
    Configuration Workspace::s_configuration;

    void Workspace::Initialize() {
        if (!std::filesystem::exists("nodes/")) {
            std::filesystem::create_directory("nodes");
        }
        auto iterator = std::filesystem::directory_iterator("nodes");
        for (auto &entry : iterator) {
            std::string transformedPath = std::regex_replace(
                GetBaseName(entry.path().string()), std::regex(".dll|.so|lib"), "");
            Libraries::LoadLibrary("nodes", transformedPath);
            NodeImplementation implementation;
            implementation.libraryName = transformedPath;
            implementation.description = Libraries::GetFunction<NodeDescription()>(transformedPath, "GetDescription")();
            implementation.spawn = Libraries::GetFunction<AbstractNode()>(transformedPath, "SpawnNode");
            s_nodeImplementations.push_back(implementation);
            std::cout << "Loading node '" << transformedPath << std::endl;
        }
    }

    void Workspace::AddNode(std::string t_nodeName) {
        auto node = InstantiateNode(t_nodeName);
        if (node.has_value()) {
            s_nodes.push_back(node.value());
        }
    }

    std::optional<AbstractNode> Workspace::InstantiateNode(std::string t_nodeName) {
        std::optional<NodeImplementation> nodeImplementation;
        for (auto& impl : s_nodeImplementations) {
            if (impl.libraryName == t_nodeName) {
                nodeImplementation = impl;
                break;
            }
        }
        if (nodeImplementation.has_value()) {
            return PopulateNode(nodeImplementation.value().spawn());
        }
        return std::nullopt;
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

    std::optional<GenericPin> Workspace::GetPinByLinkID(int linkID) {
        for (auto& node : s_nodes) {
            if (node->flowInputPin.has_value()) {
                if (node->flowInputPin.value().linkID == linkID) {
                    return node->flowInputPin.value();
                }
            }
            if (node->flowOutputPin.has_value()) {
                if (node->flowOutputPin.value().linkID== linkID) {
                    return node->flowOutputPin.value();
                }
            }
            for (auto& inputPin : node->inputPins) {
                if (inputPin.linkID == linkID) return inputPin;
            }
            for (auto& outputPin : node->outputPins) {
                if (outputPin.linkID == linkID) return outputPin;
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