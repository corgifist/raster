#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "make_sampler_settings.h"

namespace Raster {


    MakeSamplerSettings::MakeSamplerSettings() {
        NodeBase::Initialize();

        AddOutputPin("Value");

        SetupAttribute("TextureFiltering", static_cast<int>(TextureFilteringMode::Linear));
        SetupAttribute("TextureWrapping", static_cast<int>(TextureWrappingMode::Repeat));
    }

    AbstractPinMap MakeSamplerSettings::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto textureFilteringCandidate = GetAttribute<int>("TextureFiltering", t_contextData);
        auto textureWrappingCandidate = GetAttribute<int>("TextureWrapping", t_contextData);
        m_lastTextureFiltering = textureFilteringCandidate;
        m_lastTextureWrapping = textureWrappingCandidate;
        if (textureFilteringCandidate.has_value() && textureWrappingCandidate.has_value()) {
            SamplerSettings settings;
            settings.filteringMode = static_cast<TextureFilteringMode>(textureFilteringCandidate.value());
            settings.wrappingMode = static_cast<TextureWrappingMode>(textureWrappingCandidate.value());

            TryAppendAbstractPinMap(result, "Value", settings);
        }

        return result;
    }

    void MakeSamplerSettings::AbstractRenderProperties() {
        auto textureFilteringCandidate = m_lastTextureFiltering;
        auto textureWrappingCandidate = m_lastTextureWrapping;
        if (textureFilteringCandidate.has_value() && textureWrappingCandidate.has_value()) {
            SamplerSettings settings;
            settings.filteringMode = static_cast<TextureFilteringMode>(textureFilteringCandidate.value());
            settings.wrappingMode = static_cast<TextureWrappingMode>(textureWrappingCandidate.value());

            // FIXME: restore this functionallity
            
            /* this->m_attributes.GetFrontValue()["SamplerSettings"] = settings;
            RenderAttributeProperty("SamplerSettings");
            auto modifiedSettings = std::any_cast<SamplerSettings>(this->m_attributes.GetFrontValue()["SamplerSettings"]);
            this->m_attributes.GetFrontValue().erase("SamplerSettings");
            this->m_attributes.GetFrontValue()["TextureFiltering"] = static_cast<int>(modifiedSettings.filteringMode);
            this->m_attributes.GetFrontValue()["TextureWrapping"] = static_cast<int>(modifiedSettings.wrappingMode); */
        }
    }

    void MakeSamplerSettings::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data); 
    }

    Json MakeSamplerSettings::AbstractSerialize() {
        return SerializeAllAttributes();
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
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MakeSamplerSettings>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Make Sampler Settings",
            .packageName = RASTER_PACKAGED "make_sampler_settings",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}