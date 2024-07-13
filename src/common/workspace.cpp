#include "common/common.h"
#include "gpu/gpu.h"

namespace Raster {

    std::vector<NodeCategory> Workspace::s_categories = {
        NodeCategory::Attributes,
        NodeCategory::Rendering,
        NodeCategory::Resources,
        NodeCategory::Utilities,
        NodeCategory::Other
    };

    std::optional<Project> Workspace::s_project;
    std::vector<int> Workspace::s_selectedCompositions;
    std::vector<NodeImplementation> Workspace::s_nodeImplementations;
    Configuration Workspace::s_configuration;

    std::unordered_map<int, std::any> Workspace::s_pinCache;

    std::vector<int> Workspace::s_selectedNodes;
    std::vector<int> Workspace::s_targetSelectNodes;

    std::unordered_map<std::type_index, uint32_t> Workspace::s_typeColors = {
        {ATTRIBUTE_TYPE(std::string), RASTER_COLOR32(204, 0, 103, 255)},
        {ATTRIBUTE_TYPE(Texture), RASTER_COLOR32(0, 102, 255, 255)},
        {ATTRIBUTE_TYPE(float), RASTER_COLOR32(66, 135, 245, 255)},
        {ATTRIBUTE_TYPE(glm::vec4), RASTER_COLOR32(242, 183, 22, 255)},
        {ATTRIBUTE_TYPE(Raster::Framebuffer), RASTER_COLOR32(52, 235, 171, 255)}
    };

    std::unordered_map<std::type_index, std::string> Workspace::s_typeNames = {
        RASTER_TYPE_NAME(std::string),
        RASTER_TYPE_NAME(Raster::Texture),
        RASTER_TYPE_NAME(float),
        RASTER_TYPE_NAME(glm::vec4),
        RASTER_TYPE_NAME(Raster::Framebuffer)
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

    std::optional<AbstractNode> Workspace::CopyAbstractNode(AbstractNode node) {
        auto nodeCandidate = InstantiateNode(node->libraryName);
        if (nodeCandidate.has_value()) {
            auto node = nodeCandidate.value();
            node->AbstractLoadSerialized(node->Serialize()["Data"]);
            return node;
        }
        return std::nullopt;
    }

    std::optional<Composition*> Workspace::GetCompositionByID(int t_id) {
        if (!s_project.has_value()) return std::nullopt;
        auto& project = s_project.value();
        for (auto& composition : project.compositions) {
            if (composition.id == t_id) return &composition;
        }
        return std::nullopt;
    }

    std::optional<std::vector<Composition*>> Workspace::GetSelectedCompositions() {
        if (!s_project.has_value()) return std::nullopt;
        auto& project = s_project.value();
        std::vector<Composition*> result;
        for (auto& id : s_selectedCompositions) {
            auto compositionCandidate = GetCompositionByID(id);
            if (!compositionCandidate.has_value()) return std::nullopt;
            result.push_back(compositionCandidate.value());
        }
        if (result.empty()) return std::nullopt;
        return result;
    }

    std::optional<Composition*> Workspace::GetCompositionByNodeID(int t_nodeID) {
        if (!s_project.has_value()) return std::nullopt;
        auto& project = s_project.value();
        for (auto& composition : project.compositions) {
            for (auto& node : composition.nodes) {
                if (node->nodeID == t_nodeID) return &composition;
            }
        }
        return std::nullopt;
    }

    std::optional<Composition*> Workspace::GetCompositionByAttributeID(int t_attributeID) {
        if (!s_project.has_value()) return std::nullopt;
        auto& project = s_project.value();
        for (auto& composition : project.compositions) {
            for (auto& attribute : composition.attributes) {
                if (attribute->id == t_attributeID) return &composition;
            }
        }
        return std::nullopt;
    }

    void Workspace::UpdatePinCache(AbstractPinMap& t_pinMap) {
        for (auto& pair : t_pinMap) {
            s_pinCache[pair.first] = pair.second;
        }
    }

    std::optional<AbstractNode> Workspace::AddNode(std::string t_nodeName) {
        auto node = InstantiateNode(t_nodeName);
        if (node.has_value()) {
            auto compositionsCandidate = GetSelectedCompositions();
            if (compositionsCandidate.has_value()) {
                compositionsCandidate.value()[0]->nodes.push_back(node.value());
            }
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
        std::cout << data.dump() << std::endl;
        auto nodeImplementation = GetNodeImplementationByPackageName((std::string) data["PackageName"]);
        if (nodeImplementation.has_value()) {
            auto nodeInstance = InstantiateNode(nodeImplementation.value().libraryName);
            if (nodeInstance.has_value()) {
                auto& node = nodeInstance.value();
                node->nodeID = data["NodeID"];
                node->libraryName = nodeImplementation.value().libraryName;
                node->enabled = data["Enabled"];
                node->bypassed = data["Bypassed"];
                node->AbstractLoadSerialized(data["NodeData"]);
                if (!data["FlowInputPin"].is_null()) {
                    node->flowInputPin = GenericPin(data["FlowInputPin"]);
                }
                if (!data["FlowOutputPin"].is_null()) {
                    node->flowOutputPin = GenericPin(data["FlowOutputPin"]);
                }

                for (auto& pin : data["InputPins"]) {
                    node->inputPins.push_back(GenericPin(pin));
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
        auto compositionsCandidate = GetSelectedCompositions();
        if (!compositionsCandidate.has_value()) return std::nullopt;
        for (auto& composition : compositionsCandidate.value()) {
            for (auto& node : composition->nodes) {
                if (node->nodeID == nodeID) return node;
            }
        }
        return std::nullopt;
    }

    std::optional<AbstractNode> Workspace::GetNodeByPinID(int pinID) {
        auto compositionsCandidate = GetSelectedCompositions();
        if (!compositionsCandidate.has_value()) return std::nullopt;
        auto compositions = compositionsCandidate.value();
        for (auto& composition : compositions) {
            for (auto& node : composition->nodes) {
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
        }
        return std::nullopt;
    }

    std::optional<GenericPin> Workspace::GetPinByLinkID(int linkID) {
        auto compositionsCandidate = GetSelectedCompositions();
        if (!compositionsCandidate.has_value()) return std::nullopt;
        auto compositions = compositionsCandidate.value();
        for (auto& composition : compositions) {
            for (auto& node : composition->nodes) {
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
        }
        return std::nullopt;
    }

    std::optional<GenericPin> Workspace::GetPinByPinID(int pinID) {
        auto compositionsCandidate = GetSelectedCompositions();
        if (!compositionsCandidate.has_value()) return std::nullopt;
        auto compositions = compositionsCandidate.value();
        for (auto& composition : compositions) {
            for (auto& node : composition->nodes) {
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
        }
        return std::nullopt;
    }

    void Workspace::UpdatePinByID(GenericPin pin, int pinID) {
        auto compositionsCandidate = GetSelectedCompositions();
        if (!compositionsCandidate.has_value()) return;
        auto compositions = compositionsCandidate.value();
        for (auto& composition : compositions) {
            for (auto& node : composition->nodes) {
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

    std::optional<AbstractAttribute> Workspace::GetAttributeByKeyframeID(int t_keyframeID) {
        if (s_project.has_value()) {
            auto& project = s_project.value();
            for (auto& composition : project.compositions) {
                for (auto& attribute : composition.attributes) {
                    for (auto& keyframe : attribute->keyframes) {
                        if (keyframe.id == t_keyframeID) {
                            return attribute;
                        }
                    }
                }
            }
        }
        return std::nullopt;
    }

    std::optional<AbstractAttribute> Workspace::GetAttributeByAttributeID(int t_attributeID) {
        if (s_project.has_value()) {
            auto& project = s_project.value();
            for (auto& composition : project.compositions) {
                for (auto& attribute : composition.attributes) {
                    if (attribute->id == t_attributeID) return attribute;
                }
            }
        }
        return std::nullopt;
    }

    std::optional<AbstractAttribute> Workspace::GetAttributeByName(Composition* t_composition, std::string t_name) {
        for (auto& attribute : t_composition->attributes) {
            if (attribute->name == t_name) return attribute;
        }
        return std::nullopt;
    }

    std::optional<AttributeKeyframe*> Workspace::GetKeyframeByKeyframeID(int t_keyframeID) {
        if (s_project.has_value()) {
            auto& project = s_project.value();
            for (auto& composition : project.compositions) {
                for (auto& attribute : composition.attributes) {
                    for (auto& keyframe : attribute->keyframes) {
                        if (keyframe.id == t_keyframeID) {
                            return &keyframe;
                        }
                    }
                }
            }
        }
        return std::nullopt;
    }

    std::string Workspace::GetTypeName(std::any& t_value) {
        if (s_typeNames.find(std::type_index(t_value.type())) != s_typeNames.end()) {
            return s_typeNames[std::type_index(t_value.type())];
        }
        return t_value.type().name();
    }
}