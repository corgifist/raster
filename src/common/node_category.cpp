#include "common/node_category.h"
#include "common/localization.h"
#include "common/randomizer.h"
#include "font/font.h"

namespace Raster {
    std::vector<NodeCategory> NodeCategoryUtils::s_categoriesOrder;

    std::unordered_map<NodeCategory, std::string> NodeCategoryUtils::s_iconMap;
    std::unordered_map<NodeCategory, std::string> NodeCategoryUtils::s_nameMap;

    NodeCategory DefaultNodeCategories::s_audio;
    NodeCategory DefaultNodeCategories::s_resources;
    NodeCategory DefaultNodeCategories::s_attributes;
    NodeCategory DefaultNodeCategories::s_math;
    NodeCategory DefaultNodeCategories::s_utilities;
    NodeCategory DefaultNodeCategories::s_rendering;
    NodeCategory DefaultNodeCategories::s_other;

    void DefaultNodeCategories::Initialize() {
        s_audio = NodeCategoryUtils::RegisterCategory(ICON_FA_VOLUME_HIGH, Localization::GetString("AUDIO"));
        s_resources = NodeCategoryUtils::RegisterCategory(ICON_FA_FOLDER, Localization::GetString("RESOURCES"));
        s_utilities = NodeCategoryUtils::RegisterCategory(ICON_FA_SCREWDRIVER, Localization::GetString("UTILITIES"));
        s_attributes = NodeCategoryUtils::RegisterCategory(ICON_FA_LINK, Localization::GetString("ATTRIBUTES"));
        s_math = NodeCategoryUtils::RegisterCategory(ICON_FA_DIVIDE, Localization::GetString("MATH"));
        s_rendering = NodeCategoryUtils::RegisterCategory(ICON_FA_IMAGE, Localization::GetString("RENDERING"));
        s_other = NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, Localization::GetString("OTHER"));
    }

    std::string NodeCategoryUtils::ToIcon(NodeCategory t_category) {
        if (s_iconMap.find(t_category) != s_iconMap.end()) {
            return s_iconMap[t_category];
        }
        return ICON_FA_QUESTION;
    }

    std::string NodeCategoryUtils::ToString(NodeCategory t_category) {
        std::string icon = ToIcon(t_category) + " ";
        if (s_nameMap.find(t_category) != s_nameMap.end()) {
            return icon + s_nameMap[t_category];
        }
        return icon + "?";
    }

    NodeCategory NodeCategoryUtils::RegisterCategory(std::string t_icon, std::string t_name) {
        for (auto& pair : s_nameMap) {
            if (pair.second == t_name) return pair.first;
        }

        NodeCategory categoryID = static_cast<NodeCategory>(Randomizer::GetRandomInteger());
        s_iconMap[categoryID] = t_icon;
        s_nameMap[categoryID] = t_name;
        s_categoriesOrder.push_back(categoryID);
        return categoryID;
    }
}