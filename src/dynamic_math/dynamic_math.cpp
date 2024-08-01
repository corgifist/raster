#include "dynamic_math/dynamic_math.h"

#define BINARY_CLAUSE(a_type, b_type, operation) \
    if (t_a.type() == typeid(a_type) && t_b.type() == typeid(b_type)) return std::any_cast<a_type>(t_a) operation std::any_cast<b_type>(t_b)

#define MULTIPLY_CLAUSE(a_type, b_type) \
    BINARY_CLAUSE(a_type, b_type, *)

#define DIVIDE_CLAUSE(a_type, b_type) \
    BINARY_CLAUSE(a_type, b_type, /)

#define ADD_CLAUSE(a_type, b_type) \
    BINARY_CLAUSE(a_type, b_type, +)

#define SUBTRACT_CLAUSE(a_type, b_type) \
    BINARY_CLAUSE(a_type, b_type, -)

#define BINARY_TYPE_CLAUSES(clause_type) \
    clause_type(float, float); \
    clause_type(glm::vec4, float); \
    clause_type(glm::vec4, glm::vec4);

#define SINGLE_GLM_FUNCTION_CLAUSE(t_type, function) \
    if (t_value.type() == typeid(t_type)) return function(std::any_cast<t_type>(t_value));

#define SINE_CLAUSE(t_type) \
    SINGLE_GLM_FUNCTION_CLAUSE(t_type, glm::sin)

#define ABS_CLAUSE(t_type) \
    SINGLE_GLM_FUNCTION_CLAUSE(t_type, glm::abs)

#define SINGLE_VALUE_CLAUSES(clause_type) \
    clause_type(float); \
    clause_type(glm::vec2); \
    clause_type(glm::vec3); \
    clause_type(glm::vec4); 

namespace Raster {
    std::optional<std::any> DynamicMath::Sine(std::any t_value) {
        SINGLE_VALUE_CLAUSES(SINE_CLAUSE);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Abs(std::any t_value) {
        SINGLE_VALUE_CLAUSES(ABS_CLAUSE);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Multiply(std::any t_a, std::any t_b) {
        MULTIPLY_CLAUSE(float, float);
        MULTIPLY_CLAUSE(glm::vec4, float);
        MULTIPLY_CLAUSE(glm::vec4, glm::vec4);
        BINARY_TYPE_CLAUSES(MULTIPLY_CLAUSE);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Divide(std::any t_a, std::any t_b) {
        BINARY_TYPE_CLAUSES(DIVIDE_CLAUSE);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Add(std::any t_a, std::any t_b) {
        BINARY_TYPE_CLAUSES(ADD_CLAUSE);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Subtract(std::any t_a, std::any t_b) {
        BINARY_TYPE_CLAUSES(SUBTRACT_CLAUSE);
        return std::nullopt;
    }
};