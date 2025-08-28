#version 310 es

#ifdef GL_ES
precision highp float;
#endif

#define a base
#define b blend

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;

uniform sampler2D uBase;
uniform sampler2D uBlend;

uniform float uOpacity;
uniform int uBlendMode;

__GENERATED_FUNCTIONS_GO_HERE__

vec3 blendMaster(vec3 base, vec3 blend, float opacity) {
    __GENERATED_CODE_GOES_HERE__
    return mix(base, blend, opacity);
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 baseTexel = texture(uBase, uv);
    vec4 blendTexel = texture(uBlend, uv);
    gColor = vec4(0);
    if (baseTexel.w != 0.0 && blendTexel.w != 0.0) {
        gColor = vec4(blendMaster(baseTexel.xyz, blendTexel.xyz, uOpacity), 1.0);
    } else if (baseTexel.w == 0.0 && blendTexel.w != 0.0) {
        gColor = blendTexel;
    } else if (baseTexel.w != 0.0 && blendTexel.w == 0.0) {
        gColor = baseTexel;
    }
}