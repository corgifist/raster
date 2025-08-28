// Created by jianinz in 2022-01-26 - https://www.shadertoy.com/view/fsjyzW

// Ported to Flame Matchbox by Ted Stanley (KuleshovEffect) - February, 2023
// v1.0 

#define PI 3.141592653589793238

uniform sampler2D back, front;
uniform float adsk_back_w, adsk_back_h;
uniform float adsk_result_w, adsk_result_h;
uniform float adsk_time;

uniform float amplitude, ordinaryFrequency, frequency, lambda;

void main( void )
{
    vec2 iResolution = vec2(adsk_result_w, adsk_result_h);
    float iTime = adsk_time / 24.0;
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = gl_FragCoord.xy/iResolution.xy;

    // amplitude
    //float amplitude = 0.5;

    // ordinary frequency
    //float ordinaryFrequency = 5.59;
    
    // frequency
    //float frequency = 10.0;

    // initial phase
    float initialPhase = frequency * iTime;
    
    // wave length
    //float lambda = 10.0;

    // y(t)=A * sin(2*pi*ft + phase)
    float y = amplitude * sin((2.0 * PI * ordinaryFrequency * uv.x) + initialPhase);

    uv.y += y / lambda;

    // Output to screen
    //gl_FragColor = texture(iChannel0, uv);
    gl_FragColor = texture2D(back, uv);
}