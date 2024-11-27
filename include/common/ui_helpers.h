#pragma once

#include "common/audio_discretization_options.h"
#include "raster.h"
#include "workspace.h"
#include "audio_samples.h"
#include "gradient_1d.h"

namespace Raster {
    struct UIHelpers {
        static void SelectAttribute(Composition* t_composition, int& t_attributeID, std::string t_headerText = "", std::string* t_customAttributeFilter = nullptr);
        static void OpenSelectAttributePopup();

        static void SelectAsset(int& t_assetID, std::string t_headerText, std::string* t_customAttributeFilter = nullptr);
        static void OpenSelectAssetPopup();

        static bool CustomTreeNode(std::string t_id);

        static void RenderAudioSamplesWaveform(AudioSamples& t_samples);
        static void RenderRawAudioSamplesWaveform(std::vector<float>* t_raw);

        static void RenderDecibelScale(float t_linear);

        static void RenderNothingToShowText();

        static void RenderGradient1D(Gradient1D& t_gradient, float t_width = 0, float t_height = 0, float t_alpha = 1.0f);
        static bool RenderGradient1DEditor(Gradient1D& t_gradient, float t_width = 0, float t_height = 0.0f);

        static void RenderProjectEditor(Project& t_project);
        static void RenderAudioDiscretizationOptionsEditor(AudioDiscretizationOptions& t_options);

        static bool CenteredButton(const char* string, float alignment = 0.5f);
        static bool AnyItemFocused();
    };
};