#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

#define iResolution uResolution
#define fragCoord gl_FragCoord

uniform sampler2D uTexture;
uniform vec2 uResolution;
uniform float uSigma;
uniform float uBSigma;
uniform float uOpacity;

#define SIGMA uSigma
#define BSIGMA uBSigma
#define MSIZE 15
#define iChannel0 uTexture
#define fragColor gColor

float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}

float normpdf3(in vec3 v, in float sigma)
{
	return 0.39894*exp(-0.5*dot(v,v)/(sigma*sigma))/sigma;
}

void main() {
	vec3 c = texture(iChannel0, fragCoord.xy / iResolution.xy).rgb;
    //declare stuff
    const int kSize = (MSIZE-1)/2;
    float kernel[MSIZE];
    vec3 final_colour = vec3(0.0);

    //create the 1-D kernel
    float Z = 0.0;
    for (int j = 0; j <= kSize; ++j)
    {
        kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), SIGMA);
    }

    vec3 cc;
    float factor;
    float bZ = 1.0/normpdf(0.0, BSIGMA);
    //read out the texels
    for (int i=-kSize; i <= kSize; ++i)
    {
        for (int j=-kSize; j <= kSize; ++j)
        {
            cc = texture(iChannel0, (fragCoord.xy+vec2(float(i),float(j))) / iResolution.xy).rgb;
            factor = normpdf3(cc-c, BSIGMA)*bZ*kernel[kSize+j]*kernel[kSize+i];
            Z += factor;
            final_colour += factor*cc;

        }
    }

    fragColor = vec4(final_colour/Z, 1.0);
    fragColor = mix(texture(iChannel0, fragCoord.xy / iResolution.xy), fragColor, uOpacity);
    vec2 uv = fragCoord.xy / iResolution.xy;
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, uResolution.x / uResolution.y, 1.0);
}