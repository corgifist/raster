#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "load_texture_by_path.h"

namespace Raster {

    LoadTextureByPath::LoadTextureByPath() {
        NodeBase::Initialize();

        AddOutputPin("Texture");

        SetupAttribute("Path", std::string(""));

        this->archive = std::nullopt;
        this->m_asyncUploadID = 0;
    }

    LoadTextureByPath::~LoadTextureByPath() {
        if (archive.has_value()) {
            auto& archiveValue = archive.value();
            GPU::DestroyTexture(archiveValue.texture);
        }
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
        if (std::filesystem::exists(path) && !std::filesystem::is_directory(path)) {
            if (!m_loader.IsInitialized() && !m_asyncUploadID) {
                m_loader = AsyncImageLoader(path);
                if (archive.has_value()) {
                    auto& textureArchive = archive.value();
                    AsyncUpload::DestroyTexture(textureArchive.texture);
                }
                archive = std::nullopt;
            }
            if (m_loader.IsReady()) {
                auto imageCandidate = m_loader.Get();
                if (imageCandidate.has_value()) {
                    auto& image = imageCandidate.value();
                    
                    TexturePrecision precision = TexturePrecision::Usual;
                    if (image->precision == ImagePrecision::Half) precision = TexturePrecision::Half;
                    if (image->precision == ImagePrecision::Full) precision = TexturePrecision::Full;

                    if (!m_asyncUploadID) {
                        m_asyncUploadID = AsyncUpload::GenerateTextureFromImage(image);
                        m_loader = AsyncImageLoader();
                        std::cout << "requesting async upload" << std::endl;
                    }
                }
            }
            if (AsyncUpload::IsUploadReady(m_asyncUploadID)) {
                auto& info = AsyncUpload::GetUpload(m_asyncUploadID);
                archive = TextureArchive(info.texture, path);
                AsyncUpload::DestroyUpload(m_asyncUploadID);
            }
        }
    }

    bool LoadTextureByPath::AbstractDetailsAvailable() {
        return false;
    }

    void LoadTextureByPath::AbstractRenderProperties() {
        RenderAttributeProperty("Path");
    }

    std::string LoadTextureByPath::AbstractHeader() {
        std::string base = FormatString("Load Texture By Path: %s", GetAttribute<std::string>("Path").value_or("").c_str());
        if (m_loader.IsInitialized() && !m_loader.IsReady()) {
            base = ICON_FA_SPINNER + (" " + base);
        }
        return base;
    }

    void LoadTextureByPath::AbstractLoadSerialized(Json t_data) {
        SetAttributeValue("Path", t_data["Path"].get<std::string>());
    }

    Json LoadTextureByPath::AbstractSerialize() {
        return {
            {"Path", RASTER_ATTRIBUTE_CAST(std::string, "Path")}
        };
    }

    std::optional<std::string> LoadTextureByPath::Footer() {
        return std::nullopt;
    }

    std::string LoadTextureByPath::Icon() {
        return ICON_FA_FOLDER_OPEN;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::LoadTextureByPath>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Load Texture By Path",
            .packageName = "packaged.raster.load_texture_by_path",
            .category =Raster::DefaultNodeCategories::s_resources
        };
    }
}