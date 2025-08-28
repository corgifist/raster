
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