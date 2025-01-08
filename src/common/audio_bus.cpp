#include "common/audio_bus.h"
#include "common/workspace.h"
#include "common/audio_info.h"

namespace Raster {
    AudioBus::AudioBus() {
        this->name = "Audio Bus";
        this->id = Randomizer::GetRandomInteger();
        this->redirectID = 0;
        this->main = false;
        this->colorMark = Workspace::s_colorMarks[Workspace::s_defaultColorMark];
    }

    AudioBus::AudioBus(Json t_data) {
        this->name = t_data["Name"];
        this->id = t_data["ID"];
        this->redirectID = t_data["RedirectID"];
        this->main = t_data["Main"];
        if (t_data.contains("ColorMark")) this->colorMark = t_data["ColorMark"];
    }

    void AudioBus::ValidateBuffers() {
        if (samples.size() != AudioInfo::s_periodSize * AudioInfo::s_channels) {
            samples.resize(AudioInfo::s_periodSize * AudioInfo::s_channels);
        }
    }

    Json AudioBus::Serialize() {
        return {
            {"Name", name},
            {"ID", id},
            {"RedirectID", redirectID},
            {"Main", main},
            {"ColorMark", colorMark}
        };
    }
};