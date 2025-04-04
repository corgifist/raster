#include "posterize_time.h"

namespace Raster {

    PosterizeTime::PosterizeTime() {
        NodeBase::Initialize();

        SetupAttribute("Input", std::nullopt);
        SetupAttribute("Levels", 5);

        AddInputPin("Input");
        AddOutputPin("Output");
    }

    AbstractPinMap PosterizeTime::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto& project = Workspace::GetProject();

        auto levelsCandidate = GetAttribute<int>("Levels", t_contextData);
        if (levelsCandidate.has_value()) {
            PerformPosterization(levelsCandidate.value());
            auto dynamicCandidate = GetDynamicAttribute("Input", t_contextData);
            if (dynamicCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Output", dynamicCandidate.value());
            }
            project.ResetTimeTravel();
        }
        return result;
    }

    void PosterizeTime::PerformPosterization(float t_levels) {
        auto& project = Workspace::GetProject();
        float currentTime = project.GetCorrectCurrentTime();
        float posterizedTime = glm::ceil((currentTime / project.framerate) * t_levels) / t_levels * project.framerate;

        project.TimeTravel(posterizedTime - currentTime);
    }

    void PosterizeTime::AbstractRenderProperties() {
        RenderAttributeProperty("Levels");
    }

    void PosterizeTime::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json PosterizeTime::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool PosterizeTime::AbstractDetailsAvailable() {
        return false;
    }

    std::string PosterizeTime::AbstractHeader() {
        return "Posterize Time";
    }

    std::string PosterizeTime::Icon() {
        return ICON_FA_STOPWATCH;
    }

    std::optional<std::string> PosterizeTime::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::PosterizeTime>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Posterize Time",
            .packageName = RASTER_PACKAGED "posterize_time",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}