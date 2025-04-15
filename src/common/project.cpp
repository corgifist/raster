#include "common/audio_discretization_options.h"
#include "common/project_color_precision.h"
#include "raster.h"
#include "common/common.h"
#include "common/rendering.h"

namespace Raster {
    Project::Project(Json data) {
        this->name = data["Name"];
        this->description = data["Description"];
        this->framerate = data["Framerate"];
        this->currentFrame = data["CurrentFrame"];
        this->preferredResolution = {
            (float) data["PreferredResolution"][0],
            (float) data["PreferredResolution"][1]
        };
        this->backgroundColor = {
            (float) data["BackgroundColor"][0],
            (float) data["BackgroundColor"][1],
            (float) data["BackgroundColor"][2],
            (float) data["BackgroundColor"][3]
        };
        this->playing = data["Playing"];
        this->looping = data["Looping"];
        if (data.contains("AudioDiscretizationOptions")) {
            this->audioOptions = AudioDiscretizationOptions(data["AudioDiscretizationOptions"]);
        }
        if (data.contains("ColorPrecision")) {
            this->colorPrecision = static_cast<ProjectColorPrecision>(data["ColorPrecision"].get<int>());
        } else {
            this->colorPrecision = ProjectColorPrecision::Half;
        }
        this->selectedCompositions = data["SelectedCompositions"].get<std::vector<int>>();
        this->selectedAttributes = data["SelectedAttributes"].get<std::vector<int>>();
        this->selectedNodes = data["SelectedNodes"].get<std::vector<int>>();
        this->selectedKeyframes = data["SelectedKeyframes"].get<std::vector<int>>();
        this->selectedAssets = data["SelectedAssets"].get<std::vector<int>>();
        this->customData = data["CustomData"];
        if (data.contains("ROI")) {
            this->roi = ROI(data["ROI"]);
        }
        for (auto& composition : data["Compositions"]) {
            compositions.push_back(Composition(composition));
        }
        for (auto& asset : data["Assets"]) {
            auto assetCandidate = Assets::InstantiateSerializedAsset(asset);
            if (assetCandidate.has_value()) {
                assets.push_back(assetCandidate.value());
            }
        }

        for (auto& bus : data["AudioBuses"]) {
            audioBuses.push_back(AudioBus(bus));
        }
        
        this->audioBusesMutex = std::make_shared<std::mutex>();
    }

    Project::Project() {
        this->name = "New Project";
        this->description = "Empty Description";
        this->framerate = 60;
        this->currentFrame = 0;
        this->preferredResolution = {
            1080, 1080
        };
        this->playing = false;
        this->looping = false;
        this->backgroundColor = {0, 0, 0, 1};
        this->customData = {
            {"Placeholder", true}
        };
        this->colorPrecision = ProjectColorPrecision::Half;

        AudioBus mainBus;
        mainBus.main = true;
        mainBus.name = "Primary Audio Bus";

        audioBuses.push_back(mainBus);

        this->audioBusesMutex = std::make_shared<std::mutex>();

        this->compositions.push_back(Composition());
        this->selectedCompositions = {compositions[0].id};
    }

    float Project::GetProjectLength() {
        float candidate = FLT_MIN;
        for (auto& composition : compositions) {
            if (composition.GetEndFrame() > candidate) {
                candidate = composition.GetEndFrame();
            }
        }
        return candidate == FLT_MIN ? 0 : candidate;
    }

    std::string Project::FormatFrameToTime(float frame) {
        frame = std::floor(frame);
        framerate = std::floor(framerate);
        int transformedFrame = frame / framerate;
        int minutes = transformedFrame / 60;
        int seconds = transformedFrame % 60;
        return FormatString("%02i:%02i", minutes, seconds);
    }

    glm::mat4 Project::GetProjectionMatrix(bool invert) {
        float aspect = preferredResolution.x / preferredResolution.y;
        return glm::ortho(-aspect, aspect, 1.0f * (invert ? -1 : 1), -1.0f * (invert ? -1 : 1), -1.0f, 1.0f);
    }

    std::optional<Camera> Project::GetCamera() {
        for (int i = compositions.size(); i --> 0;) {
            auto& composition = compositions[i];
            if (!composition.enabled) continue;
            if (!IsInBounds(GetCorrectCurrentTime(), composition.beginFrame - 1, composition.endFrame + 1)) continue;
            for (auto& attribute : composition.attributes) {
                if (attribute->packageName != RASTER_PACKAGED "camera_attribute") continue;
                auto cameraCandidate = attribute->Get(GetCorrectCurrentTime() - composition.beginFrame, &composition);
                auto camera = std::any_cast<Camera>(cameraCandidate);
                if (camera.enabled) return camera;
            }
        }
        return std::nullopt;
    }

    std::optional<AbstractAttribute> Project::GetCameraAttribute() {
        for (int i = compositions.size(); i --> 0;) {
            auto& composition = compositions[i];
            if (!composition.enabled) continue;
            if (!IsInBounds(GetCorrectCurrentTime(), composition.beginFrame - 1, composition.endFrame + 1)) continue;
            for (auto& attribute : composition.attributes) {
                if (attribute->packageName != RASTER_PACKAGED "camera_attribute") continue;
                auto cameraCandidate = attribute->Get(GetCorrectCurrentTime() - composition.beginFrame, &composition);
                auto camera = std::any_cast<Camera>(cameraCandidate);
                if (camera.enabled) return attribute;
            }
        }
        return std::nullopt;
    }

    float Project::GetCorrectCurrentTime() {
        auto& fakeTime = m_fakeTime.Get();
        if (fakeTime) {
            return *fakeTime + GetTimeTravelOffset();
        }
        return currentFrame + GetTimeTravelOffset();
    }

    float Project::GetTimeTravelOffset() {
        if (timeTravelStack.Get().empty()) return 0.0;
        return std::reduce(timeTravelStack.Get().begin(), timeTravelStack.Get().end());
    }

    void Project::TimeTravel(float t_offset) {
        timeTravelStack.Get().push_back(t_offset);
    }

    void Project::ResetTimeTravel() {
        if (timeTravelStack.Get().empty()) return;
        timeTravelStack.Get().pop_back();
    }

    void Project::Traverse(ContextData t_data) {
        if (RASTER_GET_CONTEXT_VALUE(t_data, "RESET_WORKSPACE_STATE", bool)) {
            Workspace::s_pinCache.Get().clear();
        }
        for (auto& composition : compositions) {
            composition.Traverse(t_data);
        }

        timeTravelStack.Get().clear();
    }

    void Project::SetFakeTime(float t_frame) {
        auto& fakeTime = m_fakeTime.Get();
        fakeTime = t_frame;
    }

    void Project::ResetFakeTime() {
        auto& fakeTime = m_fakeTime.Get();
        fakeTime = std::nullopt;
    }

    void Project::OnTimelineSeek() {
        Rendering::ForceRenderFrame();
        for (auto& composition : compositions) {
            composition.OnTimelineSeek();
        }
    }

    Json Project::Serialize() {
        Json data = {};

        data["Name"] = name;
        data["Description"] = description;
        data["Framerate"] = framerate;
        data["CurrentFrame"] = currentFrame;
        data["PreferredResolution"] = {
            preferredResolution.x, preferredResolution.y
        };
        data["BackgroundColor"] = {
            backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a
        };
        data["Playing"] = playing;
        data["Looping"] = looping;
        data["SelectedCompositions"] = selectedCompositions;
        data["SelectedNodes"] = selectedNodes;
        data["SelectedAttributes"] = selectedAttributes;
        data["SelectedKeyframes"] = selectedKeyframes;
        data["SelectedAssets"] = selectedAssets;
        data["CustomData"] = customData;
        data["AudioDiscretizationOptions"] = audioOptions.Serialize();
        data["ColorPrecision"] = static_cast<int>(colorPrecision);
        data["ROI"] = roi.Serialize();

        data["Compositions"] = {};
        for (auto& composition : compositions) {
            data["Compositions"].push_back(composition.Serialize());
        }
        
        data["Assets"] = {};
        for (auto& asset : assets) {
            data["Assets"].push_back(asset->Serialize());
        }

        data["AudioBuses"] = {};
        for (auto& bus : audioBuses) {
            data["AudioBuses"].push_back(bus.Serialize());
        }

        return data;
    }
};