
//combining front and matte

uniform sampler2D front, matte;
uniform float adsk_result_w, adsk_result_h;

void main()
{
	vec2 uv = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h );
	vec3 col = texture2D(front, uv).rgb;
	float alpha = texture2D(matte, uv).r;

	gl_FragColor = vec4(col, alpha);
}
