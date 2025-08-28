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