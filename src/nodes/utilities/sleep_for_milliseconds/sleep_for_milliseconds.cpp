#include "sleep_for_milliseconds.h"

namespace Raster {

    SleepForMilliseconds::SleepForMilliseconds() {
        NodeBase::Initialize();

        SetupAttribute("PassthroughData", std::nullopt);
        SetupAttribute("Milliseconds", 20);
        SetupAttribute("PreciseSleep", false);

        AddInputPin("PassthroughData");
        AddOutputPin("Output");
    }

    AbstractPinMap SleepForMilliseconds::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto passthroughDataCandidate = GetDynamicAttribute("PassthroughData", t_contextData);
        auto millisecondsCandidate = GetAttribute<int>("Milliseconds", t_contextData);
        auto preciseSleepCandidate = GetAttribute<bool>("PreciseSleep", t_contextData);
        if (passthroughDataCandidate.has_value() && millisecondsCandidate.has_value() && preciseSleepCandidate.has_value()) {
            auto& passthroughData = passthroughDataCandidate.value();
            auto& milliseconds = millisecondsCandidate.value();
            auto& preciseSleep = preciseSleepCandidate.value();
            
            auto startTime = std::chrono::high_resolution_clock::now();

            if (preciseSleep) {
                ExperimentalSleepFor(std::abs(milliseconds) / 1000.0);
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(std::abs(milliseconds)));
            }

            // time elapsed in milliseconds
            m_lastMilliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now() - startTime
            ).count();
            TryAppendAbstractPinMap(result, "Output", passthroughData);
        }
        return result;
    }

    void SleepForMilliseconds::AbstractRenderProperties() {
        RenderAttributeProperty("PassthroughData");
        RenderAttributeProperty("Milliseconds");
        RenderAttributeProperty("PreciseSleep");
    }

    void SleepForMilliseconds::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json SleepForMilliseconds::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool SleepForMilliseconds::AbstractDetailsAvailable() {
        return false;
    }

    std::string SleepForMilliseconds::AbstractHeader() {
        return "Sleep for Milliseconds";
    }

    std::string SleepForMilliseconds::Icon() {
        return ICON_FA_MOON " " ICON_FA_STOPWATCH;
    }

    std::optional<std::string> SleepForMilliseconds::Footer() {
        return FormatString("%s %s: %i ms", ICON_FA_STOPWATCH, "Time Spent Sleeping", m_lastMilliseconds);
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SleepForMilliseconds>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Sleep for Milliseconds",
            .packageName = RASTER_PACKAGED "sleep_for_milliseconds",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}