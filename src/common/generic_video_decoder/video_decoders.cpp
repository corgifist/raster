#include "video_decoders.h"
#include "common/randomizer.h"
#include "video_decoder.h"

namespace Raster {
    std::unordered_map<int, SharedVideoDecoder> VideoDecoders::s_decoders;

    int VideoDecoders::CreateDecoder() {
        int id = Randomizer::GetRandomInteger();
        s_decoders[id] = std::make_shared<VideoDecoder>();
        return id;
    }

    SharedVideoDecoder VideoDecoders::GetDecoder(int t_handle) {
        return s_decoders[t_handle];
    }

    void VideoDecoders::DestroyDecoder(int t_handle) {
        s_decoders.erase(t_handle);
    }

    bool VideoDecoders::DecoderExists(int t_handle) {
        return s_decoders.find(t_handle) != s_decoders.end();
    }
}