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



float blendColorBurn(float base, float blend) {
	return (blend==0.0)?blend:max((1.0-((1.0-base)/blend)),0.0);
}


float blendColorDodge(float base, float blend) {
	return (blend==1.0)?blend:min(base/(1.0-blend),1.0);
}

vec3 blendColorDodge(vec3 base, vec3 blend) {
	return vec3(blendColorDodge(base.x,blend.x),blendColorDodge(base.y,blend.y),blendColorDodge(base.z,blend.z));
}

vec3 blendColorDodge(vec3 base, vec3 blend, float opacity) {
	return (blendColorDodge(base, blend) * opacity + base * (1.0 - opacity));
}

float blendDarken(float base, float blend) {
	return min(blend,base);
}

vec3 blendDarken(vec3 base, vec3 blend) {
	return vec3(blendDarken(base.x,blend.x),blendDarken(base.y,blend.y),blendDarken(base.z,blend.z));
}

vec3 blendDarken(vec3 base, vec3 blend, float opacity) {
	return (blendDarken(base, blend) * opacity + base * (1.0 - opacity));
}


vec3 blendMaster(vec3 base, vec3 blend, float opacity) {
    
	if (uBlendMode == 0) return (min(base+blend,vec3(1.0)) * opacity + base * (1.0 - opacity));
	if (uBlendMode == 1) return ((base+blend)/2.0 * opacity + base * (1.0 - opacity));
	if (uBlendMode == 2) return ((vec3(blendColorBurn(base.x,blend.x),blendColorBurn(base.y,blend.y),blendColorBurn(base.z,blend.z)) * opacity + base * (1.0 - opacity)));
	if (uBlendMode == 3) return (blendColorDodge(base, blend, opacity));
	if (uBlendMode == 4) return (blendDarken(base, blend, opacity));
	if (uBlendMode == 5) return (abs(base-blend) * opacity + base * (1.0 - opacity));
	if (uBlendMode == 6) return ((base+blend-2.0*base*blend) * opacity + base * (1.0 - opacity));
	if (uBlendMode == 7) return (base * blend * opacity + base * (1.0 - opacity));

    return mix(base, blend, opacity);
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    gColor = vec4(blendMaster(texture(uBase, uv).rgb, texture(uBlend, uv).rgb, uOpacity), 1.0);
}