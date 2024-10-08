#include "common/common.h"
#include "gpu/gpu.h"
#include "common/transform2d.h"
#include "common/audio_samples.h"

namespace Raster {

    std::optional<Project> Workspace::s_project;
    std::mutex Workspace::s_projectMutex;

    std::vector<NodeImplementation> Workspace::s_nodeImplementations;
    Configuration Workspace::s_configuration;

    std::mutex Workspace::s_pinCacheMutex;
    std::unordered_map<int, std::any> Workspace::s_pinCache;

    std::vector<int> Workspace::s_targetSelectNodes;

    std::unordered_map<std::type_index, uint32_t> Workspace::s_typeColors = {
        {ATTRIBUTE_TYPE(std::string), RASTER_COLOR32(204, 0, 103, 255)},
        {ATTRIBUTE_TYPE(Texture), RASTER_COLOR32(0, 102, 255, 255)},
        {ATTRIBUTE_TYPE(float), RASTER_COLOR32(66, 135, 245, 255)},
        {ATTRIBUTE_TYPE(glm::vec4), RASTER_COLOR32(242, 183, 22, 255)},
        {ATTRIBUTE_TYPE(glm::vec3), RASTER_COLOR32(242, 183, 0, 255)},
        {ATTRIBUTE_TYPE(glm::vec2), RASTER_COLOR32(185, 214, 56, 255)},
        {ATTRIBUTE_TYPE(Raster::Framebuffer), RASTER_COLOR32(52, 235, 171, 255)},
        {ATTRIBUTE_TYPE(Transform2D), RASTER_COLOR32(120, 66, 245, 255)},
        {ATTRIBUTE_TYPE(SamplerSettings), RASTER_COLOR32(124, 186, 53, 255)},
        {ATTRIBUTE_TYPE(int), RASTER_COLOR32(50, 168, 82, 255)},
        {ATTRIBUTE_TYPE(AudioSamples), RASTER_COLOR32(139, 95, 239, 255)}
    };

    std::unordered_map<std::type_index, std::string> Workspace::s_typeNames = {
        RASTER_TYPE_NAME(std::string),
        RASTER_TYPE_NAME(Texture),
        RASTER_TYPE_NAME(float),
        RASTER_TYPE_NAME(int),
        RASTER_TYPE_NAME(glm::vec4),
        RASTER_TYPE_NAME(glm::vec3),
        RASTER_TYPE_NAME(glm::vec2),
        RASTER_TYPE_NAME(Framebuffer),
        RASTER_TYPE_NAME(Transform2D),
        RASTER_TYPE_NAME(SamplerSettings),
        RASTER_TYPE_NAME(AudioSamples),
        RASTER_TYPE_NAME(bool)
    };

    std::unordered_map<std::string, uint32_t> Workspace::s_colorMarks = {
        {"Red", RASTER_COLOR32(241, 70, 63, 255)},
        {"Pink", RASTER_COLOR32(230, 42, 101, 255)},
        {"Purple", RASTER_COLOR32(153, 54, 171, 255)},
        {"Deep Purple", RASTER_COLOR32(100, 68, 179, 255)},
        {"Indigo", RASTER_COLOR32(60, 87, 176, 255)},
        {"Blue", RASTER_COLOR32(32, 154, 239, 255)},
        {"Light Blue", RASTER_COLOR32(13, 172, 238, 255)},
        {"Cyan", RASTER_COLOR32(28, 188, 210, 255)}, 
        {"Teal", RASTER_COLOR32(24, 149, 135, 255)},
        {"Green", RASTER_COLOR32(84, 172, 88, 255)},
        {"Light Green", RASTER_COLOR32(143, 192, 87, 255)}
    };

    std::vector<int> Workspace::s_persistentPins = {};

    std::string Workspace::s_defaultColorMark = "Teal";

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
            try {
                Libraries::GetFunction<void()>(transformedPath, "OnStartup")();
            } catch (internalDylib::exception ex) {
                // skip
            }
            s_nodeImplementations.push_back(implementation);
            std::cout << "loading node '" << implementation.description.packageName << "'" << std::endl;
        }

        Easings::Initialize();
        Assets::Initialize();
        Attributes::Initialize();
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
        for (auto& id : project.selectedCompositions) {
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
        RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
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
        RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        for (auto& composition : project.compositions) {
            for (auto& attribute : composition.attributes) {
                if (attribute->id == t_attributeID) return &composition;
            }
        }
        return std::nullopt;
    }

    void Workspace::UpdatePinCache(AbstractPinMap& t_pinMap) {
        RASTER_SYNCHRONIZED(s_pinCacheMutex);
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
            if (impl.description.packageName == t_nodeName) {
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
                if (data.contains("NodeData") && !data["NodeData"].is_null()) {
                    node->AbstractLoadSerialized(data["NodeData"]);
                }
                if (!data["FlowInputPin"].is_null()) {
                    node->flowInputPin = GenericPin(data["FlowInputPin"]);
                }
                if (!data["FlowOutputPin"].is_null()) {
                    node->flowOutputPin = GenericPin(data["FlowOutputPin"]);
                }

                if (!data["InputPins"].is_null()) {
                    for (auto& pin : data["InputPins"]) {
                        bool continueLoop = false;
                        for (auto& inputPin : node->inputPins) {
                            if (inputPin.linkedAttribute == pin["LinkedAttribute"].get<std::string>()) {
                                continueLoop = true;
                                break;
                            }
                        }
                        if (continueLoop) {
                            int pinIndex = 0;
                            for (auto& inputPin : node->inputPins) {
                                if (inputPin.linkedAttribute == pin["LinkedAttribute"].get<std::string>()) break;
                                pinIndex++;
                            }
                            node->inputPins.erase(node->inputPins.begin() + pinIndex);
                        }
                        node->inputPins.push_back(GenericPin(pin));
                    }
                }

                if (!data["OutputPins"].is_null()) {
                    for (auto& pin : data["OutputPins"]) {
                        bool continueLoop = false; 
                        for (auto& outputPin : node->outputPins) {
                            if (outputPin.linkedAttribute == pin["LinkedAttribute"].get<std::string>()) {
                                continueLoop = true;
                                break;
                            }
                        }
                        if (continueLoop) {
                            int pinIndex = 0;
                            for (auto& outputPin : node->outputPins) {
                                if (outputPin.linkedAttribute == pin["LinkedAttribute"].get<std::string>()) break;
                                pinIndex++;
                            }
                            node->outputPins.erase(node->outputPins.begin() + pinIndex);
                        }
                        node->outputPins.push_back(GenericPin(pin));
                    }
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
            if (impl.description.packageName == t_libraryName) {
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
        if (!s_project.has_value()) return std::nullopt;
        auto& compositions = s_project.value().compositions;
        RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        for (auto& composition : compositions) {
            for (auto& node : composition.nodes) {
                if (node->nodeID == nodeID) return node;
            }
        }
        return std::nullopt;
    }

    std::optional<AbstractNode> Workspace::GetNodeByPinID(int pinID) {
        if (!s_project.has_value()) return std::nullopt;
        auto& compositions = s_project.value().compositions;
        RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        for (auto& composition : compositions) {
            for (auto& node : composition.nodes) {
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
        if (!s_project.has_value()) return std::nullopt;
        auto& compositions = s_project.value().compositions;
        RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        for (auto& composition : compositions) {
            for (auto& node : composition.nodes) {
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
        if (!s_project.has_value()) return std::nullopt;
        auto& compositions = s_project.value().compositions;
        RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        for (auto& composition : compositions) {
            for (auto& node : composition.nodes) {
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
        if (!s_project.has_value()) return;
        auto& compositions = s_project.value().compositions;
        RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        for (auto& composition : compositions) {
            for (auto& node : composition.nodes) {
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
            RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
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
            RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
            for (auto& composition : project.compositions) {
                for (auto& attribute : composition.attributes) {
                    if (attribute->id == t_attributeID) return attribute;
                }
            }
        }
        return std::nullopt;
    }

    std::optional<AbstractAttribute> Workspace::GetAttributeByName(Composition* t_composition, std::string t_name) {
        RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        for (auto& attribute : t_composition->attributes) {
            if (attribute->name == t_name) return attribute;
        }
        return std::nullopt;
    }

    std::optional<AttributeKeyframe*> Workspace::GetKeyframeByKeyframeID(int t_keyframeID) {
        if (s_project.has_value()) {
            auto& project = s_project.value();
            RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
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

    std::optional<AbstractAsset> Workspace::GetAssetByAssetID(int t_assetID) {
        if (Workspace::IsProjectLoaded()) {
            auto& project = Workspace::GetProject();
            RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
            for (auto& asset : project.assets) {
                if (asset->id == t_assetID) return asset;
            }
        }
        return std::nullopt;
    }

    std::optional<int> Workspace::GetAssetIndexByAssetID(int t_assetID) {
        int index = 0;
        if (Workspace::IsProjectLoaded()) {
            RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
            auto& project = Workspace::GetProject();
            for (auto& asset : project.assets) {
                if (asset->id == t_assetID) return index;
                index++;
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

    std::optional<AudioBus*> Workspace::GetAudioBusByID(int t_busID) {
        if (Workspace::IsProjectLoaded()) {
            auto& project = Workspace::GetProject();
            for (auto& bus : project.audioBuses) {
                if (bus.id == t_busID) {
                    return &bus;
                }
            }
        }
        return std::nullopt;
    }


    Project& Workspace::GetProject() {
        return s_project.value();
    }

    bool Workspace::IsProjectLoaded() {
        return s_project.has_value();
    }
}