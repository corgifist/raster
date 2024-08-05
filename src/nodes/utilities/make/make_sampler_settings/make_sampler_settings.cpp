#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "make_sampler_settings.h"

namespace Raster {


    MakeSamplerSettings::MakeSamplerSettings() {
        NodeBase::Initialize();

        AddOutputPin("Value");

        this->m_attributes["TextureFiltering"] = static_cast<int>(TextureFilteringMode::Linear);
        this->m_attributes["TextureWrapping"] = static_cast<int>(TextureWrappingMode::Repeat);

        SetupAttribute("TextureFiltering", static_cast<int>(TextureFilteringMode::Linear));
        SetupAttribute("TextureWrapping", static_cast<int>(TextureWrappingMode::Repeat));
    }

    AbstractPinMap MakeSamplerSettings::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto textureFilteringCandidate = GetAttribute<int>("TextureFiltering");
        auto textureWrappingCandidate = GetAttribute<int>("TextureWrapping");
        if (textureFilteringCandidate.has_value() && textureWrappingCandidate.has_value()) {
            SamplerSettings settings;
            settings.filteringMode = static_cast<TextureFilteringMode>(textureFilteringCandidate.value());
            settings.wrappingMode = static_cast<TextureWrappingMode>(textureWrappingCandidate.value());

            TryAppendAbstractPinMap(result, "Value", settings);
        }

        return result;
    }

    void MakeSamplerSettings::AbstractRenderProperties() {
        auto textureFilteringCandidate = GetAttribute<int>("TextureFiltering");
        auto textureWrappingCandidate = GetAttribute<int>("TextureWrapping");
        if (textureFilteringCandidate.has_value() && textureWrappingCandidate.has_value()) {
            SamplerSettings settings;
            settings.filteringMode = static_cast<TextureFilteringMode>(textureFilteringCandidate.value());
            settings.wrappingMode = static_cast<TextureWrappingMode>(textureWrappingCandidate.value());

            this->m_attributes["SamplerSettings"] = settings;
            RenderAttributeProperty("SamplerSettings");
            auto modifiedSettings = std::any_cast<SamplerSettings>(this->m_attributes["SamplerSettings"]);
            this->m_attributes.erase("SamplerSettings");
            this->m_attributes["TextureFiltering"] = static_cast<int>(modifiedSettings.filteringMode);
            this->m_attributes["TextureWrapping"] = static_cast<int>(modifiedSettings.wrappingMode);
        }
    }

    bool MakeSamplerSettings::AbstractDetailsAvailable() {
        return false;
    }

    std::string MakeSamplerSettings::AbstractHeader() {
        return "Make Sampler Settings";
    }

    std::string MakeSamplerSettings::Icon() {
        return ICON_FA_GEARS;
    }

    std::optional<std::string> MakeSamplerSettings::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MakeSamplerSettings>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Make Sampler Settings",
            .packageName = RASTER_PACKAGED "make_sampler_settings",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}