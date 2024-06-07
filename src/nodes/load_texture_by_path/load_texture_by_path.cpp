#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "load_texture_by_path.h"

namespace Raster {

    LoadTextureByPath::LoadTextureByPath() {
        NodeBase::Initialize();
        NodeBase::GenerateFlowPins();

        AddOutputPin("Texture");

        m_attributes["Path"] = std::string("");

        this->archive = std::nullopt;
    }

    AbstractPinMap LoadTextureByPath::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        if (!archive.has_value()) UpdateTextureArchive();
        
        if (archive.has_value()) {
            auto unpackedArchive = archive.value();
            auto path = GetAttribute<std::string>("Path").value_or("");
            if (path != unpackedArchive.path) UpdateTextureArchive();

            TryAppendAbstractPinMap(result, "Texture", unpackedArchive.texture);
        }

        return result;
    }

    void LoadTextureByPath::UpdateTextureArchive() {
        std::string path = GetAttribute<std::string>("Path").value_or("");
        if (std::filesystem::exists(path)) {
            archive = TextureArchive(
                GPU::ImportTexture(path.c_str()),
                path
            );
        }
    }

    void LoadTextureByPath::AbstractRenderProperties() {
        RenderAttributeProperty("Path");
    }

    std::string LoadTextureByPath::AbstractHeader() {
        return FormatString(" Load Texture By Path: %s", GetAttribute<std::string>("Path").value_or("").c_str());
    }

    std::optional<std::string> LoadTextureByPath::Footer() {
        return std::nullopt;
    }

    std::string LoadTextureByPath::Icon() {
        return ICON_FA_FOLDER_OPEN;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::LoadTextureByPath>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Load Texture By Path",
            .packageName = "packaged.raster.load_texture_by_path",
            .category = Raster::NodeCategory::Resources
        };
    }
}