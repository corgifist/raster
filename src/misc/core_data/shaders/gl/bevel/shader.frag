#version 310 es

#ifdef GL_ES
precision highp float;
#endif
layout(location = 0) out vec4 finalColor;

uniform sampler2D uTexture;
uniform float uAngle;
uniform float uThickness;
uniform vec4 uLightColor;
uniform vec4 uShadowColor;

uniform vec2 uInputSize;

void main(void) {
    float thickness = uThickness;
    vec2 uv = gl_FragCoord.xy / uInputSize.xy;
    vec2 uTransform = vec2(thickness * cos(radians(uAngle)), thickness * sin(radians(uAngle)));
    vec2 transform = vec2(1.0 / uInputSize) * vec2(uTransform.x, uTransform.y);
    vec4 color = texture(uTexture, uv);
    float light = texture(uTexture, uv - transform).a;
    float shadow = texture(uTexture, uv + transform).a;

    color.rgb = mix(color.rgb, uLightColor.rgb, clamp((color.a - light) * uLightColor.a, 0.0, 1.0));
    color.rgb = mix(color.rgb, uShadowColor.rgb, clamp((color.a - shadow) * uShadowColor.a, 0.0, 1.0));
    finalColor = vec4(color.rgb * color.a, color.a);
}