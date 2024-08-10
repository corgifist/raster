#include "common/easings.h"

namespace Raster {
    std::vector<EasingImplementation> Easings::s_implementations;

    void Easings::Initialize() {
        if (!std::filesystem::exists("easings/")) {
            std::filesystem::create_directory("easings");
        }
        auto iterator = std::filesystem::directory_iterator("easings");
        for (auto &entry : iterator) {
            std::string transformedPath = std::regex_replace(
                GetBaseName(entry.path().string()), std::regex(".dll|.so|lib"), "");
            Libraries::LoadLibrary("easings", transformedPath);
            EasingImplementation implementation;
            implementation.description = Libraries::GetFunction<EasingDescription()>(transformedPath, "GetDescription")();
            implementation.spawn = Libraries::GetFunction<AbstractEasing()>(transformedPath, "SpawnEasing");
            try {
                Libraries::GetFunction<void()>(transformedPath, "OnStartup")();
            } catch (internalDylib::exception ex) {
                // skip
            }
            s_implementations.push_back(implementation);
            std::cout << "loading easing '" << implementation.description.packageName << "'" << std::endl;
        }
    }

    std::optional<AbstractEasing> Easings::InstantiateEasing(std::string t_packageName) {
        auto implementationCandidate = GetEasingImplementation(t_packageName);
        if (implementationCandidate.has_value()) {
            auto& implementation = implementationCandidate.value();
            auto result = implementation.spawn();
            result->packageName = t_packageName;
            result->prettyName = implementation.description.prettyName;
            return result;
        }
        return std::nullopt;
    }

    std::optional<AbstractEasing> Easings::InstantiateSerializedEasing(Json t_data) {
        auto easingCandidate = InstantiateEasing(t_data["PackageName"]);
        if (easingCandidate.has_value()) {
            auto& easing = easingCandidate.value();
            easing->id = t_data["ID"];
            easing->Load(t_data["Data"]);
            return easing;
        }
        return std::nullopt;
    }

    std::optional<EasingImplementation> Easings::GetEasingImplementation(std::string t_packageName) {
        for (auto& impl : s_implementations) {
            if (impl.description.packageName == t_packageName) return impl;
        }
        return std::nullopt;
    }
};