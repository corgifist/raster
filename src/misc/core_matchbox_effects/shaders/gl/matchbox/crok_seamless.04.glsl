

uniform float adsk_result_w, adsk_result_h;


uniform sampler2D adsk_results_pass2, adsk_results_pass3;
uniform int version;

void main()
{
	vec2 res = vec2(adsk_result_w, adsk_result_h);
	vec2 uv = gl_FragCoord.xy / res.x;
	vec4 col = vec4(0.0);

	if ( version == 0 )
  	col = texture2D( adsk_results_pass2, uv );
	else if ( version == 1 )
		col = texture2D( adsk_results_pass3, uv );

	 gl_FragColor = col;
}
