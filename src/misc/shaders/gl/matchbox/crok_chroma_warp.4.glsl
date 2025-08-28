uniform sampler2D adsk_results_pass1, adsk_results_pass2;
uniform float adsk_image_w, adsk_image_h;
uniform float adsk_result_w, adsk_result_h;
uniform float chromatic_abb;
uniform int num_iter;
uniform bool add_distortion;
uniform float off_chroma;
uniform vec2 center;
uniform bool inv_ca_matte;

#define iResolution vec2(adsk_result_w, adsk_result_h)

vec2 barrelDistortion(vec2 coord, float amt)
{
	vec2 cc = (((gl_FragCoord.xy/iResolution.xy) - center ));
	float distortion = dot(cc * .3, cc);
	return coord + cc * amt * -.05;
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

vec4 spectrum_offset( float t ) {
	vec3 tmp;
	vec4 ret;
	float lo = step(t,0.5);
	float hi = 1.0-lo;
	float w = linterp( remap( t, 1.0/6.0, 5.0/6.0 ) );
	tmp = vec3(lo,1.0,hi) * vec3(1.0-w, w, 1.0-w);
	vec3 W = vec3(0.2125, 0.7154, 0.0721);
	vec3 lum = vec3(dot(tmp, W));
	ret = vec4(tmp, lum.r);
	return pow( ret, vec4(1.0/2.2) );
}

void main()
{
	vec2 uv=(gl_FragCoord.xy/iResolution.xy);
	vec4 sumcol = vec4(0.0);
	vec3 sumalpha = vec3(0.0);
	vec4 sumw = vec4(0.0);
	vec4 front = texture2D(adsk_results_pass1, uv);
	float ca_str = texture2D( adsk_results_pass2, uv).r;

	// invert chromatic abberration strength mate
	if ( inv_ca_matte )
		ca_str = 1.0 - ca_str;

	for ( int i=0; i<num_iter;++i )
	{
		float t = float(i) * (1.0 / float(num_iter));
		vec4 w_off = spectrum_offset( t );
		vec4 w_st = vec4(1.0);
		vec4 w = mix(w_st, w_off, off_chroma);
		sumw += w;
		sumcol += w * texture2D( adsk_results_pass1, barrelDistortion(uv, chromatic_abb * t * ca_str));
	}

	vec4 col = vec4(sumcol / sumw);

	col.rgb = col.rgb + front.rgb * (1.0 - col.a);

	gl_FragColor = col;
}
