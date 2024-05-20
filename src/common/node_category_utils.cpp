#include "common/common.h"

namespace Raster {
    std::string NodeCategoryUtils::ToIcon(NodeCategory t_category) {
        switch (t_category) {
            case NodeCategory::Resources: return ICON_FA_FOLDER;
            case NodeCategory::Utilities: return ICON_FA_GEARS;
            case NodeCategory::Other: return ICON_FA_SHAPES;
        }
        return ICON_FA_QUESTION;
    }

    std::string NodeCategoryUtils::ToString(NodeCategory t_category) {
        std::string icon = ToIcon(t_category) + " ";
        switch (t_category) {
            case NodeCategory::Resources: return icon + Localization::GetString("RESOURCES");
            case NodeCategory::Utilities: return icon + Localization::GetString("UTILITIES");
            case NodeCategory::Other: return icon + Localization::GetString("OTHER");
        }
        return icon + "?";
    }
}