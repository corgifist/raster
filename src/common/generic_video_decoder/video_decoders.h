#pragma once

#include "video_decoder.h"

namespace Raster {
    struct VideoDecoders {
        static int CreateDecoder();
        static SharedVideoDecoder GetDecoder(int t_handle);
        static void DestroyDecoder(int t_handle);
        static bool DecoderExists(int t_handle);

        static std::unordered_map<int, SharedVideoDecoder> s_decoders;
    };
};