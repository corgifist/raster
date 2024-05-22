#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "font/IconsFontAwesome5.h"

#define INSTANTIATE_ATTRIBUTE_TEMPLATE(T) \
    template std::optional<T> NodeBase::GetAttribute<T>(std::string);

namespace Raster {

    using AbstractPinMap = std::unordered_map<int, std::any>;
    using Json = nlohmann::json;

    enum class PinType {
        Input, Output
    };

    enum class NodeCategory {
        Resources,
        Utilities,
        Other
    };

    struct NodeCategoryUtils {
        static std::string ToIcon(NodeCategory t_category);
        static std::string ToString(NodeCategory t_category);
    };

    struct Localization {
        static void Load(Json t_map);
        static std::string GetString(std::string t_key);

        protected:
        static std::unordered_map<std::string, std::string> m_map;
    };

    struct Configuration {
        std::string localizationCode;

        Configuration(Json data);
        Configuration();

        Json Serialize();
    };

    struct GenericPin {
        int linkID, pinID, connectedPinID;
        std::string linkedAttribute;
        PinType type;

        GenericPin(std::string t_linkedAttribute, PinType t_type);
        GenericPin();
    };

    struct NodeDescription {
        std::string prettyName;
        std::string packageName;
        NodeCategory category;
    };

    struct NodeBase {
        int nodeID;
        std::optional<GenericPin> flowInputPin, flowOutputPin;
        std::vector<GenericPin> inputPins, outputPins;

        AbstractPinMap Execute(AbstractPinMap accumulator = {});
        virtual std::string Header() = 0;
        virtual std::optional<std::string> Footer() = 0;

        template <typename T>
        std::optional<T> GetAttribute(std::string t_attribute);

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

    using NodeSpawnProcedure = std::function<AbstractNode()>;

    struct NodeImplementation {
        std::string libraryName;
        NodeDescription description;    
        NodeSpawnProcedure spawn;
    };

    struct Workspace {
        static std::vector<NodeCategory> s_categories;
        static std::vector<AbstractNode> s_nodes;
        static std::vector<NodeImplementation> s_nodeImplementations;
        static Configuration s_configuration;

        static void Initialize();

        static void AddNode(std::string t_nodeName);
        static std::optional<AbstractNode> InstantiateNode(std::string t_nodeName);
        static AbstractNode PopulateNode(AbstractNode node);

        static std::optional<AbstractNode> GetNodeByPinID(int pinID);
        static std::optional<GenericPin> GetPinByPinID(int pinID);
        static std::optional<GenericPin> GetPinByLinkID(int linkID);
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

    template<typename ... Args>
    static std::string FormatString( const std::string& format, Args ... args ) {
        int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
        if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
        auto size = static_cast<size_t>( size_s );
        std::unique_ptr<char[]> buf( new char[ size ] );
        std::snprintf( buf.get(), size, format.c_str(), args ... );
        return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
    }

    static Json ReadJson(std::string path) {
        return Json::parse(std::fstream(path));
    }
}