#include "common/transform3d.h"

namespace Raster {
    Transform3D::Transform3D() {
        this->position = this->anchor = this->rotation = glm::vec3(0);
        this->size = glm::vec3(1);
        this->scale = 1.0f;

        this->parentTransform = nullptr;
    }

    Transform3D::Transform3D(Json t_data) {
        this->position = {
            t_data["Position"][0], t_data["Position"][1], t_data["Position"][2]
        };
        this->size = {
            t_data["Size"][0], t_data["Size"][1], t_data["Size"][2]
        };
        this->anchor = {
            t_data["Anchor"][0], t_data["Anchor"][1], t_data["Anchor"][2]
        };
        this->rotation = {
            t_data["Rotation"][0], t_data["Rotation"][1], t_data["Rotation"][2]
        };
        if (t_data.contains("Scale")) this->scale = t_data["Scale"];
        else this->scale = 1.0f;
        this->parentTransform = nullptr;
    }

    glm::mat4 Transform3D::GetTransformationMatrix() {
        auto transform = glm::identity<glm::mat4>();
        transform = glm::translate(transform, position);
        transform = glm::translate(transform, anchor);
        transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(1, 0, 0));
        transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        transform = glm::translate(transform, -anchor); 
        transform = glm::scale(transform, size * scale); 

        return GetParentMatrix() * transform;
    }

    glm::mat4 Transform3D::GetParentMatrix() {
        return (parentTransform == nullptr) ? glm::identity<glm::mat4>() : parentTransform->GetTransformationMatrix();
    }

    glm::vec3 Transform3D::DecomposePosition() {
        auto model = GetTransformationMatrix();
        return glm::vec3(model[3].x, model[3].y, model[3].z);
    }

    glm::vec3 Transform3D::DecomposeSize() {
        auto model = GetTransformationMatrix();
        glm::vec3 size;
        size.x = glm::length(glm::vec3(model[0])); // Basis vector X
        size.y = glm::length(glm::vec3(model[1])); // Basis vector Y
        size.z = glm::length(glm::vec3(model[2]));
        return size;
    }

    glm::vec3 Transform3D::DecomposeRotation() {
        if (parentTransform != nullptr) {
            return parentTransform->DecomposeRotation() + rotation;
        }
        return rotation;
    }

    Json Transform3D::Serialize() {
        return {
            {"Position", {position.x, position.y, position.z}},
            {"Size", {size.x, size.y, size.z}},
            {"Anchor", {anchor.x, anchor.y, anchor.z}},
            {"Rotation", {rotation.x, rotation.y, rotation.z}},
            {"Scale", scale}
        };
    }
};