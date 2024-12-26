
// based on https://www.shadertoy.com/view/Xtl3zf
// The MIT License
// Copyright © 2017 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction,
//including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
//subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// One way to avoid texture tile repetition one using one small texture to cover a huge area.
// Basically, it creates 8 different offsets for the texture and picks two to interpolate
// between.
//
// Unlike previous methods that tile space (https://www.shadertoy.com/view/lt2GDd or
// https://www.shadertoy.com/view/4tsGzf), this one uses a random low frequency texture
// (cache friendly) to pick the actual texture's offset.

uniform sampler2D front, adsk_results_pass1;
uniform float adsk_result_w, adsk_result_h, adsk_result_frameratio;
uniform float adsk_front_w, adsk_front_h;
uniform float shuffle1, zoom1, noise_scale, blend;

float sum( vec3 v ) { return v.x+v.y+v.z; }

vec3 textureNoTile( in vec2 x, float v )
{
    float k = texture2D( adsk_results_pass1, 0.0007*x*noise_scale ).x; // cheap (cache friendly) lookup
    float l = k*8.0;
    float i = floor( l );
    float f = fract( l );
    vec2 offa = sin(vec2(3.0,7.0)*(i+0.0)); // can replace with any other hash
    vec2 offb = sin(vec2(3.0,7.0)*(i+1.0)); // can replace with any other hash
		vec3 cola = texture2D( front, x + v*offa).xyz;
		vec3 colb = texture2D( front, x + v*offb).xyz;
    return mix( cola, colb, smoothstep(0.2,0.8,f-0.1*sum(cola-colb)) );
}

void main( void )
{
  vec2 res = vec2(adsk_result_w, adsk_result_h);
	vec2 uv = 2.0 * ((gl_FragCoord.xy / res.xy) - 0.5) * zoom1;
  vec3 col = textureNoTile(uv, shuffle1 );
	gl_FragColor = vec4( col, 1.0 );
}
