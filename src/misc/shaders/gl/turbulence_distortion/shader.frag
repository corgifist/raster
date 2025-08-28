#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// Ported from https://www.shadertoy.com/view/WclSWn
// and adapted for Raster

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform float uTurbulenceNum;
uniform float uTurbulenceAmplitude;
uniform float uTurbulenceTime;
uniform float uTurbulenceFrequency;
uniform float uTurbulenceFrequencyMultiplier;
uniform float uTurbulenceSpeed;
uniform sampler2D uColor;

//Number of turbulence waves
#define TURB_NUM uTurbulenceNum
//Turbulence wave amplitude
#define TURB_AMP uTurbulenceAmplitude
//Turbulence frequency
#define TURB_FREQ uTurbulenceFrequency
//Turbulence frequency multiplier
#define TURB_EXP uTurbulenceFrequencyMultiplier
#define TURB_SPEED uTurbulenceSpeed

//Apply turbulence to coordinates
vec2 turbulence(vec2 p)
{
    //Turbulence starting scale
    float freq = TURB_FREQ;
    
    //Turbulence rotation matrix
    mat2 rot = mat2(0.6, -0.8, 0.8, 0.6);
    
    //Loop through turbulence octaves
    for(float i=0.0; i<TURB_NUM; i++)
    {
        //Scroll along the rotated y coordinate
        float phase = freq * (p * rot).y + TURB_SPEED*uTurbulenceTime + i;
        //Add a perpendicular sine wave offset
        p += TURB_AMP * rot[0] * sin(phase) / freq;
        
        //Rotate for the next octave
        rot *= mat2(0.6, -0.8, 0.8, 0.6);
        //Scale down for the next octave
        freq *= TURB_EXP;
    }
    
    return p;
}

void main() {
    //Screen coordinates, centered and aspect corrected
    vec2 p = 2.0*(gl_FragCoord.xy*2.0-uResolution.xy)/uResolution.y;
    
    //Apply Turbulence
    p = turbulence(p);
    
    gColor = texture(uColor, p);
    gUV = vec4(p, uResolution.x / uResolution.y, 1.0);
}