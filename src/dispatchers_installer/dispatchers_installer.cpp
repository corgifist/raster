#include "dispatchers_installer/dispatchers_installer.h"
#include "attribute_dispatchers.h"
#include "overlay_dispatchers.h"
#include "preview_dispatchers.h"
#include "string_dispatchers.h"
#include "common/common.h"
#include "common/transform2d.h"


namespace Raster {
    void DispatchersInstaller::Initialize() {
        Dispatchers::s_propertyDispatchers = {
            {ATTRIBUTE_TYPE(std::string), AttributeDispatchers::DispatchStringAttribute},
            {ATTRIBUTE_TYPE(float), AttributeDispatchers::DispatchFloatAttribute},
            {ATTRIBUTE_TYPE(int), AttributeDispatchers::DispatchIntAttribute},
            {ATTRIBUTE_TYPE(glm::vec4), AttributeDispatchers::DispatchVec4Attribute},
            {ATTRIBUTE_TYPE(glm::vec3), AttributeDispatchers::DispatchVec3Attribute},
            {ATTRIBUTE_TYPE(glm::vec2), AttributeDispatchers::DispatchVec2Attribute},
            {ATTRIBUTE_TYPE(Transform2D), AttributeDispatchers::DispatchTransform2DAttribute},
            {ATTRIBUTE_TYPE(SamplerSettings), AttributeDispatchers::DispatchSamplerSettingsAttribute},
            {ATTRIBUTE_TYPE(bool), AttributeDispatchers::DispatchBoolAttribute}
        };

        Dispatchers::s_stringDispatchers = {
            {ATTRIBUTE_TYPE(std::string), StringDispatchers::DispatchStringValue},
            {ATTRIBUTE_TYPE(float), StringDispatchers::DispatchFloatValue},
            {ATTRIBUTE_TYPE(int), StringDispatchers::DispatchIntValue},
            {ATTRIBUTE_TYPE(Texture), StringDispatchers::DispatchTextureValue},
            {ATTRIBUTE_TYPE(glm::vec4), StringDispatchers::DispatchVector4Value},
            {ATTRIBUTE_TYPE(glm::vec3), StringDispatchers::DispatchVector3Value},
            {ATTRIBUTE_TYPE(glm::vec2), StringDispatchers::DispatchVector2Value},
            {ATTRIBUTE_TYPE(Framebuffer), StringDispatchers::DispatchFramebufferValue},
            {ATTRIBUTE_TYPE(SamplerSettings), StringDispatchers::DispatchSamplerSettingsValue},
            {ATTRIBUTE_TYPE(Transform2D), StringDispatchers::DispatchTransform2DValue},
            {ATTRIBUTE_TYPE(bool), StringDispatchers::DispatchBoolValue}
        };

        Dispatchers::s_previewDispatchers = {
            {ATTRIBUTE_TYPE(std::string), PreviewDispatchers::DispatchStringValue},
            {ATTRIBUTE_TYPE(Texture), PreviewDispatchers::DispatchTextureValue},
            {ATTRIBUTE_TYPE(float), PreviewDispatchers::DispatchFloatValue},
            {ATTRIBUTE_TYPE(int), PreviewDispatchers::DispatchIntValue},
            {ATTRIBUTE_TYPE(glm::vec4), PreviewDispatchers::DispatchVector4Value},
            {ATTRIBUTE_TYPE(glm::vec3), PreviewDispatchers::DispatchVector3Value},
            {ATTRIBUTE_TYPE(glm::vec2), PreviewDispatchers::DispatchVector2Value},
            {ATTRIBUTE_TYPE(Framebuffer), PreviewDispatchers::DispatchFramebufferValue}, 
            {ATTRIBUTE_TYPE(bool), PreviewDispatchers::DispatchBoolValue}
        };

        Dispatchers::s_overlayDispatchers = {
            {ATTRIBUTE_TYPE(Transform2D), OverlayDispatchers::DispatchTransform2DValue}
        };
    }
};