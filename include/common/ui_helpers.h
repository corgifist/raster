#pragma once

#include "raster.h"
#include "workspace.h"
#include "audio_samples.h"

namespace Raster {
    struct UIHelpers {
        static void SelectAttribute(Composition* t_composition, int& t_attributeID, std::string t_headerText = "", std::string* t_customAttributeFilter = nullptr);
        static void OpenSelectAttributePopup();

        static void SelectAsset(int& t_assetID, std::string t_headerText, std::string* t_customAttributeFilter = nullptr);
        static void OpenSelectAssetPopup();

        static bool CustomTreeNode(std::string t_id);

        static void RenderAudioSamplesWaveform(AudioSamples& t_samples);
    };
};