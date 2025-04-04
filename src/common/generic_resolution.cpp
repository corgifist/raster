#include "common/generic_resolution.h"
#include "common/workspace.h"

namespace Raster {
    GenericResolution::GenericResolution() {
        this->useRawResolution = false;
        this->useProjectAspectResolution = true;
        this->aspectRatio = 1.0f;
        this->useRawResolutionAsReference = false;
        this->rawResolution = glm::vec2(512);
    }
    
    GenericResolution::GenericResolution(Json t_data) {
        this->useRawResolution = t_data["UseRawResolution"];
        this->useProjectAspectResolution = t_data["UseProjectAspectResolution"];
        this->aspectRatio = t_data["AspectRatio"];
        this->useRawResolutionAsReference = t_data["UseRawResolutionAsReference"];
        this->rawResolution = glm::vec2(t_data["RawResolution"][0].get<float>(), t_data["RawResolution"][1].get<float>());
    }

    glm::vec2 GenericResolution::CalculateResolution() {
        if (useRawResolution) {
            return rawResolution;
        } else {
            if (!Workspace::IsProjectLoaded()) return rawResolution;
            auto& project = Workspace::GetProject();
            auto resolution = project.preferredResolution;
            float finalAspectRatio = aspectRatio;
            if (useProjectAspectResolution) {
                finalAspectRatio = resolution.x / resolution.y;
            }
            finalAspectRatio = 1.0f / finalAspectRatio;
            if (useRawResolutionAsReference) {
                resolution = rawResolution;
            } 
            resolution.y = resolution.x * finalAspectRatio;
            return resolution;
        }
    }

    Json GenericResolution::Serialize() {
        return {
            {"UseRawResolution", useRawResolution},
            {"UseProjectAspectResolution", useProjectAspectResolution},
            {"AspectRatio", aspectRatio},
            {"UseRawResolutionAsReference", useRawResolutionAsReference},
            {"RawResolution", {rawResolution.x, rawResolution.y}}
        };
    }
};