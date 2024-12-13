#include "common/transform2d.h"

namespace Raster {
    Transform2D::Transform2D() {
        this->position = this->anchor = glm::vec2(0);
        this->size = glm::vec2(1);
        this->angle = 0.0f;
        this->scale = 1.0f;

        this->parentTransform = nullptr;
    }

    Transform2D::Transform2D(Json t_data) {
        this->position = {
            t_data["Position"][0], t_data["Position"][1]
        };
        this->size = {
            t_data["Size"][0], t_data["Size"][1]
        };
        this->anchor = {
            t_data["Anchor"][0], t_data["Anchor"][1]
        };
        if (t_data.contains("Scale")) this->scale = t_data["Scale"];
        else this->scale = 1.0f;
        this->angle = t_data["Angle"];
        this->parentTransform = nullptr;
    }

    glm::mat4 Transform2D::GetTransformationMatrix() {
        glm::mat4 transform = glm::identity<glm::mat4>();

        transform = glm::translate(transform, glm::vec3(position, 0.0f));
        transform = glm::translate(transform, glm::vec3(anchor, 0.0f));
        transform = glm::rotate(transform, glm::radians(-angle), glm::vec3(0, 0, 1));
        transform = glm::translate(transform, glm::vec3(-anchor, 0.0f)); 
        transform = glm::scale(transform, glm::vec3(size * scale, 1.0f));  
        return GetParentMatrix() * transform;
    }

    glm::mat4 Transform2D::GetParentMatrix() {
        return (parentTransform == nullptr) ? glm::identity<glm::mat4>() : parentTransform->GetTransformationMatrix();
    }

    glm::vec2 Transform2D::DecomposePosition() {
        auto model = GetTransformationMatrix();
        return glm::vec2(model[3].x, model[3].y);
    }

    glm::vec2 Transform2D::DecomposeSize() {
        auto model = GetTransformationMatrix();
        glm::vec2 size;
        size.x = glm::length(glm::vec3(model[0])); // Basis vector X
        size.y = glm::length(glm::vec3(model[1])); // Basis vector Y
        return size;
    }

    float Transform2D::DecomposeRotation() {
        if (parentTransform != nullptr) {
            return parentTransform->DecomposeRotation() + angle;
        }
        return angle;
    }

    Json Transform2D::Serialize() {
        return {
            {"Position", {position.x, position.y}},
            {"Size", {size.x, size.y}},
            {"Anchor", {anchor.x, anchor.y}},
            {"Angle", angle},
            {"Scale", scale}
        };
    }
};