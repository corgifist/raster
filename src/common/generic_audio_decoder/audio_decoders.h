#pragma once

#include "audio_decoder.h"

namespace Raster {


    struct AudioDecoders {
        static int CreateDecoder();
        static SharedAudioDecoder GetDecoder(int t_handle);
        static void DestroyDecoder(int t_handle);
        static bool DecoderExists(int t_handle);

        static std::unordered_map<int, SharedAudioDecoder> s_decoders;
    };
};