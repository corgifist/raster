
// colour correction and combining

uniform sampler2D adsk_results_pass1, adsk_results_pass4, adsk_results_pass9;
uniform float adsk_result_w, adsk_result_h;

uniform float c_sat, l_blend;
uniform vec3 c_tint;
uniform bool leak_only;
uniform int blendMode;


vec4 adsk_getBlendedValue( int blendType, vec4 srcColor, vec4 dstColor );

vec3 colorDodge( vec3 s, vec3 d )
{
	return d / (1.0 - s);
}

vec3 saturation(vec3 rgb, float adjustment)
{
    // Algorithm from Chapter 16 of OpenGL Shading Language
    const vec3 W = vec3(0.2125, 0.7154, 0.0721);
    vec3 intensity = vec3(dot(rgb, W));
    return mix(intensity, rgb, adjustment);
}

void main()
{

	vec2 uv = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h );
	vec3 col = vec3(0.0);

	vec3 org_col = texture2D(adsk_results_pass1, uv).rgb;
	vec3 atmo_col = texture2D(adsk_results_pass4, uv).rgb;
	vec3 chroma_col = texture2D(adsk_results_pass9, uv).rgb;

	col = colorDodge(atmo_col, chroma_col);

	// add color correction
	col = saturation(col, c_sat);
	col *= c_tint;

	vec4 f_col = adsk_getBlendedValue( blendMode, vec4( org_col, 1.0 ), vec4( col, 1.0 ));

	f_col.rgb = mix(org_col, f_col.rgb, l_blend);

	if ( leak_only )
		f_col.rgb = mix(vec3(0.0),col, l_blend);

	gl_FragColor = f_col;

}
