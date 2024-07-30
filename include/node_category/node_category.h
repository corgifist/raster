#pragma once

#include "raster.h"

namespace Raster {
    using NodeCategory = unsigned int;

    struct NodeCategoryUtils {
        static std::vector<NodeCategory> s_categoriesOrder;

        static std::unordered_map<NodeCategory, std::string> s_iconMap;
        static std::unordered_map<NodeCategory, std::string> s_nameMap;

        static std::string ToIcon(NodeCategory t_category);
        static std::string ToString(NodeCategory t_category);

        static NodeCategory RegisterCategory(std::string t_icon, std::string t_name);
    };

    struct DefaultNodeCategories {
        static NodeCategory s_resources;
        static NodeCategory s_attributes;
        static NodeCategory s_math;
        static NodeCategory s_utilities;
        static NodeCategory s_rendering;
        static NodeCategory s_other;

        static void Initialize();
    };

};