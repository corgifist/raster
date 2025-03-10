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
            RASTER_LOG(OIIO::geterror());
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

    static std::optional<std::string> s_imageWriterError = std::nullopt;

    bool ImageWriter::Write(std::string t_path, Image& t_image) {
        auto output = OIIO::ImageOutput::create(t_path);
        if (!output) {
            s_imageWriterError = OIIO::geterror();
            return false;
        }

        OIIO::TypeDesc specFormat = OIIO::TypeDesc::UINT8;
        if (t_image.precision == ImagePrecision::Half) {
            specFormat = OIIO::TypeDesc::HALF;
        } else if (t_image.precision == ImagePrecision::Full) {
            specFormat = OIIO::TypeDesc::FLOAT;
        }
        OIIO::ImageSpec spec(t_image.width, t_image.height, t_image.channels, specFormat);
        if (!output->open(t_path, spec)) {
            s_imageWriterError = OIIO::geterror();
            return false;
        }

        if (!output->write_image(specFormat, t_image.data.data())) {
            s_imageWriterError = OIIO::geterror();
            return false;
        }

        if (!output->close()) {
            s_imageWriterError = OIIO::geterror();
            return false;
        }

        return true;
    }

    std::optional<std::string> ImageWriter::GetError() {
        auto error = s_imageWriterError;
        s_imageWriterError = std::nullopt;
        return error;
    }

    std::string ImageLoader::GetImplementationName() {
        return FormatString("OpenImageIO %i.%i.%i", OIIO_VERSION_MAJOR, OIIO_VERSION_MINOR, OIIO_VERSION_PATCH);
    }

    static void TryDeleteExtension(std::vector<std::string>& extensions, std::string extension) {
        auto extensionIterator = std::find(extensions.begin(), extensions.end(), extension);
        if (extensionIterator != extensions.end()) {
            extensions.erase(extensionIterator);
        }
    }

    std::vector<std::string> ImageLoader::GetSupportedExtensions() {
        std::vector<std::string> result;
        for (auto& pair : OIIO::get_extension_map()) {
            for (auto& extension : pair.second) {
                result.push_back(ReplaceString(extension, "\\.", ""));
            }
        }
        TryDeleteExtension(result, "avi");
        TryDeleteExtension(result, "qt");
        TryDeleteExtension(result, "mov");
        TryDeleteExtension(result, "mp4");
        TryDeleteExtension(result, "m4a");
        TryDeleteExtension(result, "m4v");
        TryDeleteExtension(result, "3gp");
        TryDeleteExtension(result, "3g2");
        TryDeleteExtension(result, "mj2");
        TryDeleteExtension(result, "m4v");
        TryDeleteExtension(result, "mpg");
        TryDeleteExtension(result, "null");
        TryDeleteExtension(result, "nul");
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