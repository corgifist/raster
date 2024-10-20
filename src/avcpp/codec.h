#pragma once

#include <list>
#include <deque>
#include <memory>

#include "ffmpeg.h"
#include "rational.h"
#include "format.h"
#include "pixelformat.h"
#include "sampleformat.h"
#include "channellayout.h"
#include "avutils.h"

namespace av {

class Codec : public FFWrapperPtr<const AVCodec>
{
public:
    using FFWrapperPtr<const AVCodec>::FFWrapperPtr;

    const char *name() const;
    const char *longName() const;
    bool        canEncode() const;
    bool        canDecode() const;
    bool        isEncoder() const;
    bool        isDecoder() const;
    AVMediaType type() const;

    std::deque<Rational>       supportedFramerates()    const;
    std::deque<PixelFormat>    supportedPixelFormats()  const;
    std::deque<int>            supportedSamplerates()   const;
    std::deque<SampleFormat>   supportedSampleFormats() const;
    std::deque<uint64_t>       supportedChannelLayouts() const;

#if API_NEW_CHANNEL_LAYOUT
    std::deque<ChannelLayoutView> supportedChannelLayouts2() const;
#endif

    AVCodecID id() const;
};


Codec findEncodingCodec(const OutputFormat &format, bool isVideo = true);
Codec findEncodingCodec(AVCodecID id);
Codec findEncodingCodec(const std::string& name);

Codec findDecodingCodec(AVCodecID id);
Codec findDecodingCodec(const std::string& name);

Codec guessEncodingCodec(OutputFormat format, const char *name, const char *url, const char* mime, AVMediaType mediaType);

} // ::fmpeg
