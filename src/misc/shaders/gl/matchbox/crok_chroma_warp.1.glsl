uniform float adsk_result_w, adsk_result_h;
uniform sampler2D front;

void main()
{
	vec2 uv = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h);
	vec3 c = texture2D(front, uv).rgb;
	gl_FragColor = vec4(c, 1.0);
}
