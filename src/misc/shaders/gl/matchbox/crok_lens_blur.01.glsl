// based on https://www.shadertoy.com/view/ldXBzB by luluco250

uniform sampler2D front;
uniform float adsk_result_w, adsk_result_h, adsk_time;

#define res vec2(adsk_result_w, adsk_result_h)
#define time adsk_time * 0.05

uniform float HDR_CURVE; // 4.0
uniform float threshold;
uniform float gain;
uniform float amount;

vec3 saturation(vec3 rgb, float adjustment)
{
    // Algorithm from Chapter 16 of OpenGL Shading Language
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(rgb, W));
    return mix(intensity, rgb, adjustment);
}

void main( void )
{
	vec2 uv = ( gl_FragCoord.xy / res);
  vec4 col = texture2D(front, uv);

  float luma = saturation(col.rgb, 0.0).r;
  float t = step(threshold, luma);
  col.rgb = mix(col.rgb, col.rgb * gain * (amount * 1.5 + 1.), t);
  gl_FragColor = col;
}
