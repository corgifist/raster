#include "hue_to_rgb.h"

#include <glm/gtc/integer.hpp>

#define MIN3(t_a, t_b, t_c) \
    glm::min(glm::min((t_a), (t_b)), (t_c))

namespace Raster {

    HueToRGB::HueToRGB() {
        NodeBase::Initialize();

        SetupAttribute("Hue", 0.0f);

        AddOutputPin("RGB");
    }

    AbstractPinMap HueToRGB::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto hueCandidate = GetAttribute<float>("Hue");
        if (hueCandidate.has_value()) {
            auto hue = hueCandidate.value();
            
            auto kr = glm::mod(5.0f + hue * 6.0f, 6.0f);
            auto kg = glm::mod(3.0f + hue * 6.0f, 6.0f);
            auto kb = glm::mod(1.0f + hue * 6.0f, 6.0f);

            auto r = 1.0f - glm::max(MIN3(kr, 4.0f - kr, 1.0f), 0.0f);
            auto g = 1.0f - glm::max(MIN3(kg, 4.0f - kg, 1.0f), 0.0f);
            auto b = 1.0f - glm::max(MIN3(kb, 4.0f - kb, 1.0f), 0.0f);

            TryAppendAbstractPinMap(result, "RGB", glm::vec3(r, g, b));
        }
        return result;
    }

    void HueToRGB::AbstractRenderProperties() {
        RenderAttributeProperty("Hue");
    }

    void HueToRGB::AbstractLoadSerialized(Json t_data) {
        RASTER_DESERIALIZE_WRAPPER(float, "Hue");
    }

    Json HueToRGB::AbstractSerialize() {
        return {
            RASTER_SERIALIZE_WRAPPER(float, "Hue")
        };
    }

    bool HueToRGB::AbstractDetailsAvailable() {
        return false;
    }

    std::string HueToRGB::AbstractHeader() {
        return "Hue To RGB";
    }

    std::string HueToRGB::Icon() {
        return ICON_FA_DROPLET;
    }

    std::optional<std::string> HueToRGB::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::HueToRGB>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Hue to RGB",
            .packageName = RASTER_PACKAGED "hue_to_rgb",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}