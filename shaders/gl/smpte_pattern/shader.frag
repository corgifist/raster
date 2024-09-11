#version 310 es

#ifdef GL_ES
precision highp float;
#endif

// This shader was taken from https://www.shadertoy.com/view/ctByzK
// And was modified in order to be compatible with Raster
// Many thanks to martymarty (https://www.shadertoy.com/user/martymarty) on Shadertoy!

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

void main() {
    vec2 u = gl_FragCoord.xy / uResolution;
    vec4 O = vec4(vec3(0.0), 1.0);

    float r = u.x*7.;
    vec4 z = vec4(0), v = .075+z,
         c = vec4(0,.22,.35,.5);
   
    O = mod( ceil(r/vec4( 2, 4, 1, 0)) , 2. );
    
    O = u.y > .33 ? O * .75
      : u.y > .25 ? vec4(1.-O.xy, O.zz) * .75*O.z
      : r < 1.25  ? c
      : r < 2.5   ? v/v
      : r < 3.75  ? c.yxwx
      : r < 5.    ? v
      : r < 5.33  ? z 
      : r < 6. && r > 5.67 ? z+.15 
      : v;

    O.a = 1.0;
    gColor = O;
    gUV = vec4(u, 0.0, 1.0);
}