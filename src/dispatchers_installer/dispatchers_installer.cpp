#include "dispatchers_installer/dispatchers_installer.h"
#include "attribute_dispatchers.h"
#include "overlay_dispatchers.h"
#include "preview_dispatchers.h"
#include "string_dispatchers.h"
#include "conversion_dispatchers.h"
#include "common/common.h"
#include "common/transform2d.h"
#include "common/audio_samples.h"
#include "common/asset_id.h"
#include "common/generic_audio_decoder.h"
#include "common/generic_resolution.h"
#include "common/gradient_1d.h"

#define TYPE_PAIR(T1, T2) std::type_index(typeid(T1)), std::type_index(typeid(T2))

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
            {ATTRIBUTE_TYPE(bool), AttributeDispatchers::DispatchBoolAttribute},
            {ATTRIBUTE_TYPE(AssetID), AttributeDispatchers::DispatchAssetIDAttribute},
            {ATTRIBUTE_TYPE(GenericAudioDecoder), AttributeDispatchers::DispatchGenericAudioDecoderAttribute},
            {ATTRIBUTE_TYPE(AudioSamples), AttributeDispatchers::DispatchAudioSamplesAttribute},
            {ATTRIBUTE_TYPE(GenericResolution), AttributeDispatchers::DispatchGenericResolutionAttribute},
            {ATTRIBUTE_TYPE(Gradient1D), AttributeDispatchers::DispatchGradient1DAttribute}
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
            {ATTRIBUTE_TYPE(Gradient1D), StringDispatchers::DispatchGradient1DValue}
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
            {ATTRIBUTE_TYPE(Transform2D), OverlayDispatchers::DispatchTransform2DValue}
        };

        Dispatchers::s_conversionDispatchers = {
            {TYPE_PAIR(AssetID, int), ConversionDispatchers::ConvertAssetIDToInt},
            {TYPE_PAIR(float, int), ConversionDispatchers::ConvertFloatToInt},
            {TYPE_PAIR(int, float), ConversionDispatchers::ConvertIntToFloat},
            {TYPE_PAIR(glm::vec3, glm::vec4), ConversionDispatchers::ConvertVec3ToVec4},
            {TYPE_PAIR(AssetID, Texture), ConversionDispatchers::ConvertAssetIDToTexture},
            {TYPE_PAIR(GenericAudioDecoder, AudioSamples), ConversionDispatchers::ConvertGenericAudioDecoderToAudioSamples},
            {TYPE_PAIR(GenericResolution, glm::vec2), ConversionDispatchers::ConvertGenericResolutionToVec2}
        };
    }
};