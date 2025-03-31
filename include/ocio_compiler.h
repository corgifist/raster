#pragma once

#include <OpenColorIO/OpenColorTransforms.h>
#include <OpenColorIO/OpenColorTypes.h>
#include "gpu/gpu.h"
#include "common/randomizer.h"

namespace Raster {
    static Pipeline CompileOCIOShader(std::string t_shaderText) {
        static std::optional<std::string> s_ocioPlaceholder;
        if (!s_ocioPlaceholder) {
            s_ocioPlaceholder = ReadFile(GPU::GetShadersPath() + "/ocio_processor/shader.frag");
        }
        auto& placeholder = *s_ocioPlaceholder;
        auto finalShaderCode = ReplaceString(placeholder, "OCIO_SHADER_PLACEHOLDER", t_shaderText);
        int id = Randomizer::GetRandomInteger();
        auto cleanShaderPath = "ocio/shader" + std::to_string(id);
        auto writePath = GPU::GetShadersPath() + cleanShaderPath + ".frag";
        WriteFile(writePath, finalShaderCode);
        auto generatedShader = GPU::GenerateShader(ShaderType::Fragment, cleanShaderPath, false);
        std::filesystem::remove(writePath);
        return GPU::GeneratePipeline(GPU::s_basicShader, generatedShader);
    }  
};