#include "common/asset_base.h"
#include "common/workspace.h"

namespace Raster {

    AssetBase::AssetBase() {}

    AssetBase::~AssetBase() {}

    void AssetBase::Initialize() {
        this->id = Randomizer::GetRandomInteger();
        this->name = "New Asset";
        this->colorMark = Workspace::s_colorMarks[Workspace::s_defaultColorMark];
    }

    void AssetBase::RenderDetails() {
        AbstractRenderDetails();
    }
    
    void AssetBase::RenderPopup() {
        AbstractRenderPopup();
    }

    Json AssetBase::Serialize() {
        return {
            {"ID", id},
            {"Name", name},
            {"Data", AbstractSerialize()},
            {"PackageName", packageName},
            {"ColorMark", colorMark}
        };
    }


    std::optional<std::vector<std::shared_ptr<AssetBase>>*> AssetBase::GetChildAssets() {
        return AbstractGetChildAssets();
    }

    bool AssetBase::IsReady() {
        return AbstractIsReady();
    }

    void AssetBase::Import(std::string t_path) {
        AbstractImport(t_path);
    }

    std::optional<Texture> AssetBase::GetPreviewTexture() {
        return AbstractGetPreviewTexture();
    }
    
    std::optional<std::uintmax_t> AssetBase::GetSize() {
        return AbstractGetSize();
    }

    std::optional<std::string> AssetBase::GetResolution() {
        return AbstractGetResolution();
    }

    std::optional<std::string> AssetBase::GetDuration() {
        return AbstractGetDuration();
    }

    std::optional<std::string> AssetBase::GetPath() {
        return AbstractGetPath();
    }

    void AssetBase::RenderPreviewOverlay(glm::vec2 t_regionSize) {
        AbstractRenderPreviewOverlay(t_regionSize);
    }

    void AssetBase::OnTimelineDrop(float t_frame) {
        AbstractOnTimelineDrop(t_frame);
    }

    void AssetBase::Delete() {
        AbstractDelete();
    }

    void AssetBase::Load(Json t_data) {
        AbstractLoad(t_data);
    }
};