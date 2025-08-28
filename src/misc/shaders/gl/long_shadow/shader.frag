#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;

uniform vec4 uShadowColor;
uniform float uLength;
uniform float uAngle;
uniform float uQuality;
uniform sampler2D uColor;

void main() {
	vec2 uv = gl_FragCoord.xy / uResolution.xy;
	vec4 vid = texture(uColor, uv);

	// Correct aspect ratio for non-square images
	vec2 aspect = 1.0 / uResolution.xy;

	// In pixels
	float shadowLength = uResolution.x * uLength / 10.0;
	// Lower values mean better precision
	float shadowPrecision = 1.0 / uQuality;
	// In radians
	float shadowAngle = radians(-uAngle);

	float shadowMatte = 0.0;
	for (float i = shadowPrecision; i <= shadowLength; i += shadowPrecision) {
		vec2 offset = vec2(sin(shadowAngle), cos(shadowAngle));
		vec2 np = uv + offset * i * aspect;
		np.x = clamp(np.x, 0., 1.);
		np.y = clamp(np.y, 0., 1.);
		vec4 col = texture(uColor, np);
		shadowMatte = max(shadowMatte, col.a);
	}

	gColor = mix(shadowMatte * uShadowColor, vid, vid.a);
}