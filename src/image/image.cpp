#define OIIO_STATIC_BUILD

#include <OpenImageIO/imageio.h>
#include "image/image.h"


namespace Raster {
    Image::Image() {
        this->width = this->height = 0;
        this->channels = 0;
        this->precision = ImagePrecision::Usual;
    }

    std::optional<Image> ImageLoader::Load(std::string t_path) {
        auto input = OIIO::ImageInput::open(t_path);
        if (!input) {
            std::cout << OIIO::geterror() << std::endl;
            return std::nullopt;
        }
        const OIIO::ImageSpec& spec = input->spec();

        OIIO::TypeDesc targetTypeDesc = OIIO::TypeDesc::UINT8;
        if (spec.format.elementsize() == 2) targetTypeDesc = OIIO::TypeDesc::HALF;
        if (spec.format.elementsize() == 4) targetTypeDesc = OIIO::TypeDesc::FLOAT;
        std::vector<uint8_t> data(spec.width * spec.height * spec.nchannels * targetTypeDesc.elementsize());
        input->read_image(0, 0, 0, spec.nchannels, targetTypeDesc, data.data());

        Image result;
        result.precision = ImagePrecision::Usual;
        if (targetTypeDesc == OIIO::TypeDesc::UINT8) {
            result.precision = ImagePrecision::Usual;
        } else if (targetTypeDesc == OIIO::TypeDesc::HALF) {
            result.precision = ImagePrecision::Half;
        } else if (targetTypeDesc == OIIO::TypeDesc::FLOAT) {
            result.precision = ImagePrecision::Full;
        }
        result.channels = spec.nchannels;
        result.width = spec.width;
        result.height = spec.height;
        result.data = data;

        input->close();
        return result;
    }

    AsyncImageLoader::AsyncImageLoader() {
        this->m_initialized = false;
    }

    AsyncImageLoader::AsyncImageLoader(std::string t_path) {
        this->m_future = std::async(std::launch::async, [t_path] {
            auto candidate = ImageLoader::Load(t_path);
            if (!candidate.has_value()) return std::optional<std::shared_ptr<Image>>(std::nullopt);
            return std::optional(std::make_shared<Image>(candidate.value()));
        });
        this->m_initialized = true;
    }

    std::optional<std::shared_ptr<Image>> AsyncImageLoader::Get() {
        return m_future.get();
    }

    bool AsyncImageLoader::IsReady() {
        try {
            return IsFutureReady(m_future);
        } catch (...) {
            return false;
        }
    }

    bool AsyncImageLoader::IsInitialized() {
        return m_initialized;
    }
};