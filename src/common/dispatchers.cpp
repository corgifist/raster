#include "common/common.h"
#include "gpu/gpu.h"
#include "../ImGui/imgui.h"
#include "common/transform2d.h"

#include "common/dispatchers.h"

#include "font/font.h"

namespace Raster {
    PropertyDispatchersCollection Dispatchers::s_propertyDispatchers;
    StringDispatchersCollection Dispatchers::s_stringDispatchers;
    PreviewDispatchersCollection Dispatchers::s_previewDispatchers;
    OverlayDispatchersCollection Dispatchers::s_overlayDispatchers;
    ConversionDispatchersCollection Dispatchers::s_conversionDispatchers;

    bool Dispatchers::s_enableOverlays = true;


    void Dispatchers::DispatchProperty(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata) {
        for (auto& dispatcher : s_propertyDispatchers) {
            if (std::type_index(t_value.type()) == dispatcher.first) {
                dispatcher.second(t_owner, t_attribute, t_value, t_isAttributeExposed, t_metadata);
            }
        }
    }

    void Dispatchers::DispatchString(std::any& t_value) {
        bool dispatchingWasSuccessfull = false;
        for (auto& dispatcher : s_stringDispatchers) {
            if (std::type_index(t_value.type()) == dispatcher.first) {
                dispatcher.second(t_value);
                dispatchingWasSuccessfull = true;
            }
        }
        if (!dispatchingWasSuccessfull) {
            ImGui::Text("%s %s: %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("NO_DISPATCHER_FOR_TYPE").c_str(), Workspace::GetTypeName(t_value).c_str());
        }
    }

    void Dispatchers::DispatchPreview(std::any& t_value) {
        for (auto& dispatcher : s_previewDispatchers) {
            if (std::type_index(t_value.type()) == dispatcher.first) {
                dispatcher.second(t_value);
            }
        }
    }

    bool Dispatchers::DispatchOverlay(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize) {
        for (auto& dispatcher : s_overlayDispatchers) {
            if (std::type_index(t_attribute.type()) == dispatcher.first) {
                return dispatcher.second(t_attribute, t_composition, t_attributeID, t_zoom, t_regionSize);
            }
        }
        return true;
    }

    std::optional<std::any> Dispatchers::DispatchConversion(std::any& t_value, std::type_index t_targetType) {
        for (auto& dispatcher : s_conversionDispatchers) {
            if (dispatcher.from == t_value.type() && dispatcher.to == t_targetType) {
                return dispatcher.function(t_value);
            }
        }
        return std::nullopt;
    }

};