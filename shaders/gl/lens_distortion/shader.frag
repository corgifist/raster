#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// This shader was taken from https://www.shadertoy.com/view/3tXczj
// And was modified in order to be compatible with Raster
// Many thanks to peterekepeter (https://www.shadertoy.com/user/peterekepeter) on Shadertoy!

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;
uniform sampler2D uTexture;

uniform float uK1;
uniform float uK2;
uniform float uK3;
uniform float uEdge;
uniform float uDispersion;
uniform int uDarkEdges;

void main() {
    vec2 uv = gl_FragCoord.xy/uResolution.xy;
    float aspect = uResolution.x/uResolution.y;
   	vec2 disorsion = uv-.5;
    
    disorsion.x*=aspect; 
    
   	float len = length(disorsion);
    
    disorsion 
        = disorsion*uK1 
        + disorsion*len*uK2
        + disorsion*len*len*uK3;
    
    
    disorsion.x/=aspect; // aspect correction
    
    vec4 col = texture(uTexture, disorsion+.5);
    
    if (uDarkEdges == 1) {
        float edge = uEdge;
        float dispersion = uDispersion;
    	col *= vec4(
            pow(max(edge-len, 0.0), 0.2),
            pow(max(edge-dispersion-len, 0.0), 0.2),
            pow(max(edge-dispersion*2.0-len, 0.0), 0.2),
        1)*1.2;
    }

    gColor = col;
    gUV = vec4(disorsion+.5, 0.0, 1.0);
}