#pragma once

#include "common/audio_discretization_options.h"
#include "raster.h"
#include "typedefs.h"
#include "audio_bus.h"
#include "thread_unique_value.h"
#include "audio_discretization_options.h"
#include "asset_base.h"
#include "project_color_precision.h"

namespace Raster {
    struct Project {
        std::string path;
        std::string name, description;
        float framerate;
        float currentFrame;
        ThreadUniqueValue<std::vector<float>> timeTravelStack;
        bool playing, looping;
        AudioDiscretizationOptions audioOptions;
        ProjectColorPrecision colorPrecision;

        glm::vec2 preferredResolution;
        glm::vec4 backgroundColor;
        
        std::vector<Composition> compositions;
        std::vector<AbstractAsset> assets;

        std::vector<int> selectedCompositions;
        std::vector<int> selectedNodes;
        std::vector<int> selectedAttributes;
        std::vector<int> selectedKeyframes;
        std::vector<int> selectedAssets;

        std::vector<AudioBus> audioBuses;
        std::shared_ptr<std::mutex> audioBusesMutex;

        Json customData;

        Project();
        Project(Json data);

        float GetProjectLength();

        // Respects TimeTravel offsets
        float GetCorrectCurrentTime();

        void TimeTravel(float t_offset);
        float GetTimeTravelOffset();

        void ResetTimeTravel();

        std::string FormatFrameToTime(float frame);

        void Traverse(ContextData t_data = {});
        void OnTimelineSeek();

        glm::mat4 GetProjectionMatrix(bool inverted = false);

        Json Serialize();
    };
};