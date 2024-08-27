#include "dynamic_math/dynamic_math.h"

/*        IF IT WORKS, DON'T TOUCH IT       */
/* HIGHLY CONFUSING PREPROCESSOR CODE AHEAD */

#define TYPE_EQUALS(t_a, t_type) \
    t_a.type() == typeid(t_type)

#define ANY_CAST(t_any, t_type) \
    std::any_cast<t_type>(t_any)

#define BINARY_CLAUSE(a_type, b_type, operation)              \
    if (TYPE_EQUALS(t_a, a_type) && TYPE_EQUALS(t_b, b_type)) \
        return ANY_CAST(t_a, a_type) operation ANY_CAST(t_b, b_type)

#define MULTIPLY_CLAUSE(a_type, b_type) \
    BINARY_CLAUSE(a_type, b_type, *)

#define DIVIDE_CLAUSE(a_type, b_type) \
    BINARY_CLAUSE(a_type, b_type, /)

#define ADD_CLAUSE(a_type, b_type) \
    BINARY_CLAUSE(a_type, b_type, +)

#define SUBTRACT_CLAUSE(a_type, b_type) \
    BINARY_CLAUSE(a_type, b_type, -)

#define MIX_CLAUSE(a_type, b_type, percentage_type)                                                         \
    if (TYPE_EQUALS(t_a, a_type) && TYPE_EQUALS(t_b, b_type) && TYPE_EQUALS(t_percentage, percentage_type)) \
    return glm::mix(ANY_CAST(t_a, a_type), ANY_CAST(t_b, b_type), ANY_CAST(t_percentage, percentage_type))

#define VECTOR_FLOAT_CLAUSE(clause_type, t_type) \
    clause_type(t_type, float);                  \
    clause_type(float, t_type);                  \
    clause_type(t_type, t_type);

#define BINARY_TYPE_CLAUSES(clause_type)         \
    clause_type(float, float);                   \
    VECTOR_FLOAT_CLAUSE(clause_type, glm::vec4); \
    VECTOR_FLOAT_CLAUSE(clause_type, glm::vec3); \
    VECTOR_FLOAT_CLAUSE(clause_type, glm::vec2);

#define MIX_VECTOR_CLAUSE(clause_type, t_type) \
    clause_type(t_type, t_type, float);        \
    clause_type(t_type, t_type, t_type)

#define MIX_TYPE_CLAUSES(clause_type)          \
    clause_type(float, float, float);          \
    MIX_VECTOR_CLAUSE(clause_type, glm::vec2); \
    MIX_VECTOR_CLAUSE(clause_type, glm::vec3); \
    MIX_VECTOR_CLAUSE(clause_type, glm::vec4)

#define SINGLE_GLM_FUNCTION_CLAUSE(t_type, function) \
    if (t_value.type() == typeid(t_type))            \
        return function(std::any_cast<t_type>(t_value));

#define SINE_CLAUSE(t_type) \
    SINGLE_GLM_FUNCTION_CLAUSE(t_type, glm::sin)

#define ABS_CLAUSE(t_type) \
    SINGLE_GLM_FUNCTION_CLAUSE(t_type, glm::abs)

#define SINGLE_VALUE_CLAUSES(clause_type) \
    clause_type(float);                   \
    clause_type(glm::vec2);               \
    clause_type(glm::vec3);               \
    clause_type(glm::vec4);

namespace Raster
{
    std::optional<std::any> DynamicMath::Sine(std::any t_value)
    {
        SINGLE_VALUE_CLAUSES(SINE_CLAUSE);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Abs(std::any t_value)
    {
        SINGLE_VALUE_CLAUSES(ABS_CLAUSE);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Multiply(std::any t_a, std::any t_b)
    {
        BINARY_TYPE_CLAUSES(MULTIPLY_CLAUSE);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Divide(std::any t_a, std::any t_b)
    {
        BINARY_TYPE_CLAUSES(DIVIDE_CLAUSE);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Add(std::any t_a, std::any t_b)
    {
        BINARY_TYPE_CLAUSES(ADD_CLAUSE);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Subtract(std::any t_a, std::any t_b)
    {
        BINARY_TYPE_CLAUSES(SUBTRACT_CLAUSE);
        return std::nullopt;
    }

    std::optional<std::any> DynamicMath::Mix(std::any t_a, std::any t_b, std::any t_percentage)
    {
        MIX_TYPE_CLAUSES(MIX_CLAUSE);
        return std::nullopt;
    }
};