// based on https://www.shadertoy.com/view/ldXBzB by luluco250

uniform sampler2D adsk_results_pass1;
uniform float adsk_result_w, adsk_result_h, adsk_time;
#define res vec2(adsk_result_w, adsk_result_h)
#define time adsk_time * 0.05
//uniform vec2 direction;

#define SAMPLES 10
uniform float amount;
uniform float aspect;

int ANGLE_SAMPLES = 4 * SAMPLES;
int OFFSET_SAMPLES = 1 * SAMPLES;
uniform float HDR_CURVE;

float degs2rads(float degrees) {
    return degrees * 0.01745329251994329576923690768489;
}

vec2 rot2D(float offset, float angle) {
    angle = degs2rads(angle);
    return vec2(cos(angle) * offset, sin(angle) * offset);
}

vec3 circle_blur(sampler2D sp, vec2 uv, vec2 scale) {
    vec2 ps = (1.0 / res.xy) * scale * amount;
    vec3 col = vec3(0.0);
    float accum = 0.0;

    for (int a = 0; a < 360; a += 360 / ANGLE_SAMPLES) {
        for (int o = 0; o < OFFSET_SAMPLES; ++o) {
			col += texture2D(sp, uv + ps * rot2D(float(o), float(a))).rgb * float(o * o);
            accum += float(o * o);
        }
    }

    return col / accum;
}

void main( void )
{
  /*
  if ( aspect > 1.0 )
    direction.x = (direction.x - 1.0) * 10.0 + 1.0; */
  vec2 dir = vec2(1.0);
  dir = vec2(dir.x / aspect, dir.y * aspect);
  vec2 uv = ( gl_FragCoord.xy / res);
  vec3 col = circle_blur(adsk_results_pass1, uv, dir);
  gl_FragColor = vec4(col, 1.0);
}
