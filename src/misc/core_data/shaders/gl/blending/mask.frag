vec3 blendMask(vec3 base, vec3 blend) {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 baseTexel = texture(uBase, uv);
    vec4 blendTexel = texture(uBlend, uv);
    return blendTexel.rgb;
}

vec3 blendMask(vec3 base, vec3 blend, float opacity) {
	return (blendMask(base, blend) * opacity + base * (1.0 - opacity));
}