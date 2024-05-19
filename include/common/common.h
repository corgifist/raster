#pragma once

#include "raster.h"
#include "dylib.hpp"

namespace Raster {

    using AbstractPinMap = std::unordered_map<int, std::any>;

    enum class PinType {
        Input, Output
    };

    struct GenericPin {
        int linkID, pinID, connectedPinID;
        std::string linkedAttribute;
        PinType type;

        GenericPin(std::string t_linkedAttribute, PinType t_type);
        GenericPin();
    };

    struct NodeBase {
        int nodeID;
        std::optional<GenericPin> flowInputPin, flowOutputPin;
        std::vector<GenericPin> inputPins, outputPins;

        AbstractPinMap Execute(AbstractPinMap accumulator = {});
        virtual std::string Header() = 0;
        virtual std::optional<std::string> Footer() = 0;

        std::optional<std::string> GetStringAttribute(std::string t_attribute);

        protected:
        virtual AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {}) = 0;
        void GenerateFlowPins();

        private:
        AbstractPinMap m_accumulator;
    };

    using AbstractNode = std::shared_ptr<NodeBase>;

    struct Randomizer {
        static int GetRandomInteger();

        static std::random_device s_random_device;
        static std::mt19937 s_random;
        static std::uniform_int_distribution<std::mt19937::result_type> s_distribution;
    };

    struct Workspace {
        static std::vector<AbstractNode> s_nodes;
        static std::vector<std::string> s_initializedNodes;

        static void Initialize();

        static std::optional<AbstractNode> InstantiateNode(std::string t_nodeName);
        static AbstractNode PopulateNode(AbstractNode node);

        static std::optional<AbstractNode> GetNodeByPinID(int pinID);
        static std::optional<GenericPin> GetPinByPinID(int pinID);
        static void UpdatePinByID(GenericPin pin, int pinID);

        template<class T>
        static T GetBaseName(T const & path, T const & delims = "/\\") {
            return path.substr(path.find_last_of(delims) + 1);
        }
    };

    struct Libraries {
        static std::unordered_map<std::string, internalDylib> s_registry;

        static void LoadLibrary(std::string t_path, std::string t_key) {
            if (s_registry.find(t_key) != s_registry.end()) return; // do not reload libraries
            s_registry[t_key] = internalDylib(t_path, t_key);
        }

        template<typename T>
        static T* GetFunction(std::string t_key, std::string t_name) {
            if (s_registry.find(t_key) == s_registry.end())
                LoadLibrary(".", t_key);
            internalDylib* d = &s_registry[t_key];
            return d->get_function<T>(t_name.c_str());
        }

        template<typename T>
        static T& GetVariable(std::string t_key, std::string t_name) {
            if (s_registry.find(t_key) == s_registry.end())
                LoadLibrary(".", t_key);
            internalDylib* d = &s_registry[t_key];
            return d->get_variable<T>(t_name.c_str());
        }
    };

}