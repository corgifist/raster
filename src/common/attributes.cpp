#include "common/attributes.h"
#include "common/randomizer.h"
#include "common/libraries.h"
#include "common/easings.h"

namespace Raster {
    std::vector<AttributeImplementation> Attributes::s_implementations;

    std::optional<AttributeImplementation> Attributes::GetAttributeImplementationByPackageName(std::string t_packageName) {
        for (auto& entry : s_implementations) {
            if (entry.description.packageName == t_packageName) return entry;
        }
        return std::nullopt;
    }

    std::optional<AbstractAttribute> Attributes::InstantiateAttribute(std::string t_packageName) {
        auto description = GetAttributeImplementationByPackageName(t_packageName);
        if (description.has_value()) {
            auto attribute = description.value().spawn();
            attribute->packageName = t_packageName;
            return attribute;
        }
        return std::nullopt;
    }

    std::optional<AbstractAttribute> Attributes::InstantiateSerializedAttribute(Json t_data) {
        if (t_data.contains("PackageName")) {
            auto attributeCandidate = InstantiateAttribute(t_data["PackageName"]);
            if (attributeCandidate.has_value()) {
                attributeCandidate.value()->id = t_data["ID"];
                attributeCandidate.value()->keyframes.clear();
                for (auto& keyframe : t_data["Keyframes"]) {
                    auto targetKeyframe = AttributeKeyframe(
                        keyframe["ID"], keyframe["Timestamp"], attributeCandidate.value()->LoadKeyframeValue(keyframe["Value"])
                    );
                    if (keyframe["Easing"] != nullptr) {
                        targetKeyframe.easing = Easings::InstantiateSerializedEasing(keyframe["Easing"]);
                    }
                    attributeCandidate.value()->keyframes.push_back(
                        targetKeyframe
                    );
                }
                attributeCandidate.value()->Load(t_data["Data"]);
                attributeCandidate.value()->packageName = t_data["PackageName"];
                attributeCandidate.value()->name = t_data["Name"];
                return attributeCandidate;
            }
        }
        return std::nullopt;
    }

    std::optional<AbstractAttribute> Attributes::CopyAttribute(AbstractAttribute t_base) {
        auto copiedAttributeCandidate = Attributes::InstantiateSerializedAttribute(t_base->Serialize());
        if (copiedAttributeCandidate.has_value()) {
            auto& attribute = copiedAttributeCandidate.value();
            attribute->id = Randomizer::GetRandomInteger();
            attribute->name += " (Copy)";
            for (auto& keyframe : attribute->keyframes) {
                keyframe.id = Randomizer::GetRandomInteger();
                if (keyframe.easing.has_value()) {
                    auto& easing = keyframe.easing.value();
                    easing->id = Randomizer::GetRandomInteger();
                }
            }
            return attribute;
        }
        return std::nullopt;
    }

    void Attributes::Initialize() {
        if (!std::filesystem::exists("attributes/")) {
            std::filesystem::create_directory("attributes");
        }
        auto iterator = std::filesystem::directory_iterator("attributes");
        for (auto &entry : iterator) {
            std::string transformedPath = std::regex_replace(
                GetBaseName(entry.path().string()), std::regex(".dll|.so|lib"), "");
            Libraries::LoadLibrary("attributes", transformedPath);
            AttributeImplementation implementation;
            implementation.description = Libraries::GetFunction<AttributeDescription()>(transformedPath, "GetDescription")();
            implementation.spawn = Libraries::GetFunction<AbstractAttribute()>(transformedPath, "SpawnAttribute");
            try {
                Libraries::GetFunction<void()>(transformedPath, "OnStartup")();
            } catch (internalDylib::exception ex) {
                // skip
            }
            s_implementations.push_back(implementation);
            std::cout << "loading attribute '" << implementation.description.packageName << "'" << std::endl;
        }
    }
};