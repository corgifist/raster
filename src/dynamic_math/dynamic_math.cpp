#include "dynamic_math/dynamic_math.h"

#define MULTIPLY_CLAUSE(a_type, b_type) \
    if (t_a.type() == typeid(a_type) && t_b.type() == typeid(b_type)) return std::any_cast<a_type>(t_a) * std::any_cast<b_type>(t_b);

#define SINE_CLAUSE(t_type) \
    if (t_value.type() == typeid(t_type)) return glm::sin(std::any_cast<t_type>(t_value));

namespace Raster {
    std::optional<std::any> DynamicMath::Sine(std::any t_value) {
        SINE_CLAUSE(float);
        SINE_CLAUSE(glm::vec4);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Multiply(std::any t_a, std::any t_b) {
        MULTIPLY_CLAUSE(float, float);
        MULTIPLY_CLAUSE(glm::vec4, float);
        MULTIPLY_CLAUSE(glm::vec4, glm::vec4);
        return std::nullopt;
    }
};