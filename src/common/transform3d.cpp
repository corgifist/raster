#include "common/transform3d.h"

namespace Raster {
    Transform3D::Transform3D() {
        this->position = this->rotation = glm::vec3(0);
        this->size = glm::vec3(1);

        this->parentTransform = nullptr;
    }

    Transform3D::Transform3D(Json t_data) {
        if (t_data.contains("Position")) this->position = {
            t_data["Position"][0], t_data["Position"][1], t_data["Position"][2]
        }; else this->position = glm::vec3(0);
        if (t_data.contains("Size")) this->size = {
            t_data["Size"][0], t_data["Size"][1], t_data["Size"][2]
        }; else this->size = glm::vec3(1);
        if (t_data.contains("Rotation")) this->rotation = {
            t_data["Rotation"][0], t_data["Rotation"][1], t_data["Rotation"][2]
        }; else this->rotation = glm::vec3(0);
        this->parentTransform = nullptr;
    }

    glm::mat4 Transform3D::GetTransformationMatrix() {
        return GetParentMatrix() * GetBaseMatrix();
    }

    glm::mat4 Transform3D::GetBaseMatrix() {
        auto transform = glm::identity<glm::mat4>();
        transform = glm::translate(transform, position * glm::vec3(1, -1, -1));
        transform = glm::rotate(transform, glm::radians(rotation.y), glm::vec3(1, 0, 0));
        transform = glm::rotate(transform, glm::radians(rotation.x), glm::vec3(0, 1, 0));
        transform = glm::rotate(transform, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        transform = glm::scale(transform, size); 
        return transform;
    }

    glm::mat4 Transform3D::GetParentMatrix() {
        return (parentTransform == nullptr) ? glm::identity<glm::mat4>() : parentTransform->GetTransformationMatrix();
    }

    glm::vec3 Transform3D::DecomposePosition() {
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(GetTransformationMatrix(), scale, rotation, translation, skew, perspective);
        return translation;
    }

    glm::vec3 Transform3D::DecomposeSize() {
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(GetTransformationMatrix(), scale, rotation, translation, skew, perspective);
        return scale;
    }

    glm::vec3 Transform3D::DecomposeRotation() {
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(GetTransformationMatrix(), scale, rotation, translation, skew, perspective);
        return translation;
        if (parentTransform != nullptr) {
            return parentTransform->DecomposeRotation() + glm::degrees(glm::eulerAngles(rotation));
        }
        return glm::degrees(glm::eulerAngles(rotation));
    }

    
    glm::vec3 Transform3D::DecomposeBasePosition() {
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(GetBaseMatrix(), scale, rotation, translation, skew, perspective);
        return translation;
    }

    glm::vec3 Transform3D::DecomposeBaseSize() {
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(GetBaseMatrix(), scale, rotation, translation, skew, perspective);
        return scale;
    }

    glm::vec3 Transform3D::DecomposeBaseRotation() {
        glm::vec3 scale;
        glm::quat rotation;
        glm::vec3 translation;
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(GetBaseMatrix(), scale, rotation, translation, skew, perspective);
        return translation;
        if (parentTransform != nullptr) {
            return parentTransform->DecomposeRotation() + glm::degrees(glm::eulerAngles(rotation));
        }
        return glm::degrees(glm::eulerAngles(rotation));
    }

    Json Transform3D::Serialize() {
        return {
            {"Position", {position.x, position.y, position.z}},
            {"Size", {size.x, size.y, size.z}},
            {"Rotation", {rotation.x, rotation.y, rotation.z}},
        };
    }
};