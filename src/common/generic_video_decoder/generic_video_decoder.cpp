#include "common/generic_video_decoder.h"

#include "video_decoder.h"
#include "video_decoders.h"
#include <cstdint>
#include <glm/fwd.hpp>
#include <libavcodec/avcodec.h>
#include <libavutil/pixfmt.h>
#include <optional>

namespace Raster {
    GenericVideoDecoder::GenericVideoDecoder() {
        this->assetID = 0;
        this->decoderContexts = std::make_shared<std::unordered_map<float, int>>();
        this->seekTarget = std::make_shared<float>();
        *this->seekTarget = 0;
        this->targetPrecision = VideoFramePrecision::Usual;
    }

    static SharedVideoDecoder AllocateDecoderContext(int t_decoderID) {
        if (VideoDecoders::s_decoders.find(t_decoderID) == VideoDecoders::s_decoders.end()) {
            auto result = std::make_shared<VideoDecoder>();
            VideoDecoders::s_decoders[t_decoderID] = result;
            return result;
        }
        return VideoDecoders::GetDecoder(t_decoderID);
    }

    static SharedVideoDecoder GetDecoderContext(GenericVideoDecoder* t_decoder) {
        std::vector<float> deadDecoders;
        for (auto& context : *t_decoder->decoderContexts) {
            auto decoder = AllocateDecoderContext(context.second);
            decoder->health--;
            if (decoder->health < 0) {
                deadDecoders.push_back(context.first);
            }
        }

        for (auto& deadDecoder : deadDecoders) {
            auto correspondingDecoderID = (*t_decoder->decoderContexts)[deadDecoder];
            auto decoderIterator = VideoDecoders::s_decoders.find(correspondingDecoderID);
            if (decoderIterator != VideoDecoders::s_decoders.end()) {
                VideoDecoders::s_decoders.erase(correspondingDecoderID);
                t_decoder->decoderContexts->erase(deadDecoder);
            }
        }

        auto& project = Workspace::GetProject();
        auto currentOffset = project.GetTimeTravelOffset();
        if (t_decoder->decoderContexts->find(currentOffset) != t_decoder->decoderContexts->end()) {
            auto allocatedDecoder = AllocateDecoderContext((*t_decoder->decoderContexts)[currentOffset]);
            allocatedDecoder->health = MAX_GENERIC_VIDEO_DECODER_LIFESPAN;
            allocatedDecoder->timeOffset = currentOffset;
            allocatedDecoder->id = (*t_decoder->decoderContexts)[currentOffset];
            return allocatedDecoder;
        }
        
        int newDecoderID = Randomizer::GetRandomInteger();
        auto newDecoder = AllocateDecoderContext(newDecoderID);
        (*t_decoder->decoderContexts)[currentOffset] = newDecoderID;
        newDecoder->health = MAX_GENERIC_VIDEO_DECODER_LIFESPAN;
        newDecoder->timeOffset = currentOffset;
        newDecoder->id = newDecoderID;
        return newDecoder;
    }

    static AVPixelFormat InterpretVideoFramePrecision(VideoFramePrecision t_precision) {
        if (t_precision == VideoFramePrecision::Full) {
            return AV_PIX_FMT_RGBAF32;
        }
        if (t_precision == VideoFramePrecision::Half) {
            return AV_PIX_FMT_RGBAF16;
        }
        return AV_PIX_FMT_RGBA;
    }

    void GenericVideoDecoder::SetVideoAsset(int t_assetID) {
        if (t_assetID == assetID) {
            return;
        }
        assetID = t_assetID;
        if (assetID == 0) {
            Destroy();
            return;
        }
        SharedLockGuard guard(m_decodingMutex);
        auto decoder = GetDecoderContext(this);
        auto& project = Workspace::GetProject();
        auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
        std::optional<std::string> assetPathCandidate;
        if (assetCandidate.has_value()) {
            assetPathCandidate = assetCandidate.value()->Serialize()["Data"]["RelativePath"];
        }

        if (assetPathCandidate.has_value() && assetPathCandidate.value() != decoder->assetPath) {
            auto& assetPath = assetPathCandidate.value();
            print("opening format context");    

            decoder->formatCtx.close();
            decoder->formatCtx.openInput(FormatString("%s/%s", project.path.c_str(), assetPath.c_str()));
            decoder->formatCtx.findStreamInfo();

            bool streamWasFound = false;

            for (int i = 0; i < decoder->formatCtx.streamsCount(); i++) {
                auto stream = decoder->formatCtx.stream(i);
                if (stream.isVideo()) {
                    decoder->targetVideoStream = stream;
                    streamWasFound = true;
                    break;
                }
            }


            if (streamWasFound) {

                // collecting all keyframes
                while (true) {
                    auto pkt = decoder->formatCtx.readPacket();
                    if (!pkt) break;
                    if (pkt.streamIndex() != decoder->targetVideoStream.index()) continue;
                    if (pkt.isKeyPacket()) {
                        decoder->keyframes.push_back(pkt.pts().seconds() * decoder->targetVideoStream.frameRate().getDouble());
                    }
                }
                decoder->formatCtx.seek({0, {1, 1}});

                if (decoder->videoDecoderCtx.isOpened()) {
                    decoder->videoDecoderCtx.close();
                }
                decoder->videoDecoderCtx = av::VideoDecoderContext(decoder->targetVideoStream);
                decoder->framerate = decoder->targetVideoStream.frameRate().getDouble();
                decoder->videoDecoderCtx.setRefCountedFrames(true);
                av::Dictionary options;
                options.set("threads", "auto");
                decoder->videoDecoderCtx.open(options);
                decoder->videoRescaler = av::VideoRescaler(decoder->videoDecoderCtx.width(), decoder->videoDecoderCtx.height(), InterpretVideoFramePrecision(targetPrecision),
                                                            decoder->videoDecoderCtx.width(), decoder->videoDecoderCtx.height(), decoder->videoDecoderCtx.pixelFormat());
                decoder->needsSeeking = true;
            }

            decoder->wasOpened = true;
            decoder->assetPath = assetPath;
        }
    }

    bool GenericVideoDecoder::DecodeFrame(ImageAllocation& t_imageAllocation, int t_renderPassID, std::optional<float> t_targetFrame) {
        if (assetID == 0) {
            Destroy();
            return false;
        }
        
        SharedLockGuard guard(m_decodingMutex);
        auto& project = Workspace::GetProject();

        auto decoder = GetDecoderContext(this);

        size_t targetFrame = t_targetFrame.value_or(0) * decoder->framerate;
        if (t_targetFrame) {
            decoder->targetFrame = targetFrame;
        }

        if (decoder->formatCtx.isOpened() && decoder->videoDecoderCtx.isOpened()) {
            if (decoder->videoRescaler.dstWidth() != decoder->videoDecoderCtx.width() || 
                decoder->videoRescaler.dstHeight() != decoder->videoDecoderCtx.height() ||
                decoder->videoRescaler.dstPixelFormat() != InterpretVideoFramePrecision(targetPrecision)) {
                decoder->videoRescaler = av::VideoRescaler(decoder->videoDecoderCtx.width(), decoder->videoDecoderCtx.height(), InterpretVideoFramePrecision(targetPrecision),
                                                            decoder->videoDecoderCtx.width(), decoder->videoDecoderCtx.height(), decoder->videoDecoderCtx.pixelFormat());
            }
            int elementSize = 1;
            if (targetPrecision == VideoFramePrecision::Half) elementSize = 2;
            if (targetPrecision == VideoFramePrecision::Full) elementSize = 4;
            if (!t_imageAllocation.Get() || 
                (t_imageAllocation.width != decoder->videoDecoderCtx.width() || t_imageAllocation.height != decoder->videoDecoderCtx.height() || t_imageAllocation.elementSize != elementSize)) {
                t_imageAllocation.Allocate(decoder->videoDecoderCtx.width(), decoder->videoDecoderCtx.height(), elementSize);
            }
            int64_t frameDifference = targetFrame - decoder->lastLoadedFrame;
            int64_t reservedLastLoadedFrame = decoder->lastLoadedFrame;
            decoder->lastLoadedFrame = targetFrame;

            av::VideoFrame videoFrame;
            if (reservedLastLoadedFrame != targetFrame) {
                auto currentKeyframe = decoder->GetCurrentKeyFrameForFrame(targetFrame);
                auto previousKeyframe = decoder->GetCurrentKeyFrameForFrame(reservedLastLoadedFrame);
                if (frameDifference > 1 || frameDifference < 0) {
                    if (frameDifference < 0) {
                        decoder->SeekDecoder(targetFrame / decoder->framerate);
                        // RASTER_LOG("backward hardcode seeking");
                    }
                    if (currentKeyframe != previousKeyframe && frameDifference > 0) {
                        // RASTER_LOG("keyframe mismatch seeking");
                        decoder->SeekDecoder(targetFrame / decoder->framerate);
                    }
                }

                float firstTimestmap = -1;
                while ((videoFrame = decoder->DecodeOneFrameWithoutRescaling()).pts().seconds() < targetFrame / decoder->framerate) {
                    if (firstTimestmap < 0) {
                        firstTimestmap = videoFrame.pts().seconds();
                    }
                    decoder->currentlyDecoding = true;
                    decoder->percentage = (videoFrame.pts().seconds() - firstTimestmap) / ((targetFrame - firstTimestmap * decoder->framerate) / decoder->framerate);
                    DUMP_VAR(decoder->percentage);
                    // RASTER_LOG("skipping frames: " << videoFrame.pts().seconds() << " to " << decoder->targetFrame / decoder->framerate);
                } 
                decoder->percentage = 1;
                decoder->currentlyDecoding = false;

                // DUMP_VAR(videoFrame.pts().seconds());

                if (videoFrame) {
                    videoFrame = decoder->videoRescaler.rescale(videoFrame);
                    memcpy(t_imageAllocation.Get(), videoFrame.data(), t_imageAllocation.allocationSize);
                    return true;
                } 
            }
        }

        return false;
    }

    std::optional<float> GenericVideoDecoder::GetDecodingProgress() {
        for (auto& decodingPair : *decoderContexts) {
            auto decoder = AllocateDecoderContext(decodingPair.second);
            if (decoder->currentlyDecoding) {
                return decoder->percentage;
            }
        }
        return std::nullopt;
    }

    void GenericVideoDecoder::Seek(float t_second) {
        // SharedLockGuard decodingGuard(m_decodingMutex);
        *seekTarget = t_second;
        for (auto& decodingPair : *decoderContexts) {
            auto decoder = AllocateDecoderContext(decodingPair.second);
            decoder->needsSeeking = true;
            // decoder->targetFrame = t_second * decoder->framerate;
        }
    }

    void GenericVideoDecoder::Destroy() {
        for (auto& context : *decoderContexts) {
            if (VideoDecoders::DecoderExists(context.second)) {
                VideoDecoders::DestroyDecoder(context.second);
            }
        }
        decoderContexts->clear();
    }

    std::optional<float> GenericVideoDecoder::GetContentDuration() {
        if (decoderContexts->find(0) != decoderContexts->end()) {
            auto decoder = AllocateDecoderContext(decoderContexts->at(0));
            if (decoder->formatCtx.isOpened()) {
                return decoder->formatCtx.duration().seconds() * Workspace::GetProject().framerate;
            }
        }
        return std::nullopt;
    }

    std::optional<glm::vec2> GenericVideoDecoder::GetContentResolution() {
        for (auto& context : *decoderContexts) {
            auto decoder = AllocateDecoderContext(context.second);
            return decoder->GetResolution();
        }
        return std::nullopt;
    }

    std::optional<float> GenericVideoDecoder::GetContentFramerate() {
        for (auto& context : *decoderContexts) {
            auto decoder = AllocateDecoderContext(context.second);
            return decoder->GetFramerate();
        }
        return std::nullopt;
    }

};