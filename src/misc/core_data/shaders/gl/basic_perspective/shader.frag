#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

in vec4 vPos;
in vec2 vUV;

uniform bool uTextureAvailable;
uniform sampler2D uTexture;

uniform float uAspectRatio;


uniform bool uDepthAvailable;
uniform sampler2D uDepth;

uniform vec2 uResolution;

void main() {
    gColor = vec4(1.0);
    if (uDepthAvailable) {
        vec4 depthTexel = texture(uDepth, gl_FragCoord.xy / uResolution.xy);
        if (depthTexel.z < vPos.z && depthTexel.a != 0.0) discard;
    }
    if (uTextureAvailable) gColor *= texture(uTexture, vUV);
    
    gUV = vec4(vUV, uAspectRatio, 1);
}