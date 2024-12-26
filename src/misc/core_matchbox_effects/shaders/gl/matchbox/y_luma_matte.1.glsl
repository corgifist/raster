//http://www.crytek.com/download/Sousa_Graphics_Gems_CryENGINE3.pdf
//http://psgraphics.blogspot.com/2011/01/improved-code-for-concentric-map.html
//https://www.shadertoy.com/view/MtlGRn



// #extension GL_ARB_shader_texture_lod : enable

#define INPUT1 Front
#define ratio adsk_result_frameratio
#define PI 3.141592653589793238462643383279502884197969
#define fstops_min 0.0
#define fstops_max 4.0
#define max_focal 10.0

float adsk_getLuminance( in vec3 color );

uniform float ratio;
uniform float adsk_result_w, adsk_result_h;

uniform sampler2D INPUT1;

uniform float aperture;
uniform float focal_length;
uniform float focal_distance;
uniform vec2 depth_pick;
uniform bool pick_depth;
uniform bool invert;

uniform int pick_type;
uniform vec3 pick_col;
uniform bool use_rgb;
uniform vec3 rgb;
uniform float falloff_u;
uniform float falloff_v;

vec3 to_yuv(vec3 col)
{
    mat3 yuv = mat3
    (
        .2126, .7152, .0722,
        -.09991, -.33609, .436,
        .615, -.55861, -.05639
    );

    return col * yuv;
}

vec3 to_rgb(vec3 col)
{
    mat3 rgb = mat3
    (
        1.0, 0.0, 1.28033,
        1.0, -.21482, -.38059,
        1.0, 2.12798, 0.0
    );

    return col * rgb;
}

void main(void) {
	vec2 res = vec2(adsk_result_w, adsk_result_h);
	vec2 texel = vec2(1.0) / res;
	vec2 st = gl_FragCoord.xy / res;

	vec3 front = texture2D(INPUT1, st).rgb;
	vec3 yuv_front = to_yuv(front);
	vec3 yuv_pick = to_yuv(rgb);

	vec3 d = vec3(1.0 - distance(yuv_front, yuv_pick) * vec3(1.0, falloff_u, falloff_v));
	float max_d = clamp(max(d.g, d.b), 0.0, 1.0);

	float depth = yuv_front.r;

	depth = clamp(depth, 0.0000, 1.0);

	float fp = focal_distance;

	if (pick_type == 1) {
		fp = texture2D(INPUT1, depth_pick).r;
	} else if (pick_type == 2) {
		fp = adsk_getLuminance(pick_col);
	}


	float blur_factor = distance(fp, depth) * aperture;
	blur_factor = (1.0 - blur_factor) * focal_length;
	blur_factor = clamp(blur_factor, 0.0, 1.0);
	float matte = blur_factor;

	if (use_rgb) {
		matte = min(matte, max_d);
	}

  if (invert) {
    matte = 1.0 - matte;
  }

	gl_FragColor = vec4(front,  matte);
}
