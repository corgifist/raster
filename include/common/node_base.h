#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "font/IconsFontAwesome5.h"
#include "typedefs.h"
#include "common/node_category.h"
#include "dynamic_serialization.h"
#include "attribute_metadata.h"
#include "double_buffered_value.h"
#include "synchronized_value.h"

#define RASTER_ATTRIBUTE_CAST(t_type, t_name) \
    std::any_cast<t_type>(m_attributes[t_name])

#define RASTER_SERIALIZE_WRAPPER(t_type, t_name) \
    {t_name, RASTER_ATTRIBUTE_CAST(t_type, t_name)}

#define RASTER_DESERIALIZE_WRAPPER(t_type, t_name) \
    if (t_data.contains(t_name)) SetAttributeValue(t_name, t_data[t_name].get<t_type>())    

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
        int nodeID;
        DoubleBufferedValue<int> executionsPerFrame;
        std::optional<GenericPin> flowInputPin, flowOutputPin;
        std::vector<GenericPin> inputPins, outputPins;
        std::string libraryName;
        std::string overridenHeader;
        bool enabled, bypassed;
        std::optional<glm::vec2> nodePosition;

        ~NodeBase();

        void SetAttributeValue(std::string t_attribute, std::any t_value);

        AbstractPinMap Execute(ContextData& t_contextData);
        std::string Header();

        bool DetailsAvailable();
        void RenderDetails();

        virtual std::optional<std::string> Footer() = 0;
        virtual std::string Icon() = 0;

        void AddOutputPin(std::string t_attribute);
        void AddInputPin(std::string t_attribute);

        void RenderAttributeProperty(std::string t_attribute, std::vector<std::any> t_metadata = {});

        void ClearAttributesCache();

        virtual void AbstractLoadSerialized(Json data) { DeserializeAllAttributes(data); };
        virtual void AbstractRenderProperties() {};

        std::optional<GenericPin> GetAttributePin(std::string t_attribute);

        void TryAppendAbstractPinMap(AbstractPinMap& t_map, std::string t_attribute, std::any t_value);

        Json Serialize();

        std::vector<std::string> GetAttributesList();

        std::optional<std::any> GetDynamicAttribute(std::string t_attribute, ContextData& t_contextData);

        template <typename T>
        std::optional<T> GetAttribute(std::string t_attribute, ContextData& t_contextData);

        bool DoesAudioMixing();
        bool DoesRendering();

        std::vector<int> GetUsedAudioBuses();

        std::optional<float> GetContentDuration();

        void OnTimelineSeek();
        std::string GetAttributeName(std::string t_attributeName);
        std::optional<std::any> GetAttributeDefaultValue(std::string t_name);

        protected:
        SynchronizedValue<std::unordered_map<std::string, std::any>> m_attributes;

        virtual bool AbstractDoesAudioMixing() { return false; }
        virtual bool AbstractDoesRendering() { return false; }

        virtual void AbstractOnTimelineSeek() { };

        virtual std::optional<float> AbstractGetContentDuration() { return std::nullopt; }

        virtual std::string AbstractHeader() = 0;

        virtual bool AbstractDetailsAvailable() { return false; };
        virtual void AbstractRenderDetails() {}

        virtual std::vector<int> AbstractGetUsedAudioBuses() { return {}; }

        virtual Json AbstractSerialize() { return SerializeAllAttributes(); };
        virtual AbstractPinMap AbstractExecute(ContextData& t_contextData) {
            return {};
        }
        void GenerateFlowPins();

        void SetupAttribute(std::string t_attribute, std::any t_defaultValue);

        void SerializeAttribute(Json& t_data, std::string t_attribute);
        Json SerializeAttributes(std::vector<std::string> t_attributes);

        void DeserializeAttribute(Json& t_data, std::string t_attribute);
        void DeserializeAttributes(Json& t_data, std::vector<std::string> t_attributes);

        Json SerializeAllAttributes();
        void DeserializeAllAttributes(Json& t_data);

        void Initialize();

        void SetAttributeAlias(std::string t_attributeName, std::string t_alias);

        private:
        DoubleBufferedValue<SynchronizedValue<std::unordered_map<std::string, std::any>>> m_attributesCache;
        std::unordered_map<std::string, std::string> m_attributeAliases;
        std::vector<std::string> m_attributesOrder;

        bool ExecutingInAudioContext(ContextData& t_data);
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