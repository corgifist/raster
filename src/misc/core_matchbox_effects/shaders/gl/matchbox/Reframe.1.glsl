
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

uniform float adsk_front_w, adsk_front_h, adsk_front_frameratio, adsk_front_pixelratio, adsk_result_w, adsk_result_h, adsk_result_frameratio;
uniform sampler2D front;
uniform bool use_fov;
uniform float image_height_0;
uniform float focal_length_0;
uniform float fov_angle_0;
uniform float rotation;
uniform bool use_abs_position;
uniform vec2 rel_position;
uniform vec2 abs_position;
uniform int scaling_mode;
uniform float scale;
uniform float focal_length_1;
uniform float fov_angle_1;
uniform float distortion_anamorphic;
uniform int force_result_par;
uniform float result_par;

void main() {
    float result_frameratio;
    vec2 coords;
    vec2 h;
    vec2 cos_phi_cos_theta;
    vec2 sin_phi_sin_theta;
    float rot_correction;
    vec2 cos_gamma_sin_gamma;
    vec2 trans_v;
    mat2 trans_m;
    float distortion_scale;
    switch (force_result_par) {
    case 0:
        result_frameratio = adsk_result_frameratio;
        break;
    case 1:
        result_frameratio = adsk_front_pixelratio * adsk_result_w / adsk_result_h;
        break;
    default:
        result_frameratio = result_par * adsk_result_w / adsk_result_h;
    }
    if (use_fov)
        h.s = 2.0 * tan(.5 * radians(fov_angle_0));
    else
        h.s = image_height_0 / focal_length_0;
    switch (scaling_mode) {
    case 0:
        h.t = 100. * h.s / scale;
        break;
    case 1:
        h.t = image_height_0 / focal_length_1;
        break;
    default:
        h.t = 2. * tan(.5 * radians(fov_angle_1));
    }
    if (use_abs_position)
        coords = h.t * abs_position / vec2(adsk_front_w, adsk_front_h);
    else
        coords = h.t * (rel_position - .5);
    coords.x *= result_frameratio;
    cos_phi_cos_theta = inversesqrt(coords * coords + 1.);
    sin_phi_sin_theta = coords * cos_phi_cos_theta;
    rot_correction = atan(sin_phi_sin_theta.s * coords.y);
    cos_gamma_sin_gamma = vec2(cos(radians(rotation) - rot_correction), sin(radians(rotation) - rot_correction));
    trans_v = vec2(-sin_phi_sin_theta.s, -cos_phi_cos_theta.s * sin_phi_sin_theta.t);
    trans_m = mat2(cos_phi_cos_theta.s, -sin_phi_sin_theta.s * sin_phi_sin_theta.t, result_frameratio, cos_phi_cos_theta.t);
    distortion_scale = 1.0 / (.25 * adsk_front_frameratio * adsk_front_frameratio / (distortion_anamorphic * distortion_anamorphic) + .25);
    if (gl_FragCoord.y < 1.) {
        if (gl_FragCoord.x < 1.) {
            gl_FragColor = vec4(h, cos_phi_cos_theta);
            return;
        }
        gl_FragColor = vec4(sin_phi_sin_theta, cos_gamma_sin_gamma);
        return;
    }
    if (gl_FragCoord.x < 1.) {
        gl_FragColor = vec4(trans_v, distortion_scale, adsk_front_frameratio);
        return;
    }
    gl_FragColor = vec4(trans_m);
}
