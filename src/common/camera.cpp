#include "common/camera.h"
#include "common/workspace.h"
#include "../ImGui/ImGui.h"
#include "../ImGui/ImGuizmo.h"

namespace Raster {
    Camera::Camera(Json t_data) {
        this->perspNear = t_data["PerspNear"];
        this->perspFar = t_data["PerspFar"];
        this->customF = t_data["CustomF"];
        this->f = t_data["F"];
        this->fSize = t_data["FSize"];
        this->fLength = t_data["FLength"];
        this->persp = t_data["Persp"];
        this->orthoWidth = t_data["OrthoWidth"];
        this->position = {t_data["Position"][0].get<float>(), t_data["Position"][1].get<float>(), t_data["Position"][2].get<float>()};
        this->rotation = {t_data["Rotation"][0].get<float>(), t_data["Rotation"][1].get<float>(), t_data["Rotation"][2].get<float>()};
        this->anchor = {t_data["Anchor"][0].get<float>(), t_data["Anchor"][1].get<float>(), t_data["Anchor"][2].get<float>()};
        this->enabled = t_data["Enabled"];
    }

    float Camera::GetF() {
        if (customF) return f;
        return glm::degrees(2 * glm::atan(fSize / (2.0f * fLength)));
    }

    glm::mat4 Camera::GetProjectionMatrix() {
        auto& project = Workspace::GetProject();
        float aspect = project.preferredResolution.x / project.preferredResolution.y;
        if (!persp) {
            aspect *= orthoWidth;
            return glm::ortho(-aspect, aspect, 1.0f, -1.0f, -1.0f, 1000.0f);
        }
        float f = GetF();
        return glm::perspective(glm::radians(f), aspect, perspNear, perspFar);
    }

    glm::mat4 Camera::GetTransformationMatrix() {
        auto transform = glm::identity<glm::mat4>();
        transform = glm::translate(transform, position);
        transform = glm::translate(transform, anchor);
        transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(1, 0, 0));
        transform = glm::translate(transform, -anchor); 
        return transform;
    }

    Json Camera::Serialize() {
        return {
            {"Position", {position.x, position.y, position.z}},
            {"Rotation", {rotation.x, rotation.y, rotation.z}},
            {"Anchor", {anchor.x, anchor.y, anchor.z}},
            {"PerspNear", perspNear},
            {"PerspFar", perspFar},
            {"F", f},
            {"CustomF", customF},
            {"FSize", fSize},
            {"FLength", fLength},
            {"Persp", persp},
            {"OrthoWidth", orthoWidth},
            {"Enabled", enabled}
        };
    }

};