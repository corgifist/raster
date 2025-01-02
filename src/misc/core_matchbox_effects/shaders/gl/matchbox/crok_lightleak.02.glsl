
// matte histogram adjustments

uniform sampler2D adsk_results_pass1;
uniform float adsk_result_w, adsk_result_h;

uniform float minInput;
uniform float maxInput;

float maxOutput = 1.0;
float minOutput = 0.0;

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
	vec4 color = texture2D(adsk_results_pass1, uv);
	float alpha = color.a;

	//desaturate front to get a luma matte
	color.a = float(saturation(color.rgb, 0.0));
	//levels input range
	color.a = min(max(color.a - minInput, 0.0) / (maxInput - minInput), 1.0);
	//levels output range
	color.a = mix(minOutput, maxOutput, color.a);

	//multiply luma matte with external matte
	color.a *= alpha;


	gl_FragColor = color;
}
