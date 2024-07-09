#include "common/common.h"
#include "gpu/gpu.h"
#include "../ImGui/imgui.h"

namespace Raster {
    PropertyDispatchersCollection Dispatchers::s_propertyDispatchers = {
        {ATTRIBUTE_TYPE(std::string), AttributeDispatchers::DispatchStringAttribute}
    };

    StringDispatchersCollection Dispatchers::s_stringDispatchers = {
        {ATTRIBUTE_TYPE(std::string), StringDispatchers::DispatchStringValue},
        {ATTRIBUTE_TYPE(float), StringDispatchers::DispatchFloatValue},
        {ATTRIBUTE_TYPE(Texture), StringDispatchers::DispatchTextureValue},
        {ATTRIBUTE_TYPE(glm::vec4), StringDispatchers::DispatchVector4Value},
        {ATTRIBUTE_TYPE(Framebuffer), StringDispatchers::DispatchFramebufferValue}
    };

    PreviewDispatchersCollection Dispatchers::s_previewDispatchers = {
        {ATTRIBUTE_TYPE(std::string), PreviewDispatchers::DispatchStringValue},
        {ATTRIBUTE_TYPE(Texture), PreviewDispatchers::DispatchTextureValue},
        {ATTRIBUTE_TYPE(float), PreviewDispatchers::DispatchFloatValue},
        {ATTRIBUTE_TYPE(glm::vec4), PreviewDispatchers::DispatchVector4Value},
        {ATTRIBUTE_TYPE(Framebuffer), PreviewDispatchers::DispatchFramebufferValue}
    };

    void Dispatchers::DispatchProperty(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        for (auto& dispatcher : s_propertyDispatchers) {
            if (std::type_index(t_value.type()) == dispatcher.first) {
                dispatcher.second(t_owner, t_attribute, t_value, t_isAttributeExposed);
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
};