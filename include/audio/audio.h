#pragma once

#include "raster.h"
#include "common/audio_discretization_options.h"

namespace Raster {

    struct AudioBackendInfo {
        std::string name, version;
    };

    struct Audio {
        static AudioBackendInfo s_backendInfo;
        static AudioDiscretizationOptions s_currentOptions;

        static void Initialize();

        // creates new audio instance
        // should not be called if audio instance already exists, cause it
        // will cause memory leaks or other side effects 
        static void CreateAudioInstance();

        // returns true if audio instance active
        static bool IsAudioInstanceActive();

        // should be called every frame
        // if current audio options are updated, then recreates audio instance
        // returns true if audio instance properties was updated
        static bool UpdateAudioInstance();

        // terminates current audio instance 
        // should be called when updating Audio::s_currentOptions
        static void TerminateAudioInstance();
    };
};