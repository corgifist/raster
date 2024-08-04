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
        DUMP_VAR(targetTypeDesc.elementsize());
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
};