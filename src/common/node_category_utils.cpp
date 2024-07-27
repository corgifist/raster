#include "common/common.h"

namespace Raster {
    std::string NodeCategoryUtils::ToIcon(NodeCategory t_category) {
        switch (t_category) {
            case NodeCategory::Resources: return ICON_FA_FOLDER;
            case NodeCategory::Utilities: return ICON_FA_SCREWDRIVER;
            case NodeCategory::Attributes: return ICON_FA_LINK;
            case NodeCategory::Rendering: return ICON_FA_IMAGE;
            case NodeCategory::SamplerConstants: return ICON_FA_GEARS;
            case NodeCategory::Other: return ICON_FA_SHAPES;
        }
        return ICON_FA_QUESTION;
    }

    std::string NodeCategoryUtils::ToString(NodeCategory t_category) {
        std::string icon = ToIcon(t_category) + " ";
        switch (t_category) {
            case NodeCategory::Resources: return icon + Localization::GetString("RESOURCES");
            case NodeCategory::Utilities: return icon + Localization::GetString("UTILITIES");
            case NodeCategory::Attributes: return icon + Localization::GetString("ATTRIBUTES");
            case NodeCategory::Rendering: return icon + Localization::GetString("RENDERING");
            case NodeCategory::SamplerConstants: return icon + Localization::GetString("SAMPLER_CONSTANTS");
            case NodeCategory::Other: return icon + Localization::GetString("OTHER");
        }
        return icon + "?";
    }
}