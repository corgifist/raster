#include "common/common.h"

namespace Raster {
    std::random_device Randomizer::s_random_device;
    std::mt19937 Randomizer::s_random(Randomizer::s_random_device());
    std::uniform_int_distribution<std::mt19937::result_type> Randomizer::s_distribution(1, 1000000000000);

    int Randomizer::GetRandomInteger() {
        return s_distribution(s_random);
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
}