
/*
**MIT License
**
**Copyright (c) 2023
**
**Permission is hereby granted, free of charge, to any person obtaining a copy
**of this software and associated documentation files (the "Software"), to deal
**in the Software without restriction, including without limitation the rights
**to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
**copies of the Software, and to permit persons to whom the Software is
**furnished to do so, subject to the following conditions:
**
**The above copyright notice and this permission notice shall be included in all
**copies or substantial portions of the Software.
**
**THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
**IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
**FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
**AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
**LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
**OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
**SOFTWARE.
*/

uniform float adsk_result_w, adsk_result_h;
uniform sampler2D adsk_results_pass1;
uniform bool distortion_correction;
uniform vec2 distortion_shift;
uniform float distortion_anamorphic;
uniform float distortion_a;
uniform float distortion_b;
uniform float distortion_c;
uniform int force_result_par;
uniform float result_par;

vec2 h;
vec2 cos_phi_cos_theta;
vec2 sin_phi_sin_theta;
mat2 rot_m;
vec2 trans_v;
mat2 trans_m;
float distortion_scale;
float front_frameratio;
float result_frameratio;

vec2 get_coords(vec2 coords) {
    vec2 r, d;
    float r2;
    coords /= vec2(adsk_result_w, adsk_result_h);
    coords -= .5;
    coords *= vec2(result_frameratio * h.t, h.t);
    coords = (trans_m * coords + trans_v) / (cos_phi_cos_theta.t * (sin_phi_sin_theta.s * coords.x + cos_phi_cos_theta.s) + sin_phi_sin_theta.t * coords.y);
    coords = rot_m * coords;
    coords /= h.s;
    if (distortion_correction) {
        coords.x /= distortion_anamorphic;
        d = vec2(distortion_shift.x / distortion_anamorphic, distortion_shift.y);
        r = coords - d;
        r2 = dot(r, r) * distortion_scale;
        coords = d + r * (((distortion_c * r2 + distortion_b) * r2 + distortion_a) * r2 + (1.0 - (distortion_a + distortion_b + distortion_c)));
        coords.x *= distortion_anamorphic;
    }
    coords.x /= front_frameratio;
    return coords + .5;
}

void main() {
    vec2 cos_gamma_sin_gamma;
    vec4 sum = vec4(0.);
    h = texture2D(adsk_results_pass1, vec2(.25)).rg;
    cos_phi_cos_theta = texture2D(adsk_results_pass1, vec2(.25)).ba;
    sin_phi_sin_theta = texture2D(adsk_results_pass1, vec2(.75, .25)).rg;
    cos_gamma_sin_gamma = texture2D(adsk_results_pass1, vec2(.75, .25)).ba;
    trans_v = texture2D(adsk_results_pass1, vec2(.25, .75)).rg;
    distortion_scale = texture2D(adsk_results_pass1, vec2(.25, .75)).b;
    front_frameratio = texture2D(adsk_results_pass1, vec2(.25, .75)).a;
    trans_m = mat2(texture2D(adsk_results_pass1, vec2(.75)));
    result_frameratio = trans_m[1][0];
    trans_m[1][0] = 0.;
    rot_m = mat2(cos_gamma_sin_gamma.s, -cos_gamma_sin_gamma.t, cos_gamma_sin_gamma.t, cos_gamma_sin_gamma.s);
    gl_FragColor = vec4(get_coords(gl_FragCoord.xy), 0., 0.);
}
