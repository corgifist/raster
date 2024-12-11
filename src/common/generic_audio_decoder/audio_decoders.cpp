#include "audio_decoders.h"
#include "common/randomizer.h"

namespace Raster {
    std::unordered_map<int, SharedAudioDecoder> AudioDecoders::s_decoders;

    int AudioDecoders::CreateDecoder() {
        int id = Randomizer::GetRandomInteger();
        s_decoders[id] = std::make_shared<AudioDecoder>();
        return id;
    }

    SharedAudioDecoder AudioDecoders::GetDecoder(int t_handle) {
        return s_decoders[t_handle];
    }

    void AudioDecoders::DestroyDecoder(int t_handle) {
        s_decoders.erase(t_handle);
    }

    bool AudioDecoders::DecoderExists(int t_handle) {
        return s_decoders.find(t_handle) != s_decoders.end();
    }
}