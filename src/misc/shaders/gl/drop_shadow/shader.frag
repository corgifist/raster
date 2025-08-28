#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec4 uColor;
uniform vec2 uResolution;

uniform sampler2D uTexture;

#define iChannel0 uTexture
#define iResolution uResolution
#define fragColor gColor
#define fragCoord gl_FragCoord.xy

uniform float uDirections; // = 15.0
uniform float uQuality; // = 5.0
uniform float uSize; // = 8.0

float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

#define directions uDirections
#define quality uQuality
#define size uSize
#define radius vec2(size * 0.1) / (iResolution.x / iResolution.y)

vec4 getBlur(in sampler2D tex, in vec2 uv, in vec2 resolution) {
    const float Pi = 3.14159265359;
    const float Pi2 = Pi * 2.0;
    
    vec4 color = texture(tex, uv);
    float total = 1.0;
    for(float d = 0.0; d < Pi2; d += Pi2 / directions) {
        for(float i = 1.0 / quality; i <= 1.0; i += 1.0 / quality) {
            float w = i;
            total += w;
            color += texture(tex, uv + vec2(cos(d), sin(d)) * radius * i) * w; 
        }
    }
    color = color / total;
    
    return color;
}


vec4 blend(in vec4 src, in vec4 dst) {
    return src * src.a + dst * (1.0 - src.a);
}

void main()
{
    vec2 uv = fragCoord/iResolution.xy;

    vec4 shadow = getBlur(iChannel0, uv, iResolution.xy);
    shadow *= vec4(0.0, 0.0, 0.0, 1.0);

    vec4 original = texture(iChannel0, uv); 
    
    vec4 result = shadow;
    result = blend(original, result);
    fragColor = result;
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, uResolution.x / uResolution.y, 1.0);
}