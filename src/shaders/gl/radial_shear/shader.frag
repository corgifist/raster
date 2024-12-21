#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform float uOpacity;
uniform sampler2D uTexture;

uniform vec2 uCenter;
uniform vec2 uStrength;

uniform float uRange;


vec2 radial_shear(vec2 uv, vec2 center, float range, vec2 strength)
{
    /*
    Source:
    https://docs.unity3d.com/Packages/com.unity.shadergraph@6.9/manual/Radial-Shear-Node.html
    */
    vec2 uv_cen = uv - center;
    vec2 scaled_dist_sq = smoothstep(0.,range,range-distance(uv,center)) * strength * dot(uv_cen, uv_cen);
    return uv + vec2(uv_cen.y, -uv_cen.x) * scaled_dist_sq;
}


void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    gColor = texture(uTexture, radial_shear(uv, uCenter, uRange, uStrength));
    gColor = mix(texture(uTexture, uv), gColor, uOpacity);
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, uResolution.x / uResolution.y, 1.0);
}