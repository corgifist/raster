#include "common/common.h"
#include "gpu/gpu.h"

namespace Raster {

    std::vector<NodeCategory> Workspace::s_categories = {
        NodeCategory::Resources,
        NodeCategory::Utilities,
        NodeCategory::Other
    };

    std::vector<AbstractNode> Workspace::s_nodes;
    std::vector<NodeImplementation> Workspace::s_nodeImplementations;
    Configuration Workspace::s_configuration;

    std::unordered_map<int, std::any> Workspace::s_pinCache;

    std::vector<int> Workspace::s_selectedNodes;
    std::vector<int> Workspace::s_targetSelectNodes;

    std::unordered_map<std::type_index, uint32_t> Workspace::s_typeColors = {
        {ATTRIBUTE_TYPE(std::string), RASTER_COLOR32(204, 0, 103, 255)},
        {ATTRIBUTE_TYPE(Texture), RASTER_COLOR32(0, 102, 255, 255)}
    };

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
            std::cout << "Loading node '" << transformedPath << "'" << std::endl;
        }
    }

    void Workspace::UpdatePinCache(AbstractPinMap& t_pinMap) {
        for (auto& pair : t_pinMap) {
            s_pinCache[pair.first] = pair.second;
        }
    }

    std::optional<AbstractNode> Workspace::AddNode(std::string t_nodeName) {
        auto node = InstantiateNode(t_nodeName);
        if (node.has_value()) {
            s_nodes.push_back(node.value());
        }
        return node;
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
            return PopulateNode(t_nodeName, nodeImplementation.value().spawn());
        }
        return std::nullopt;
    }

    std::optional<AbstractNode> Workspace::InstantiateSerializedNode(Json data) {
        auto nodeImplementation = GetNodeImplementationByPackageName((std::string) data["PackageName"]);
        if (nodeImplementation.has_value()) {
            auto nodeInstance = InstantiateNode(nodeImplementation.value().libraryName);
            if (nodeInstance.has_value()) {
                auto node = nodeInstance.value();
                node->nodeID = data["NodeID"];
                node->libraryName = nodeImplementation.value().libraryName;
                if (!data["FlowInputPin"].is_null()) {
                    node->flowInputPin = GenericPin(data["FlowInputPin"]);
                }
                if (!data["FlowOutputPin"].is_null()) {
                    node->flowOutputPin = GenericPin(data["FlowOutputPin"]);
                }

                for (auto& pin : data["InputPins"]) {
                    node->inputPins.push_back(GenericPin(pin));
                }
                for (auto& pin : data["OutputPins"]) {
                    node->outputPins.push_back(GenericPin(pin));
                }

                return node;
            }
        }
        return std::nullopt;
    }

    std::optional<NodeImplementation> Workspace::GetNodeImplementationByLibraryName(std::string t_libraryName) {
        for (auto& impl : s_nodeImplementations) {
            if (impl.libraryName == t_libraryName) {
                return impl;
            }
        }
        return std::nullopt;
    }

    std::optional<NodeImplementation> Workspace::GetNodeImplementationByPackageName(std::string t_packageName) {
        for (auto& impl : s_nodeImplementations) {
            if (impl.description.packageName == t_packageName) {
                return impl;
            }
        }
        return std::nullopt;
    }

    AbstractNode Workspace::PopulateNode(std::string t_nodeName, AbstractNode node) {
        node->nodeID = Randomizer::GetRandomInteger();
        node->libraryName = t_nodeName;
        return node;
    }

    std::optional<AbstractNode> Workspace::GetNodeByNodeID(int nodeID) {
        for (auto& node : s_nodes) {
            if (node->nodeID == nodeID) return node;
        }
        return std::nullopt;
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