#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "compositor/managed_framebuffer.h"
#include "compositor/texture_interoperability.h"
#include "common/transform2d.h"
#include "raster.h"

namespace Raster {

    struct SDFShapeUniform {
        std::string type, name;
        std::any value;

        SDFShapeUniform() {}
        SDFShapeUniform(std::string t_type, std::string t_name, std::any t_value) {
            this->type = t_type;
            this->name = t_name;
            this->value = t_value;
        }
    };

    struct SDFShape {
        int id;
        std::vector<SDFShapeUniform> uniforms;
        std::string distanceFunctionName;
        std::string distanceFunctionCode;

        SDFShape() {
            static std::optional<std::string> s_nullShapeCode;
            if (!s_nullShapeCode.has_value()) {
                s_nullShapeCode = ReadFile(GPU::GetShadersPath() + "layer2d/sdf_null_shape.frag");
            }
            this->id = Randomizer::GetRandomInteger();
            this->uniforms = {};
            this->distanceFunctionName = "fSDFNullShape";
            this->distanceFunctionCode = s_nullShapeCode.value_or("");
        }
    };

    struct SDFShapePipeline {
        SDFShape shape;
        Pipeline pipeline;
    };

    struct Layer2D : public NodeBase {
    public:
        Layer2D();
        ~Layer2D();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();


        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

    private:
        std::optional<SDFShape> GetShape();
        std::optional<Pipeline> GetPipeline();

        SDFShapePipeline GeneratePipelineFromShape(SDFShape t_shape);

        void SetShapeUniforms(SDFShape t_shape, Pipeline pipeline);

        Sampler m_sampler;
        ManagedFramebuffer m_managedFramebuffer;
        std::optional<SDFShapePipeline> m_pipeline;

        static std::optional<Pipeline> s_nullShapePipeline;
    };
};