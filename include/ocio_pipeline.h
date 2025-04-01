#pragma once

#include "raster.h"
#include "ocio_compiler.h"
#include "gpu/gpu.h"
#include "common/color_management.h"

#include <OpenColorIO/OpenColorIO.h>
#include <OpenColorIO/OpenColorTypes.h>

namespace OCIO = OCIO_NAMESPACE;

namespace Raster {

    struct OCIOTexture {
        Texture texture;
        std::string samplerName;
    };

    struct OCIOPipeline {
        uint8_t* pixels;
        std::unique_ptr<OCIO::PackedImageDesc> img;
        OCIO::ConstProcessorRcPtr processor;
        OCIO::ConstCPUProcessorRcPtr cpuProcessor;
        OCIO::GpuShaderDescRcPtr shaderDesc;
        Pipeline pipeline;
        bool valid;
        bool gpuPipeline;
        std::vector<OCIOTexture> textures;
        std::unordered_map<std::string, OCIO::GpuShaderDesc::UniformData> uniforms;

        OCIOPipeline() : valid(false) {}
        OCIOPipeline(OCIO::ConstTransformRcPtr t_transform, bool t_gpuPipeline = true) {
            try {
                processor = ColorManagement::s_config->getProcessor(t_transform);
                if (t_gpuPipeline) {
                    gpuPipeline = true;
                    auto gpuProcessor = ColorManagement::s_useLegacyGPU ?
                                        processor->getOptimizedLegacyGPUProcessor(OCIO::OptimizationFlags::OPTIMIZATION_LOSSLESS, 32) :
                                        processor->getOptimizedGPUProcessor(OCIO::OptimizationFlags::OPTIMIZATION_LOSSLESS);
                    shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
                    shaderDesc->setLanguage(OCIO::GpuLanguage::GPU_LANGUAGE_GLSL_ES_3_0);
                    shaderDesc->setFunctionName("OCIODisplay");
                    shaderDesc->setResourcePrefix("ocio_");
                    gpuProcessor->extractGpuShaderInfo(shaderDesc);
                    pipeline = CompileOCIOShader(shaderDesc->getShaderText());

                    // uploading 3d textures
                    for (int i = 0; i < shaderDesc->getNum3DTextures(); i++) {
                        const char* textureName = nullptr;
                        const char* samplerName = nullptr;
                        unsigned int edgelen = 0;
                        OCIO::Interpolation interpolation; // unused for now
                        shaderDesc->get3DTexture(i, textureName, samplerName, edgelen, interpolation);

                        auto texture = GPU::GenerateTexture(edgelen, edgelen, 3, TexturePrecision::Full, false, TextureDimensions::_3D, edgelen);

                        const float* data = nullptr;
                        shaderDesc->get3DTextureValues(i, data);
                        GPU::UpdateTexture(texture, 0, 0, edgelen, edgelen, 3, (uint8_t*) data, 0);
                        textures.push_back(OCIOTexture{
                            .texture = texture,
                            .samplerName = samplerName
                        });
                    }

                    // uploading 2d textures
                    for (int i = 0; i < shaderDesc->getNumTextures(); i++) {
                        const char* textureName = nullptr;
                        const char* samplerName = nullptr;
                        unsigned int width, height;
                        OCIO::Interpolation interpolation;
                        OCIO::GpuShaderDesc::TextureDimensions dimensions; // unsupported for now
                        OCIO::GpuShaderDesc::TextureType type;
                        shaderDesc->getTexture(i, textureName, samplerName, width, height, type, dimensions, interpolation);
                        int channelsCount = type == OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL ? 3 : 1;
                        if (dimensions == OCIO::GpuShaderDesc::TEXTURE_1D) {
                            RASTER_LOG("skipping unsupported 1-d texture");
                            continue;
                        }
                        auto texture = GPU::GenerateTexture(width, height, channelsCount, TexturePrecision::Full);
                        
                        const float* data = nullptr;
                        shaderDesc->getTextureValues(i, data);
                        GPU::UpdateTexture(texture, 0, 0, width, height, channelsCount, (uint8_t*) data);
                        textures.push_back(OCIOTexture{
                            .texture = texture,
                            .samplerName = samplerName
                        });
                    }

                    for (int i = 0; i < shaderDesc->getNumUniforms(); i++) {
                        OCIO::GpuShaderDesc::UniformData data;
                        const char* name = shaderDesc->getUniform(i, data);
                        if (data.m_type == OCIO::UNIFORM_UNKNOWN) continue;
                        uniforms[name] = data;
                    }
                } else {
                    gpuPipeline = false;
                    cpuProcessor = nullptr;
                    // cpuProcessor = processor->getOptimizedCPUProcessor(OCIO::OptimizationFlags::OPTIMIZATION_DEFAULT);
                }
                this->valid =  true;
            } catch (const OCIO::Exception& ex) {
                RASTER_LOG("failed to create OCIOPipeline due to OCIO exception");
                RASTER_LOG(ex.what());
                this->valid = false;
            } catch (...) {
                RASTER_LOG("failed to create OCIOPipeline due to unknown exception");
                this->valid = false;
            }
        }

        void Apply(Framebuffer dst, Framebuffer src) {
            if (gpuPipeline) {
                GPU::BindPipeline(pipeline);
                GPU::BindFramebuffer(dst);
                BindAllTextures(1);
                SetAllUniforms();
                GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(src.width, src.height));
                if (!src.attachments.empty()) GPU::BindTextureToShader(pipeline.fragment, "uTexture", src.attachments[0], 0);
                GPU::SetShaderUniform(pipeline.fragment, "ocio_grading_primary_localBypass", false);
                GPU::DrawArrays(3);
            } else {
                OCIO::BitDepth targetBitDepth;
                if (src.attachments[0].precision == TexturePrecision::Usual) {
                    targetBitDepth = OCIO::BitDepth::BIT_DEPTH_UINT8;
                } else if (src.attachments[0].precision == TexturePrecision::Half) {
                    targetBitDepth = OCIO::BitDepth::BIT_DEPTH_F16;
                } else if (src.attachments[0].precision == TexturePrecision::Full) {
                    targetBitDepth = OCIO::BitDepth::BIT_DEPTH_F32;
                }
                int elementSize = 1;
                if (src.attachments[0].precision == TexturePrecision::Half) {
                    elementSize = 2;
                } else if (src.attachments[0].precision == TexturePrecision::Full) {
                    elementSize = 4;
                }
                if (!img || (img && img->getWidth() != src.width || img->getHeight() != src.height || img->getBitDepth() != targetBitDepth)) {
                    delete[] pixels;
                    pixels = new uint8_t[src.width * src.height * 4 * elementSize];
                
                    auto oneChannelInBytes = elementSize;
                
                    auto m_chanStrideBytes = oneChannelInBytes;
                    auto m_xStrideBytes    = m_chanStrideBytes * 4;
                    auto m_yStrideBytes    = m_xStrideBytes * src.width;
                    img = std::make_unique<OCIO::PackedImageDesc>(pixels, src.width, src.height, 4, targetBitDepth, m_chanStrideBytes, m_xStrideBytes, m_yStrideBytes);
                }
                if (!cpuProcessor || cpuProcessor->getInputBitDepth() != targetBitDepth || cpuProcessor->getOutputBitDepth() != targetBitDepth) {
                    cpuProcessor = processor->getOptimizedCPUProcessor(targetBitDepth, targetBitDepth, OCIO::OptimizationFlags::OPTIMIZATION_LOSSLESS);
                }
                GPU::BindFramebuffer(src);
                GPU::ReadPixels(0, 0, src.width, src.height, 4, src.attachments[0].precision, pixels);
                cpuProcessor->apply(*img);

                GPU::UpdateTexture(dst.attachments[0], 0, 0, dst.attachments[0].width, dst.attachments[0].height, 4, pixels);
            }
        }

        void BindAllTextures(int t_baseIndex) {
            if (!gpuPipeline) return;
            for (int i = 0; i < textures.size(); i++) {
                GPU::BindTextureToShader(pipeline.fragment, textures[i].samplerName, textures[i].texture, i + t_baseIndex);
            }
        }

        void SetAllUniforms() {
            if (!gpuPipeline) return;
            for (auto& uniformPair : uniforms) {
                if (uniformPair.second.m_getBool) {
                    GPU::SetShaderUniform(pipeline.fragment, uniformPair.first, uniformPair.second.m_getBool());
                } else if (uniformPair.second.m_getDouble) {
                    GPU::SetShaderUniform(pipeline.fragment, uniformPair.first, (float) uniformPair.second.m_getDouble());
                } else if (uniformPair.second.m_getFloat3) {
                    auto f = uniformPair.second.m_getFloat3();
                    GPU::SetShaderUniform(pipeline.fragment, uniformPair.first, glm::vec3(f[0], f[1], f[2]));
                } else if (uniformPair.second.m_vectorFloat.m_getSize && uniformPair.second.m_vectorFloat.m_getVector) {
                    GPU::SetShaderUniform(pipeline.fragment, uniformPair.first, uniformPair.second.m_vectorFloat.m_getSize(), (float*) uniformPair.second.m_vectorFloat.m_getVector());
                } else if (uniformPair.second.m_vectorInt.m_getSize && uniformPair.second.m_vectorInt.m_getVector) {
                    GPU::SetShaderUniform(pipeline.fragment, uniformPair.first, uniformPair.second.m_vectorInt.m_getSize(), (int*) uniformPair.second.m_vectorInt.m_getVector());
                }
            }
        }

        void Destroy() {
            if (gpuPipeline) {
                GPU::DestroyPipeline(pipeline);
                for (auto& texture : textures) {
                    GPU::DestroyTexture(texture.texture);
                }
            } else {
                delete[] pixels;
            }
        }

        OCIO::DynamicPropertyRcPtr GetDynamicProperty(OCIO::DynamicPropertyType t_type) {
            return gpuPipeline ? shaderDesc->getDynamicProperty(t_type) : cpuProcessor->getDynamicProperty(t_type);
        }

        bool hasDynamicProperty(OCIO::DynamicPropertyType t_type) {
            return gpuPipeline ? shaderDesc->hasDynamicProperty(t_type) : cpuProcessor->hasDynamicProperty(t_type);
        }

        bool Ready() {
            return gpuPipeline ? true : cpuProcessor != nullptr;
        }
    };


};