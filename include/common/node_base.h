#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "font/IconsFontAwesome5.h"
#include "typedefs.h"
#include "node_category/node_category.h"

namespace Raster {

    enum class PinType {
        Input, Output
    };

    struct GenericPin {
        int linkID, pinID, connectedPinID;
        std::string linkedAttribute;
        PinType type;
        bool flow;

        GenericPin(std::string t_linkedAttribute, PinType t_type, bool t_flow = false);
        GenericPin(Json data);
        GenericPin();

        Json Serialize();
    };

    struct NodeBase {

        friend struct Dispatchers;

        int nodeID;
        int executionsPerFrame;
        std::optional<GenericPin> flowInputPin, flowOutputPin;
        std::vector<GenericPin> inputPins, outputPins;
        std::string libraryName;
        std::string overridenHeader;
        bool enabled, bypassed;

        void SetAttributeValue(std::string t_attribute, std::any t_value);

        AbstractPinMap Execute(AbstractPinMap accumulator = {});
        std::string Header();

        bool DetailsAvailable();
        void RenderDetails();

        virtual std::optional<std::string> Footer() = 0;
        virtual std::string Icon() = 0;

        void AddOutputPin(std::string t_attribute);
        void AddInputPin(std::string t_attribute);

        void RenderAttributeProperty(std::string t_attribute);

        void ClearAttributesCache();

        virtual void AbstractLoadSerialized(Json data) {};
        virtual void AbstractRenderProperties() {};

        std::optional<GenericPin> GetAttributePin(std::string t_attribute);

        void TryAppendAbstractPinMap(AbstractPinMap& t_map, std::string t_attribute, std::any t_value);

        Json Serialize();

        std::vector<std::string> GetAttributesList();

        std::optional<std::any> GetDynamicAttribute(std::string t_attribute);

        template <typename T>
        std::optional<T> GetAttribute(std::string t_attribute);

        protected:
        std::unordered_map<std::string, std::any> m_attributes;

        virtual std::string AbstractHeader() = 0;

        virtual bool AbstractDetailsAvailable() { return false; };
        virtual void AbstractRenderDetails() {}

        virtual Json AbstractSerialize() { return {}; };
        virtual AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {}) = 0;
        void GenerateFlowPins();

        void SetupAttribute(std::string t_attribute, std::any t_defaultValue);

        void Initialize();

        private:
        std::unordered_map<std::string, std::any> m_attributesCache;
        std::vector<std::string> m_attributesOrder;

        AbstractPinMap m_accumulator;
    };

    using AbstractNode = std::shared_ptr<NodeBase>;
    using NodeSpawnProcedure = std::function<AbstractNode()>;

    struct NodeDescription {
        std::string prettyName;
        std::string packageName;
        NodeCategory category;
    };

    struct NodeImplementation {
        std::string libraryName;
        NodeDescription description;    
        NodeSpawnProcedure spawn;
    };
};