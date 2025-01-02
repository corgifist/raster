
// multiply luma matte with blubbles

uniform float adsk_result_w, adsk_result_h;

//histogram output
uniform sampler2D adsk_results_pass2;

//bubbles output
uniform sampler2D adsk_results_pass3;


void main()
{
	vec2 uv = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h );
	float h_col = texture2D(adsk_results_pass2, uv).a;
	vec3 b_col = texture2D(adsk_results_pass3, uv).rgb;
	
	// multiply luma matte with bubbles
	vec3 col = vec3(h_col) * b_col.rgb;
	
	gl_FragColor = vec4(col, 1.0);
}