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


float blendReflectGlow(float base, float blend) {
	return (blend==1.0)?blend:min(base*base/(1.0-blend),1.0);
}

vec3 blendReflectGlow(vec3 base, vec3 blend) {
	return vec3(blendReflectGlow(base.x,blend.x),blendReflectGlow(base.y,blend.y),blendReflectGlow(base.z,blend.z));
}

vec3 blendReflectGlow(vec3 base, vec3 blend, float opacity) {
	return (blendReflectGlow(base, blend) * opacity + base * (1.0 - opacity));
}

vec3 blendGlow(vec3 base, vec3 blend) {
	return blendReflectGlow(blend,base);
}

vec3 blendGlow(vec3 base, vec3 blend, float opacity) {
	return (blendGlow(base, blend) * opacity + base * (1.0 - opacity));
}

float blendOverlayHardLight(float base, float blend) {
	return base<0.5?(2.0*base*blend):(1.0-2.0*(1.0-base)*(1.0-blend));
}

vec3 blendOverlayHardLight(vec3 base, vec3 blend) {
	return vec3(blendOverlayHardLight(base.x,blend.x),blendOverlayHardLight(base.y,blend.y),blendOverlayHardLight(base.z,blend.z));
}

vec3 blendOverlayHardLight(vec3 base, vec3 blend, float opacity) {
	return (blendOverlayHardLight(base, blend) * opacity + base * (1.0 - opacity));
}

vec3 blendHardLight(vec3 base, vec3 blend) {
	return blendOverlayHardLight(blend,base);
}

vec3 blendHardLight(vec3 base, vec3 blend, float opacity) {
	return (blendHardLight(base, blend) * opacity + base * (1.0 - opacity));
}

vec3 blendNegation(vec3 base, vec3 blend) {
	return vec3(1.0)-abs(vec3(1.0)-base-blend);
}

vec3 blendNegation(vec3 base, vec3 blend, float opacity) {
	return (blendNegation(base, blend) * opacity + base * (1.0 - opacity));
}


vec3 blendMaster(vec3 base, vec3 blend, float opacity) {
    
	if (uBlendMode == 0) return (min(base+blend,vec3(1.0)) * opacity + base * (1.0 - opacity));
	if (uBlendMode == 1) return ((base+blend)/2.0 * opacity + base * (1.0 - opacity));
	if (uBlendMode == 2) return ((vec3(blendColorBurn(base.x,blend.x),blendColorBurn(base.y,blend.y),blendColorBurn(base.z,blend.z)) * opacity + base * (1.0 - opacity)));
	if (uBlendMode == 3) return (blendColorDodge(base, blend, opacity));
	if (uBlendMode == 4) return (blendDarken(base, blend, opacity));
	if (uBlendMode == 5) return (abs(base-blend) * opacity + base * (1.0 - opacity));
	if (uBlendMode == 6) return ((base / blend) * opacity + base * (1.0 - opacity));
	if (uBlendMode == 7) return ((base+blend-2.0*base*blend) * opacity + base * (1.0 - opacity));
	if (uBlendMode == 8) return (blendGlow(base, blend, opacity));
	if (uBlendMode == 9) return (blendHardLight(base, blend, opacity));
	if (uBlendMode == 10) return (base * blend * opacity + base * (1.0 - opacity));
	if (uBlendMode == 11) return (blendNegation(base, blend, opacity));
	if (uBlendMode == 12) return ((base - blend) * opacity + base * (1.0 - opacity));

    return mix(base, blend, opacity);
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 baseTexel = texture(uBase, uv);
    vec4 blendTexel = texture(uBlend, uv);
    gColor = vec4(0);
    if (baseTexel.w != 0.0 && blendTexel.w != 0.0) {
        gColor = vec4(blendMaster(baseTexel.xyz, blendTexel.xyz, uOpacity), 1.0);
    } else {
        gColor = baseTexel;
    }
}