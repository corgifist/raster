#include "common/transform2d.h"

namespace Raster {
    Transform2D::Transform2D() {
        this->position = this->anchor = glm::vec2(0);
        this->size = glm::vec2(1);
        this->angle = 0.0f;
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
        this->angle = t_data["Angle"];
    }

    glm::mat4 Transform2D::GetTransformationMatrix() {
        glm::mat4 transform = glm::identity<glm::mat4>();
        /* transform = glm::translate(transform, glm::vec3(anchor, 0.0f));
        transform = glm::rotate(transform, glm::radians(angle), glm::vec3(0, 0, 1));
        transform = glm::translate(transform, glm::vec3(-anchor, 0.0f));
        transform = glm::scale(transform, glm::vec3(size, 1.0f));
        transform = glm::translate(transform, glm::vec3(position, 0.0f)); */

        transform = glm::translate(transform, glm::vec3(position, 0.0f));
        transform = glm::translate(transform, glm::vec3(anchor, 0.0f));
        transform = glm::rotate(transform, glm::radians(angle), glm::vec3(0, 0, 1));
        transform = glm::translate(transform, glm::vec3(-anchor, 0.0f));
        transform = glm::scale(transform, glm::vec3(size, 1.0f));   
        return transform;
    }

    glm::vec2 Transform2D::GetTransformedPosition() {
        glm::vec3 pos = GetTransformationMatrix()[3];
        return {pos.x, pos.y};
    }

    glm::vec2 Transform2D::GetTransformedSize() {
        auto mat = GetTransformationMatrix();
        return glm::vec2{
            glm::length(glm::vec3(mat[0])),
            glm::length(glm::vec3(mat[1]))
        };
    }

    Json Transform2D::Serialize() {
        return {
            {"Position", {position.x, position.y}},
            {"Size", {size.x, size.y}},
            {"Anchor", {anchor.x, anchor.y}},
            {"Angle", angle}
        };
    }
};