uniform float adsk_result_w, adsk_result_h;
uniform sampler2D chroma_strength;

void main()
{
	vec2 uv = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h);
	float a = texture2D(chroma_strength, uv).r;
	gl_FragColor = vec4(a);
}
