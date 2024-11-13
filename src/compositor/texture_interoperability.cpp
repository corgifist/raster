#include "compositor/texture_interoperability.h"
#include "common/dispatchers.h"

namespace Raster {
    std::optional<Framebuffer> TextureInteroperability::GetFramebuffer(std::optional<std::any> t_value) {
        if (!t_value.has_value()) return std::nullopt;
        auto value = t_value.value();
        if (value.type() == typeid(Framebuffer)) {
            return std::any_cast<Framebuffer>(value);
        }
        auto conversionCandidate = Dispatchers::DispatchConversion(value, typeid(Texture));
        if (conversionCandidate.has_value()) {
            value = conversionCandidate.value();
        }
        if (value.type() == typeid(Texture)) {
            auto texture = std::any_cast<Texture>(value);
            Framebuffer result;
            result.width = texture.width;
            result.height = texture.height;
            result.attachments = {texture};
            return result;
        }
        return std::nullopt;
    }

    std::optional<Texture> TextureInteroperability::GetTexture(std::optional<std::any> t_value) {
        if (!t_value.has_value()) return std::nullopt;
        auto value = t_value.value();
        auto conversionCandidate = Dispatchers::DispatchConversion(value, typeid(Texture));
        if (conversionCandidate.has_value()) {
            value = conversionCandidate.value();
        }
        if (value.type() == typeid(Texture)) {
            return std::any_cast<Texture>(value);
        }

        if (value.type() == typeid(Framebuffer)) {
            auto framebuffer = std::any_cast<Framebuffer>(value);
            if (!framebuffer.attachments.empty()) {
                return framebuffer.attachments.at(0);
            }
        }

        return std::nullopt;
    }
}