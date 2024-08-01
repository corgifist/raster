#include "get_time.h"

namespace Raster {

    GetTime::GetTime() {
        NodeBase::Initialize();

        this->m_attributes["RelativeTime"] = false;

        AddOutputPin("Time");
    }

    AbstractPinMap GetTime::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto relativeTimeCandidate = GetAttribute<bool>("RelativeTime");
        if (relativeTimeCandidate.has_value()) {
            bool relativeTime = relativeTimeCandidate.value();
            if (relativeTime) {
                TryAppendAbstractPinMap(result, "Time", Workspace::GetProject().currentFrame - Workspace::GetCompositionByNodeID(nodeID).value()->beginFrame);
            } else {
                TryAppendAbstractPinMap(result, "Time", Workspace::GetProject().currentFrame);
            }
        }
        return result;
    }

    void GetTime::AbstractRenderProperties() {
        RenderAttributeProperty("RelativeTime");
    }

    bool GetTime::AbstractDetailsAvailable() {
        return false;
    }

    std::string GetTime::AbstractHeader() {
        return "Get Time";
    }

    std::string GetTime::Icon() {
        return GetAttribute<bool>("RelativeTime").value_or(false) ? ICON_FA_TIMELINE " " ICON_FA_STOPWATCH : ICON_FA_STOPWATCH;
    }

    std::optional<std::string> GetTime::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::GetTime>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Get Time",
            .packageName = RASTER_PACKAGED "get_time",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}