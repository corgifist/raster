#include "common/generic_video_decoder.h"

#include "raster.h"
#include "video_decoder.h"
#include "video_decoders.h"
#include "cache_allocator.h"
#include "cache_allocator.h"

namespace Raster {
    struct VideoCacheEntry {
        uint8_t* data;
        std::string assetPath;
        size_t frame;
        size_t usageCount;
    };

    FreeListAllocator s_cacheAllocator;
    std::vector<VideoCacheEntry> s_videoCacheEntries;

    static void AllocateCache(size_t t_bytes) {
        s_cacheAllocator = FreeListAllocator(t_bytes, FreeListAllocator::FIND_FIRST);
        s_cacheAllocator.Init();
        if (!s_cacheAllocator.m_start_ptr) {
            RASTER_LOG("failed to allocate " << t_bytes << " bytes for video cache (" << t_bytes / (1024 * 1024) << " MB)");
            RASTER_LOG("trying to allocate fewer bytes");
            AllocateCache(t_bytes / 2);
        } else {
            RASTER_LOG("allocated " << t_bytes << " bytes for video cache (" << t_bytes / (1024 * 1024) << " MB)");
        }
    }

    void GenericVideoDecoder::InitializeCache(size_t t_size) {
        if (!s_cacheAllocator.m_start_ptr) {
            AllocateCache(t_size * 1024 * 1024);
        }
    }

    static AVPixelFormat correct_for_deprecated_pixel_format(AVPixelFormat pix_fmt) {
        switch (pix_fmt) {
            case AV_PIX_FMT_YUVJ420P: return AV_PIX_FMT_YUV420P;
            case AV_PIX_FMT_YUVJ422P: return AV_PIX_FMT_YUV422P;
            case AV_PIX_FMT_YUVJ444P: return AV_PIX_FMT_YUV444P;
            case AV_PIX_FMT_YUVJ440P: return AV_PIX_FMT_YUV440P;
            default:                  return pix_fmt;
        }
    }

    // returns pointer to newly cached frame
    static uint8_t* CacheVideoFrame(uint8_t* data, size_t t_frame, std::string t_assetPath, ImageAllocation& t_imageAllocation) {
        if (!s_cacheAllocator.m_start_ptr) return nullptr;
        auto videoPtr = s_cacheAllocator.Allocate(t_imageAllocation.allocationSize);
        if (videoPtr) {
            memcpy(videoPtr, data, t_imageAllocation.allocationSize);
            VideoCacheEntry newEntry;
            newEntry.data = (uint8_t*) videoPtr;
            newEntry.assetPath = t_assetPath;
            newEntry.frame = t_frame;
            s_videoCacheEntries.push_back(newEntry);
            // DUMP_VAR(newEntry.frame);
            return newEntry.data;
        } else {
            // deallocate the oldest cache entry
            if (!s_videoCacheEntries.empty()) {
                int cacheEntryCandidateIndex = 0;
                int i = 0; 
                for (auto& entry : s_videoCacheEntries) {
                    if (entry.assetPath != t_assetPath) {
                        i++;
                        continue;
                    }
                    auto& candidateEntry = s_videoCacheEntries[cacheEntryCandidateIndex];
                    if (candidateEntry.usageCount < entry.usageCount) {
                        cacheEntryCandidateIndex = i;
                    }
                    i++;
                }
                if (cacheEntryCandidateIndex > 0) {
                    auto& cacheEntryTarget = s_videoCacheEntries[cacheEntryCandidateIndex];
                    s_cacheAllocator.Free(cacheEntryTarget.data);
                    // DUMP_VAR(cacheEntryTarget.frame);
                    s_videoCacheEntries.erase(s_videoCacheEntries.begin() + cacheEntryCandidateIndex);
                } else {
                    // DUMP_VAR(s_videoCacheEntries[0].frame);
                    s_cacheAllocator.Free(s_videoCacheEntries[0].data);
                    s_videoCacheEntries.erase(s_videoCacheEntries.begin());
                }
                return CacheVideoFrame(data, t_frame, t_assetPath, t_imageAllocation);
            } else {
                RASTER_LOG("caching video frame failed despite having zero cache entries");
            }
        }
        return nullptr;
    }

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
        return AV_PIX_FMT_RGB24;
    }
    
    static size_t InterpretVideoFrameChannels(VideoFramePrecision t_precision) {
        if (t_precision == VideoFramePrecision::Full) {
            return 4;
        }
        if (t_precision == VideoFramePrecision::Half) {
            return 4;
        }
        if (t_precision == VideoFramePrecision::Usual) {
            return 3;
        }
        return 3;
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
                decoder->formatCtx.seek({0, {1, 1}});
                while (true) {
                    auto pkt = decoder->formatCtx.readPacket();
                    if (!pkt) break;
                    if (pkt.streamIndex() != decoder->targetVideoStream.index()) continue;
                    if (pkt.isKeyPacket()) {
                        decoder->keyframes.push_back(pkt.pts().seconds() * decoder->targetVideoStream.frameRate().getDouble());
                    }
                }
                if (decoder->videoDecoderCtx.isOpened()) {
                    decoder->videoDecoderCtx.close();
                }
                decoder->formatCtx.seek({0, {1, 1}});
                decoder->videoDecoderCtx = av::VideoDecoderContext(decoder->targetVideoStream);
                decoder->framerate = decoder->targetVideoStream.frameRate().getDouble();
                decoder->videoDecoderCtx.setRefCountedFrames(true);
                av::Dictionary options;
                options.set("threads", "auto");
                decoder->videoDecoderCtx.open(options);
                decoder->videoRescaler = av::VideoRescaler(decoder->videoDecoderCtx.width(), decoder->videoDecoderCtx.height(), correct_for_deprecated_pixel_format(InterpretVideoFramePrecision(targetPrecision)),
                                                            decoder->videoDecoderCtx.width(), decoder->videoDecoderCtx.height(), correct_for_deprecated_pixel_format(decoder->videoDecoderCtx.pixelFormat()));
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
                decoder->videoRescaler.dstPixelFormat() != correct_for_deprecated_pixel_format(InterpretVideoFramePrecision(targetPrecision))) {
                decoder->videoRescaler = av::VideoRescaler(decoder->videoDecoderCtx.width(), decoder->videoDecoderCtx.height(), correct_for_deprecated_pixel_format(InterpretVideoFramePrecision(targetPrecision)),
                                                            decoder->videoDecoderCtx.width(), decoder->videoDecoderCtx.height(), correct_for_deprecated_pixel_format(decoder->videoDecoderCtx.pixelFormat()));
            }

            int elementSize = 1;
            if (targetPrecision == VideoFramePrecision::Half) elementSize = 2;
            if (targetPrecision == VideoFramePrecision::Full) elementSize = 4;
            t_imageAllocation.Allocate(decoder->videoDecoderCtx.width(), decoder->videoDecoderCtx.height(), InterpretVideoFrameChannels(targetPrecision), elementSize);
            for (auto& entry : s_videoCacheEntries) {
                if (entry.frame == targetFrame && entry.assetPath == decoder->assetPath) {
                    t_imageAllocation.data = entry.data;
                    for (auto& otherEntry : s_videoCacheEntries) {
                        if (otherEntry.frame == targetFrame && otherEntry.assetPath == decoder->assetPath) continue;
                        otherEntry.usageCount++;
                    }
                    entry.usageCount = 0;
                    return true;
                }    
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
                    // RASTER_LOG("skipping frames: " << videoFrame.pts().seconds() << " to " << decoder->targetFrame / decoder->framerate);
                } 
                decoder->percentage = 1;
                decoder->currentlyDecoding = false;
                // DUMP_VAR(videoFrame.width());

                // DUMP_VAR(videoFrame.pts().seconds());
                if (!videoFrame) return false;
                
                videoFrame = decoder->videoRescaler.rescale(videoFrame);   
                if (!videoFrame) return false;

                t_imageAllocation.data = CacheVideoFrame(videoFrame.data(), targetFrame, decoder->assetPath, t_imageAllocation);
                videoFrame = av::VideoFrame();
                return true;
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