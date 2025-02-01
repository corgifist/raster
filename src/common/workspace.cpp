#include "common/workspace.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "common/transform2d.h"
#include "common/audio_samples.h"
#include "common/asset_id.h"
#include "common/generic_audio_decoder.h"
#include "common/generic_resolution.h"
#include "common/gradient_1d.h"
#include "common/node_category.h"
#include "common/choice.h"
#include "common/plugins.h"
#include "common/waveform_manager.h"
#include "common/rendering.h"
#include "common/line2d.h"
#include "raster.h"


namespace Raster {
    std::optional<Project> Workspace::s_project;
    std::mutex Workspace::s_projectMutex;
    std::mutex Workspace::s_nodesMutex;

    std::vector<NodeImplementation> Workspace::s_nodeImplementations;
    Configuration Workspace::s_configuration;

    DoubleBufferedValue<unordered_dense::map<int, std::any>> Workspace::s_pinCache;

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
        {ATTRIBUTE_TYPE(AudioSamples), RASTER_COLOR32(139, 95, 239, 255)},
        {ATTRIBUTE_TYPE(AssetID), RASTER_COLOR32(50, 168, 82, 255)},
        {ATTRIBUTE_TYPE(Gradient1D), RASTER_COLOR32(201, 24, 115, 255)},
        {ATTRIBUTE_TYPE(Line2D), RASTER_COLOR32(56, 168, 82, 255)}
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
        RASTER_TYPE_NAME(bool),
        RASTER_TYPE_NAME(AssetID),
        RASTER_TYPE_NAME(GenericAudioDecoder),
        RASTER_TYPE_NAME(GenericResolution),
        RASTER_TYPE_NAME(Gradient1D),
        RASTER_TYPE_NAME(Choice),
        RASTER_TYPE_NAME(std::nullopt),
        RASTER_TYPE_NAME(Line2D)
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
        {"Light Green", RASTER_COLOR32(143, 192, 87, 255)},
        {"Light Orange", RASTER_COLOR32(211, 145, 72, 255)}
    };

    std::string Workspace::s_defaultColorMark = "Teal";

    enum class PinLocationSpecification {
        None, FlowInput, FlowOutput,
        Input, Output
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
        // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        for (auto& composition : project.compositions) {
            if (composition.nodes.find(t_nodeID) != composition.nodes.end()) return &composition;
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
        auto& pinCache = s_pinCache.Get();
        for (auto& pair : t_pinMap) {
            pinCache[pair.first] = pair.second;
        }
    }

    std::optional<AbstractNode> Workspace::AddNode(std::string t_nodeName) {
        auto node = InstantiateNode(t_nodeName);
        if (node.has_value()) {
            auto compositionsCandidate = GetSelectedCompositions();
            if (compositionsCandidate.has_value()) {
                auto& result = node.value();
                RASTER_SYNCHRONIZED(Workspace::s_nodesMutex);
                compositionsCandidate.value()[0]->nodes[result->nodeID] = result;
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
                if (data.contains("OverridenHeader")) {
                    node->overridenHeader = data["OverridenHeader"];
                }
                if (!data["NodePosition"].is_null()) {
                    auto nodePosition = data["NodePosition"];
                    node->nodePosition = glm::vec2(nodePosition[0], nodePosition[1]);
                }
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
                        for (auto& nodePin : node->inputPins) {
                            if (nodePin.linkedAttribute == pin["LinkedAttribute"].get<std::string>()) {
                                nodePin = GenericPin(pin);
                            }
                        }
                    }                    
                    for (auto& pin : data["InputPins"]) {
                        bool hasCandidates = false;
                        for (auto& nodePin : node->inputPins) {
                            if (nodePin.linkedAttribute == pin["LinkedAttribute"].get<std::string>()) {
                                hasCandidates = true;
                                break;
                            }
                        }
                        if (!hasCandidates) {
                            node->inputPins.push_back(GenericPin(pin));
                        }
                    }
                }

                if (!data["OutputPins"].is_null()) {
                    for (auto& pin : data["OutputPins"]) {
                        for (auto& nodePin : node->outputPins) {
                            if (nodePin.linkedAttribute == pin["LinkedAttribute"].get<std::string>()) {
                                nodePin = GenericPin(pin);
                            }
                        }
                    }
                    for (auto& pin : data["OutputPins"]) {
                        bool hasCandidates = false;
                        for (auto& nodePin : node->outputPins) {
                            if (nodePin.linkedAttribute == pin["LinkedAttribute"].get<std::string>()) {
                                hasCandidates = true;
                                break;
                            }
                        }
                        if (!hasCandidates) {
                            node->outputPins.push_back(GenericPin(pin));
                        }
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
        // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        for (auto& composition : compositions) {
            if (composition.nodes.find(nodeID) != composition.nodes.end()) {
                return composition.nodes[nodeID];
            }
        }
        return std::nullopt;
    }

    std::optional<AbstractNode> Workspace::GetNodeByPinID(int pinID) {
        if (!s_project.has_value()) return std::nullopt;
        auto& compositions = s_project.value().compositions;
        // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        static unordered_dense::map<int, int> s_cache;
        if (s_cache.find(pinID) != s_cache.end()) {
            return Workspace::GetNodeByNodeID(s_cache[pinID]);
        }
        for (auto& composition : compositions) {
            for (auto& node : composition.nodes) {
                if (node.second->flowInputPin.has_value()) {
                    if (node.second->flowInputPin.value().pinID == pinID) {
                        s_cache[pinID] = node.second->nodeID;
                        return std::optional{node.second};
                    }
                }
                if (node.second->flowOutputPin.has_value()) {
                    if (node.second->flowOutputPin.value().pinID == pinID) {
                        s_cache[pinID] = node.second->nodeID;
                        return std::optional{node.second};
                    }
                }
                for (auto& inputPin : node.second->inputPins) {
                    if (inputPin.pinID == pinID) {
                        s_cache[pinID] = node.second->nodeID;
                        return std::optional{node.second};
                    }
                }
                for (auto& outputPin : node.second->outputPins) {
                    if (outputPin.pinID == pinID) {
                        s_cache[pinID] = node.second->nodeID;
                        return std::optional{node.second};
                    }
                }
            }
        }
        return std::nullopt;
    }

    std::optional<GenericPin> Workspace::GetPinByLinkID(int linkID) {
        if (!s_project.has_value()) return std::nullopt;
        auto& compositions = s_project.value().compositions;
        // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        static unordered_dense::map<int, int> s_cache;
        if (s_cache.find(linkID) != s_cache.end()) {
            return Workspace::GetPinByPinID(s_cache[linkID]);
        }
        for (auto& composition : compositions) {
            for (auto& node : composition.nodes) {
                if (node.second->flowInputPin.has_value()) {
                    if (node.second->flowInputPin.value().linkID == linkID) {
                        s_cache[linkID] = node.second->flowInputPin.value().pinID;
                        return node.second->flowInputPin.value();
                    }
                }
                if (node.second->flowOutputPin.has_value()) {
                    if (node.second->flowOutputPin.value().linkID== linkID) {
                        s_cache[linkID] = node.second->flowOutputPin.value().pinID;
                        return node.second->flowOutputPin.value();
                    }
                }
                for (auto& inputPin : node.second->inputPins) {
                    if (inputPin.linkID == linkID) {
                        s_cache[linkID] = inputPin.pinID;
                        return inputPin;
                    }
                }
                for (auto& outputPin : node.second->outputPins) {
                    if (outputPin.linkID == linkID) {
                        s_cache[linkID] = outputPin.pinID;
                        return outputPin;
                    }
                }
            }
        }
        return std::nullopt;
    }

    struct PinByPinIDCache {
        int nodeID;
        PinLocationSpecification location;

        PinByPinIDCache() : nodeID(0), location(PinLocationSpecification::None) {}
        PinByPinIDCache(int t_nodeID, PinLocationSpecification t_location) : nodeID(t_nodeID), location(t_location) {}
    };

    std::optional<GenericPin> Workspace::GetPinByPinID(int pinID) {
        if (!s_project.has_value()) return std::nullopt;
        auto& compositions = s_project.value().compositions;
        // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        static unordered_dense::map<int, PinByPinIDCache> s_cache;
        if (s_cache.find(pinID) != s_cache.end()) {
            auto& cache = s_cache[pinID];
            auto nodeCandidate = Workspace::GetNodeByNodeID(cache.nodeID);
            if (nodeCandidate.has_value()) {
                auto& node = nodeCandidate.value();
                switch (cache.location) {
                    case PinLocationSpecification::FlowInput: {
                        if (node->flowInputPin.has_value() && node->flowInputPin.value().pinID == pinID) {
                            return node->flowInputPin.value();
                        }
                        break;
                    }
                    case PinLocationSpecification::FlowOutput: {
                        if (node->flowOutputPin.has_value() && node->flowOutputPin.value().pinID == pinID) {
                            return node->flowOutputPin.value();
                        }
                        break;
                    }
                    case PinLocationSpecification::Input: {
                        for (auto& inputPin : node->inputPins) {
                            if (inputPin.pinID == pinID) {
                                return inputPin;
                            }
                        }
                        break;
                    }
                    case PinLocationSpecification::Output: {
                        for (auto& outputPin : node->outputPins) {
                            if (outputPin.pinID == pinID) {
                                return outputPin;
                            }
                        }
                        break;
                    }
                }
            }
        }


        for (auto& composition : compositions) {
            for (auto& node : composition.nodes) {
                if (node.second->flowInputPin.has_value()) {
                    if (node.second->flowInputPin.value().pinID == pinID) {
                        s_cache[pinID] = PinByPinIDCache(node.second->nodeID, PinLocationSpecification::FlowInput);
                        return node.second->flowInputPin;
                    }
                }
                if (node.second->flowOutputPin.has_value()) {
                    if (node.second->flowOutputPin.value().pinID == pinID) {
                        s_cache[pinID] = PinByPinIDCache(node.second->nodeID, PinLocationSpecification::FlowOutput);
                        return node.second->flowOutputPin;
                    }
                }
                for (auto& inputPin : node.second->inputPins) {
                    if (inputPin.pinID == pinID) {
                        s_cache[pinID] = PinByPinIDCache(node.second->nodeID, PinLocationSpecification::Input);
                        return std::optional{inputPin};
                    }
                }
                for (auto& outputPin : node.second->outputPins) {
                    if (outputPin.pinID == pinID) {
                        s_cache[pinID] = PinByPinIDCache(node.second->nodeID, PinLocationSpecification::Output);
                        return std::optional{outputPin};
                    }
                }
            }
        }
        return std::nullopt;
    }

    void Workspace::UpdatePinByID(GenericPin pin, int pinID) {
        if (!s_project.has_value()) return;
        auto& compositions = s_project.value().compositions;
        // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        for (auto& composition : compositions) {
            for (auto& node : composition.nodes) {
                if (node.second->flowInputPin.has_value()) {
                    if (node.second->flowInputPin.value().pinID == pinID) {
                        node.second->flowInputPin = pin;
                    }
                }
                if (node.second->flowOutputPin.has_value()) {
                    if (node.second->flowOutputPin.value().pinID == pinID) {
                        node.second->flowOutputPin = pin;
                    }
                }
                for (auto& inputPin : node.second->inputPins) {
                    if (inputPin.pinID == pinID) {
                        inputPin = pin;
                    }
                }
                for (auto& outputPin : node.second->outputPins) {
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
            // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
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
            // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
            for (auto& composition : project.compositions) {
                for (auto& attribute : composition.attributes) {
                    if (attribute->id == t_attributeID) return attribute;
                }
            }
        }
        return std::nullopt;
    }

    std::optional<AbstractAttribute> Workspace::GetAttributeByName(Composition* t_composition, std::string t_name) {
        // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        for (auto& attribute : t_composition->attributes) {
            if (attribute->name == t_name) return attribute;
        }
        return std::nullopt;
    }

    std::optional<AttributeKeyframe*> Workspace::GetKeyframeByKeyframeID(int t_keyframeID) {
        if (s_project.has_value()) {
            auto& project = s_project.value();
            // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
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
            // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
            for (auto& asset : project.assets) {
                if (asset->id == t_assetID) return asset;
            }
        }
        return std::nullopt;
    }

    std::optional<int> Workspace::GetAssetIndexByAssetID(int t_assetID) {
        int index = 0;
        if (Workspace::IsProjectLoaded()) {
            // RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
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

    std::optional<AbstractAsset> Workspace::ImportAsset(std::string t_path) {
        if (!Workspace::IsProjectLoaded()) return std::nullopt;
        auto& project = Workspace::GetProject();
        std::optional<std::string> assetPackageNameCandidate;
        std::string pathExtension = GetExtension(t_path);
        pathExtension = ReplaceString(pathExtension, "\\.", "");
        for (auto& implementation : Assets::s_implementations) {
            auto& extensions = implementation.description.extensions;
            if (std::find(extensions.begin(), extensions.end(), pathExtension) != extensions.end()) {
                assetPackageNameCandidate = implementation.description.packageName;
                break;
            }
        }

        if (assetPackageNameCandidate.has_value()) {
            auto& assetPackageName = assetPackageNameCandidate.value();
            auto assetCandidate = Assets::InstantiateAsset(assetPackageName);
            if (assetCandidate.has_value()) {
                auto& asset = assetCandidate.value();
                asset->Import(t_path);
                return asset;
            } else {
                RASTER_LOG("failed to instantiate '" << *assetPackageNameCandidate << "'");
            }
        } else {
            RASTER_LOG("failed to find suitable asset type for " << t_path);
        }

        return std::nullopt;
    }

    void Workspace::OpenProject(std::string t_path) {
        try {
            if (std::filesystem::exists(t_path + "/project.json")) {
                Workspace::s_project = Project(ReadJson(t_path + "/project.json"));
                Workspace::s_project.value().path = t_path + "/";

                auto& project = Workspace::GetProject();
                for (auto& composition : project.compositions) {
                    WaveformManager::RequestWaveformRefresh(composition.id);
                }
            }
        } catch (...) {
            RASTER_LOG("failed to open project: " << t_path);
        }
        Rendering::ForceRenderFrame();
    }

    void Workspace::DeleteComposition(int t_id) {
        if (!Workspace::IsProjectLoaded()) return;
        auto& project = Workspace::GetProject();
        int targetCompositionIndex = 0;
        for (auto& iterationComposition : project.compositions) {
            if (t_id == iterationComposition.id) break;
            targetCompositionIndex++;
        }
        RASTER_SYNCHRONIZED(Workspace::s_projectMutex);
        project.compositions.erase(project.compositions.begin() + targetCompositionIndex);
        WaveformManager::EraseRecord(t_id);
        for (auto& composition : project.compositions) {
            if (composition.lockedCompositionID == t_id) {
                composition.lockedCompositionID = -1;
            }
        }
        Rendering::ForceRenderFrame();
    }

    Project& Workspace::GetProject() {
        return s_project.value();
    }

    bool Workspace::IsProjectLoaded() {
        return s_project.has_value();
    }
}