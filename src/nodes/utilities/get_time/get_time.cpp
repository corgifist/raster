#include "get_time.h"

namespace Raster {

    GetTime::GetTime() {
        NodeBase::Initialize();

        SetupAttribute("RelativeTime", false);

        AddOutputPin("Time");
    }

    AbstractPinMap GetTime::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto relativeTimeCandidate = GetAttribute<bool>("RelativeTime", t_contextData);
        if (relativeTimeCandidate.has_value()) {
            bool relativeTime = relativeTimeCandidate.value();
            m_lastRelativeTime = relativeTime;
            if (relativeTime) {
                TryAppendAbstractPinMap(result, "Time", Workspace::GetProject().GetCorrectCurrentTime() - Workspace::GetCompositionByNodeID(nodeID).value()->beginFrame);
            } else {
                TryAppendAbstractPinMap(result, "Time", Workspace::GetProject().GetCorrectCurrentTime());
            }
        }
        return result;
    }

    void GetTime::AbstractRenderProperties() {
        RenderAttributeProperty("RelativeTime");
    }

    void GetTime::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json GetTime::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool GetTime::AbstractDetailsAvailable() {
        return false;
    }

    std::string GetTime::AbstractHeader() {
        return "Get Time";
    }

    std::string GetTime::Icon() {
        return m_lastRelativeTime.value_or(false) ? ICON_FA_TIMELINE " " ICON_FA_STOPWATCH : ICON_FA_STOPWATCH;
    }

    std::optional<std::string> GetTime::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::GetTime>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Get Time",
            .packageName = RASTER_PACKAGED "get_time",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}