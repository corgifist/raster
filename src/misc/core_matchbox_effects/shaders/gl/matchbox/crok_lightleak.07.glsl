
// chromatic abberations

uniform sampler2D adsk_results_pass6, strength_matte;

uniform float adsk_result_w, adsk_result_h;
uniform float c2_chromatic_abb;
uniform int c2_num_iter;
uniform float c2_d_amount;
uniform vec2 c2_center;


vec2 barrelDistortion(vec2 coord, float amt) {

	vec2 cc = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h ) - c2_center;
	float distortion = dot(cc * c2_d_amount * .3, cc);

		return coord + cc * distortion * -1. * amt;
}

float sat( float t )
{
	return clamp( t, 0.0, 1.0 );
}

float linterp( float t ) {
	return sat( 1.0 - abs( 2.0*t - 1.0 ) );
}

float remap( float t, float a, float b ) {
	return sat( (t - a) / (b - a) );
}

vec3 spectrum_offset( float t ) {
	vec3 ret;
	float lo = step(t,0.5);
	float hi = 1.0-lo;
	float w = linterp( remap( t, 1.0/6.0, 5.0/6.0 ) );
	ret = vec3(lo,1.0,hi) * vec3(1.0-w, w, 1.0-w);

	return pow( ret, vec3(1.0/2.2) );
}

void main()
{
	vec2 uv = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h );
	vec3 col = texture2D(adsk_results_pass6, uv).rgb;
	float strength = texture2D( strength_matte, uv).r;

		vec3 sumw = vec3(0.0);

		for ( int i=0; i<c2_num_iter;++i )
		{
			float t = float(i) * (1.0 / float(c2_num_iter));
			vec3 w = spectrum_offset( t );
			sumw += w;
			col += w * texture2D( adsk_results_pass6, barrelDistortion(uv, c2_chromatic_abb * t * strength ) ).rgb;
		}
		col /= sumw;

	gl_FragColor = vec4(col,  1.0 );
}
