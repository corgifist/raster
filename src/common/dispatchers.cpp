#include "common/bezier_curve.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "../ImGui/imgui.h"
#include "common/transform2d.h"
#include "common/dispatchers.h"
#include "font/font.h"
#include "dispatchers/attribute_dispatchers.h"
#include "dispatchers/overlay_dispatchers.h"
#include "dispatchers/preview_dispatchers.h"
#include "dispatchers/string_dispatchers.h"
#include "dispatchers/conversion_dispatchers.h"
#include "common/common.h"
#include "common/transform2d.h"
#include "common/audio_samples.h"
#include "common/asset_id.h"
#include "common/generic_audio_decoder.h"
#include "common/generic_resolution.h"
#include "common/gradient_1d.h"
#include "common/choice.h"
#include "common/line2d.h"

#define TYPE_PAIR(T1, T2) std::type_index(typeid(T1)), std::type_index(typeid(T2))

namespace Raster {
    PropertyDispatchersCollection Dispatchers::s_propertyDispatchers;
    StringDispatchersCollection Dispatchers::s_stringDispatchers;
    PreviewDispatchersCollection Dispatchers::s_previewDispatchers;
    OverlayDispatchersCollection Dispatchers::s_overlayDispatchers;
    ConversionDispatchersCollection Dispatchers::s_conversionDispatchers;

    bool Dispatchers::s_enableOverlays = true;
    bool Dispatchers::s_editingROI = false;
    bool Dispatchers::s_blockPopups = false;

    void Dispatchers::Initialize() {
        Dispatchers::s_propertyDispatchers = {
            {ATTRIBUTE_TYPE(std::string), AttributeDispatchers::DispatchStringAttribute},
            {ATTRIBUTE_TYPE(float), AttributeDispatchers::DispatchFloatAttribute},
            {ATTRIBUTE_TYPE(int), AttributeDispatchers::DispatchIntAttribute},
            {ATTRIBUTE_TYPE(glm::vec4), AttributeDispatchers::DispatchVec4Attribute},
            {ATTRIBUTE_TYPE(glm::vec3), AttributeDispatchers::DispatchVec3Attribute},
            {ATTRIBUTE_TYPE(glm::vec2), AttributeDispatchers::DispatchVec2Attribute},
            {ATTRIBUTE_TYPE(Transform2D), AttributeDispatchers::DispatchTransform2DAttribute},
            {ATTRIBUTE_TYPE(SamplerSettings), AttributeDispatchers::DispatchSamplerSettingsAttribute},
            {ATTRIBUTE_TYPE(bool), AttributeDispatchers::DispatchBoolAttribute},
            {ATTRIBUTE_TYPE(AssetID), AttributeDispatchers::DispatchAssetIDAttribute},
            {ATTRIBUTE_TYPE(GenericAudioDecoder), AttributeDispatchers::DispatchGenericAudioDecoderAttribute},
            {ATTRIBUTE_TYPE(AudioSamples), AttributeDispatchers::DispatchAudioSamplesAttribute},
            {ATTRIBUTE_TYPE(GenericResolution), AttributeDispatchers::DispatchGenericResolutionAttribute},
            {ATTRIBUTE_TYPE(Gradient1D), AttributeDispatchers::DispatchGradient1DAttribute},
            {ATTRIBUTE_TYPE(Choice), AttributeDispatchers::DispatchChoiceAttribute},
            {ATTRIBUTE_TYPE(Line2D), AttributeDispatchers::DispatchLine2DAttribute}
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
            {ATTRIBUTE_TYPE(bool), StringDispatchers::DispatchBoolValue},
            {ATTRIBUTE_TYPE(AudioSamples), StringDispatchers::DispatchAudioSamplesValue},
            {ATTRIBUTE_TYPE(AssetID), StringDispatchers::DispatchAssetIDValue},
            {ATTRIBUTE_TYPE(GenericResolution), StringDispatchers::DispatchGenericResolutionValue},
            {ATTRIBUTE_TYPE(Gradient1D), StringDispatchers::DispatchGradient1DValue},
            {ATTRIBUTE_TYPE(Line2D), StringDispatchers::DispatchLine2DValue},
            {ATTRIBUTE_TYPE(BezierCurve), StringDispatchers::DispatchBezierCurveValue}
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
            {ATTRIBUTE_TYPE(bool), PreviewDispatchers::DispatchBoolValue},
            {ATTRIBUTE_TYPE(AudioSamples), PreviewDispatchers::DispatchAudioSamplesValue},
            {ATTRIBUTE_TYPE(AssetID), PreviewDispatchers::DispatchAssetIDValue}
        };

        Dispatchers::s_overlayDispatchers = {
            {ATTRIBUTE_TYPE(Transform2D), OverlayDispatchers::DispatchTransform2DValue},
            {ATTRIBUTE_TYPE(Line2D), OverlayDispatchers::DispatchLine2DValue},
            {ATTRIBUTE_TYPE(ROI), OverlayDispatchers::DispatchROIValue},
            {ATTRIBUTE_TYPE(BezierCurve), OverlayDispatchers::DispatchBezierCurve}
        };

        Dispatchers::s_conversionDispatchers = {
            {TYPE_PAIR(AssetID, int), ConversionDispatchers::ConvertAssetIDToInt},
            {TYPE_PAIR(float, int), ConversionDispatchers::ConvertFloatToInt},
            {TYPE_PAIR(int, float), ConversionDispatchers::ConvertIntToFloat},
            {TYPE_PAIR(glm::vec3, glm::vec4), ConversionDispatchers::ConvertVec3ToVec4},
            {TYPE_PAIR(AssetID, Texture), ConversionDispatchers::ConvertAssetIDToTexture},
            {TYPE_PAIR(GenericResolution, glm::vec2), ConversionDispatchers::ConvertGenericResolutionToVec2},
            {TYPE_PAIR(Choice, int), ConversionDispatchers::ConvertChoiceToInt}
        };
    }

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