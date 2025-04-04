#include "swizzle_vector.h"

#define TYPE_EQUALS(t_type, t_value) \
    ((t_value).type() == typeid(t_type))

#define GET_VECTOR_ELEMENT(t_type, t_value, t_index) \
    if (TYPE_EQUALS(t_type, t_value)) \
        return std::any_cast<t_type>(t_value)[t_index];

namespace Raster {

    SwizzleVector::SwizzleVector() {
        NodeBase::Initialize();

        SetupAttribute("Value", glm::vec4(0));
        SetupAttribute("SwizzleMask", std::string("XYZW"));

        AddOutputPin("Output");
    }

    AbstractPinMap SwizzleVector::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto valueCandidate = GetDynamicAttribute("Value", t_contextData);
        auto swizzleMaskCandidate = GetAttribute<std::string>("SwizzleMask", t_contextData);
        if (valueCandidate.has_value() && swizzleMaskCandidate.has_value() && CanBeSwizzled(valueCandidate.value())) {
            auto& value = valueCandidate.value();
            auto& swizzleMask = swizzleMaskCandidate.value();
            int vectorSize = GetVectorSize(value);
            swizzleMask = LowerCase(swizzleMask);
            swizzleMask = ReplaceString(swizzleMask, "[^x|y|z|w|s|t|r|g|b|a]", "");
            m_lastSwizzleMask = swizzleMask;

            static std::unordered_map<char, int> s_swizzleIndexes = {
                {'x', 0},
                {'r', 0},
                {'s', 0},
                {'y', 1},
                {'g', 1},
                {'t', 1},
                {'z', 2},
                {'b', 2},
                {'w', 3},
                {'a', 3}
            };

            std::vector<float> swizzledVector;
            for (int i = 0; i < swizzleMask.length(); i++) {
                auto swizzleChar = swizzleMask[i];
                if (s_swizzleIndexes.find(swizzleChar) == s_swizzleIndexes.end()) continue;
                int swizzleIndex = s_swizzleIndexes[swizzleChar];
                if (swizzleIndex < vectorSize) {
                    swizzledVector.push_back(GetVectorElement(value, swizzleIndex));
                }
            }
            if (!swizzledVector.empty()) {
                std::any constructedVector = std::nullopt;
                switch (swizzledVector.size()) {
                    case 2: {
                        constructedVector = glm::make_vec2(swizzledVector.data());
                        break;
                    }
                    case 3: {
                        constructedVector = glm::make_vec3(swizzledVector.data());
                        break;
                    }
                    case 4: {
                        constructedVector = glm::make_vec4(swizzledVector.data());
                        break;
                    }
                }
                if (constructedVector.type() != typeid(std::nullopt)) {
                    TryAppendAbstractPinMap(result, "Output", constructedVector);
                }
            }

        }
        return result;
    }

    bool SwizzleVector::CanBeSwizzled(std::any t_value) {
        return TYPE_EQUALS(glm::vec2, t_value) || TYPE_EQUALS(glm::vec3, t_value) || TYPE_EQUALS(glm::vec4, t_value);
    }

    int SwizzleVector::GetVectorSize(std::any t_value) {
        if (!CanBeSwizzled(t_value)) return 0;
        if (TYPE_EQUALS(glm::vec2, t_value)) return 2;
        if (TYPE_EQUALS(glm::vec3, t_value)) return 3;
        if (TYPE_EQUALS(glm::vec4, t_value)) return 4;
        return 0;
    }

    float SwizzleVector::GetVectorElement(std::any t_value, int t_index) {
        GET_VECTOR_ELEMENT(glm::vec2, t_value, t_index);
        GET_VECTOR_ELEMENT(glm::vec3, t_value, t_index);
        GET_VECTOR_ELEMENT(glm::vec4, t_value, t_index);
        return 0;
    }

    void SwizzleVector::AbstractRenderProperties() {
        RenderAttributeProperty("Value");
        RenderAttributeProperty("SwizzleMask");
    }

    void SwizzleVector::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json SwizzleVector::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool SwizzleVector::AbstractDetailsAvailable() {
        return false;
    }

    std::string SwizzleVector::AbstractHeader() {
        auto swizzleMaskCandidate = m_lastSwizzleMask;
        return swizzleMaskCandidate.has_value() ? FormatString("Swizzle Vector: %s", swizzleMaskCandidate.value().c_str()) : "Swizzle Vector";
    }

    std::string SwizzleVector::Icon() {
        return ICON_FA_EXPAND;
    }

    std::optional<std::string> SwizzleVector::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SwizzleVector>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Swizzle Vector",
            .packageName = RASTER_PACKAGED "swizzle_vector",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}