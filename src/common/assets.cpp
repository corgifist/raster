#include "common/assets.h"

namespace Raster {
    std::vector<AssetImplementation> Assets::s_implementations;

    void Assets::Initialize() {
        if (!std::filesystem::exists("assets/")) {
            std::filesystem::create_directory("assets");
        }
        auto iterator = std::filesystem::directory_iterator("assets");
        for (auto &entry : iterator) {
            std::string transformedPath = std::regex_replace(
                GetBaseName(entry.path().string()), std::regex(".dll|.so|lib"), "");
            Libraries::LoadLibrary("assets", transformedPath);
            AssetImplementation implementation;
            implementation.description = Libraries::GetFunction<AssetDescription()>(transformedPath, "GetDescription")();
            implementation.spawn = Libraries::GetFunction<AbstractAsset()>(transformedPath, "SpawnAsset");
            try {
                Libraries::GetFunction<void()>(transformedPath, "OnStartup")();
            } catch (internalDylib::exception ex) {
                // skip
            }
            s_implementations.push_back(implementation);
            std::cout << "loading asset '" << implementation.description.packageName << "'" << std::endl;
        }
    }

    std::optional<AbstractAsset> Assets::InstantiateAsset(std::string t_packageName) {
        auto implementationCandidate = GetAssetImplementationByPackageName(t_packageName);
        if (implementationCandidate.has_value()) {
            auto& implementation = implementationCandidate.value();
            auto result = implementation.spawn();
            result->packageName = t_packageName;
            return result;
        }
        return std::nullopt;
    }

    std::optional<AbstractAsset> Assets::InstantiateSerializedAsset(Json t_data) {
        auto assetCandidate = InstantiateAsset(t_data["PackageName"]);
        if (assetCandidate.has_value()) {
            auto& asset = assetCandidate.value();
            asset->name = t_data["Name"];
            asset->id = t_data["ID"];
            if (t_data.contains("ColorMark")) {
                asset->colorMark = t_data["ColorMark"];
            }
            asset->Load(t_data["Data"]);
            return asset;
        }
        return std::nullopt;
    }

    std::optional<AbstractAsset> Assets::CopyAsset(AbstractAsset t_asset) {
        auto copyCandidate = InstantiateSerializedAsset(t_asset->Serialize());
        if (copyCandidate.has_value()) {
            auto& result = copyCandidate.value();
            result->id = Randomizer::GetRandomInteger();
            result->name += " (Copy)";
            return result;
        }
        return std::nullopt;
    }

    std::optional<AssetImplementation> Assets::GetAssetImplementationByPackageName(std::string t_packageName) {
        for (auto& impl : s_implementations) {
            if (impl.description.packageName == t_packageName) return impl;
        }
        return std::nullopt;
    }
};