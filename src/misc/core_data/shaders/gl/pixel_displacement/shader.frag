#version 310 es
/*
	--------------------------------
	After Effects / Displacement Map
	--------------------------------

    Mimic the After Effects "Displacement Map" effect, where a input texture is used to displace another one.
    Note :
		- Input textures are not linerarized !
		- The natural behaviour of an UV Offest mimic the "wrap pixels" option in After Effects

    - Mid-gray: has no effect.
    - Darker values: push pixel in one direction
    - Brighter values: push pixel in the other direction

	==> Change the "DispStrenght" to a lower value to reduce the effect

	Francois 'CoyHot' Grassard - 2017
*/

// Ported from https://www.shadertoy.com/view/ll2czc
// Many thanks to https://www.shadertoy.com/user/CoyHot on Shadertoy!

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform sampler2D uTexture;
uniform sampler2D uDisplace;
uniform vec2 uDisplacementStrength;

#define DispStrenght uDisplacementStrength
#define iChannel0 uDisplace
#define iChannel1 uTexture
#define fragColor gColor
#define fragCoord gl_FragCoord
#define iResolution uResolution

void main() {
    vec2 origuv = (fragCoord.xy / iResolution.xy);
    vec2 uvForP = origuv;
	vec2 uv = ((fragCoord.xy / iResolution.xy)-0.5)/vec2(iResolution.y / iResolution.x, 1);


    vec3 textDisp = texture(iChannel0, origuv).xyz;    
    uvForP -= (textDisp[0]-0.5)*DispStrenght;
    

    vec3 textCol = texture(iChannel1, uvForP).xyz;

	vec4 p = vec4(textCol.xyz,1.0);

	fragColor = p;
    uv = fragCoord.xy / iResolution.xy;
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, uResolution.x / uResolution.y, 1.0);
}