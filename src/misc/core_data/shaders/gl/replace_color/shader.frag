#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;

uniform vec4 uSourceColor, uDestinationColor;
uniform float uThreshold;
uniform float uSoftness;
uniform sampler2D uColor;

void main() {

  vec4 col = texture(uColor, gl_FragCoord.xy / uResolution.xy);

  
  // Get difference to use for falloff if required
  float diff = distance(col.rgba, uSourceColor) - uThreshold;
  
  // Apply linear falloff if needed, otherwise clamp
  float factor = clamp(diff / uSoftness, 0.0, 1.0);
  
  gColor = mix(uDestinationColor, col.rgba, factor);
}