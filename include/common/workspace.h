#pragma once

#include "raster.h"
#include "typedefs.h"

#include "node_base.h"
#include "project.h"
#include "configuration.h"
#include "composition.h"
#include "attribute.h"
#include "sampler_settings.h"

#define RASTER_TYPE_NAME(T) {std::type_index(typeid(T)), #T}

namespace Raster {
    struct Workspace {
        static std::vector<NodeCategory> s_categories;
        static std::optional<Project> s_project;
        static std::vector<NodeImplementation> s_nodeImplementations;
        static Configuration s_configuration;

        static std::vector<int> s_selectedCompositions;
        static std::vector<int> s_selectedNodes;
        static std::vector<int> s_selectedAttributes;

        static std::vector<int> s_targetSelectNodes;

        static std::unordered_map<int, std::any> s_pinCache;
        static std::unordered_map<std::type_index, std::string> s_typeNames;
        static std::unordered_map<std::type_index, uint32_t> s_typeColors;

        static void Initialize();

        static void UpdatePinCache(AbstractPinMap& t_pinMap);

        static std::optional<Composition*> GetCompositionByID(int t_id);
        static std::optional<std::vector<Composition*>> GetSelectedCompositions();
        static std::optional<Composition*> GetCompositionByNodeID(int t_nodeID);
        static std::optional<Composition*> GetCompositionByAttributeID(int t_attributeID);

        static std::optional<AbstractNode> AddNode(std::string t_nodeName);
        static std::optional<AbstractNode> InstantiateNode(std::string t_nodeName);
        static std::optional<AbstractNode> InstantiateSerializedNode(Json node);

        static std::optional<AbstractNode> CopyAbstractNode(AbstractNode node);
 
        static std::optional<NodeImplementation> GetNodeImplementationByLibraryName(std::string t_libraryName);
        static std::optional<NodeImplementation> GetNodeImplementationByPackageName(std::string t_packageName);

        static AbstractNode PopulateNode(std::string t_nodeName, AbstractNode node);

        static std::optional<AbstractNode> GetNodeByNodeID(int nodeID);
        static std::optional<AbstractNode> GetNodeByPinID(int pinID);

        static std::optional<GenericPin> GetPinByPinID(int pinID);
        static std::optional<GenericPin> GetPinByLinkID(int linkID);
        static void UpdatePinByID(GenericPin pin, int pinID);

        static std::optional<AbstractAttribute> GetAttributeByKeyframeID(int t_keyframeID);
        static std::optional<AbstractAttribute> GetAttributeByAttributeID(int t_attributeID);
        static std::optional<AbstractAttribute> GetAttributeByName(Composition* t_composition, std::string t_name);
        static std::optional<AttributeKeyframe*> GetKeyframeByKeyframeID(int t_keyframeID);

        static std::string GetTypeName(std::any& t_value);

        template<class T>
        static T GetBaseName(T const & path, T const & delims = "/\\") {
            return path.substr(path.find_last_of(delims) + 1);
        }
    };
};