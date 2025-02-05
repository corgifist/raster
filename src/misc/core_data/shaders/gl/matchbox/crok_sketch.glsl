// based on https://www.shadertoy.com/view/XtVGD1 by flockaroo
// created by florian berger (flockaroo) - 2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// trying to resemle some hand drawing style

uniform sampler2D front, noise;
uniform float adsk_result_w, adsk_result_h, adsk_time;

#define Res iResolution.xy
#define PI2 6.28318530717959

uniform int AngleNum; // 3
uniform int SampNum; // 16
uniform float amount; // 400
uniform float p_amount;
uniform float out_blend, out_blend2;
uniform float fill_blend, fill_blend2;
uniform int paper;
uniform int style;
uniform bool vignette;

vec4 getRand(vec2 pos)
{
	vec2 iResolution = vec2(adsk_result_w, adsk_result_h);
    return texture2D(noise, pos / Res / Res.y * Res.y);
}

vec4 getCol(vec2 pos)
{
	vec2 iResolution = vec2(adsk_result_w, adsk_result_h);
    vec2 uv = ((pos - Res.xy * 0.5) / Res.y * Res.y) / Res.xy + 0.5;
    vec4 c1 = texture2D(front, uv);
    vec4 e = smoothstep(vec4(-0.05), vec4(0.0), vec4(uv,vec2(1.0) - uv));
    c1 = mix(vec4(1.0, 1.0, 1.0, 0.0), c1, e.x * e.y * e.z * e.w);
    float d = clamp(dot(c1.xyz, vec3(-0.5, 1.0, -0.5)), 0.0, 1.0);
    vec4 c2 = vec4(0.7);
    return min(mix(c1, c2, 1.8 * d), 0.7);
}

vec4 getColHT(vec2 pos)
{
 	return smoothstep(0.95, 1.05, getCol(pos) * 0.8 + 0.2 + getRand(pos * 0.7));
}

float getVal(vec2 pos)
{
    vec4 c = getCol(pos);
 	return pow(abs(dot(c.xyz, vec3(0.333))), 1.0);
}

vec2 getGrad(vec2 pos, float eps)
{
   	vec2 d = vec2(eps, 0.0);
    return vec2(
        getVal(pos+d.xy)-getVal(pos-d.xy),
        getVal(pos+d.yx)-getVal(pos-d.yx)
    ) / eps / 2.0;
}

void main( void )
{
	vec2 iResolution = vec2(adsk_result_w, adsk_result_h);
	float iGlobalTime = adsk_time *.05;
  vec2 pos = gl_FragCoord.xy + 4.0 * Res.y / amount;
	vec2 uv = gl_FragCoord.xy / Res;
	vec3 org = texture2D(front, uv).rgb;
	vec4 f_col = vec4(0.0);
	float c_out = 0.0;
    vec3 c_fill = vec3(0.0);
    float sum = 0.0;
    for(int i=0; i<AngleNum; i++)
    {
        float ang = PI2 / float(AngleNum) * (float(i) + 0.8);
        vec2 v = vec2(cos(ang),sin(ang));
        for(int j=0; j<SampNum; j++)
        {
            vec2 dpos  = v.yx * vec2(1.0, -1.0) * float(j) * Res.y / amount;
            vec2 dpos2 = v.xy * float(j*j) / float(SampNum) * Res.y / amount;
	        vec2 g = vec2(0.0);
            float fact = 0.0;
            float fact2 = 0.0;

            for(float s=-1.0; s<=1.0; s+=2.0)
            {
                vec2 pos2 = pos + s * dpos + dpos2;
                vec2 pos3 = pos + (s * dpos + dpos2).yx * vec2(1.0, -1.0) * 2.0;
            	g = getGrad(pos2, 0.4);
            	fact = dot(g, v) - 0.5 * abs(dot(g, v.yx * vec2(1.0, -1.0))) * 1.6;
            	fact2 = dot(normalize(g + vec2(0.0001)), v.yx * vec2(1.0,-1.0));
                fact = clamp(fact, 0.0, 0.05);
                fact2 = abs(fact2);
                fact *= 1.0 - float(j) / float(SampNum);
            	c_out += fact;
            	c_fill += fact2 * getColHT(pos3).xyz;
            	sum += fact2;
            }
        }
    }
    c_out /= float(SampNum * AngleNum) * 0.75 / sqrt(Res.y);
    c_fill /= sum;
    c_out = 1.0 - c_out;
    c_out *= c_out * c_out;

	// add paper texture
	vec3 p_text=vec3(1.0);
	if ( paper == 1)
	{
		vec2 s = sin(pos.xy * 0.1 / sqrt(Res.y / amount));
	    p_text -= 0.5 * vec3(0.25, 0.1, 0.1) * (exp(-s.x * s.x * 80.0) + exp(-s.y * s.y * 80.0));
		p_text = mix(vec3(1.0), p_text, p_amount);
	}

	// add vignette
	float vign = 1.0;
	if ( vignette )
	{
		float r = length(pos - Res * 0.5) / Res.x;
		vign = 1.0 - r*r*r;
	}

	float matte = 1.0 - clamp(c_out * c_out, 0.0, 1.0);
	if ( style == 0 )
	{
		c_out = mix(1.0, c_out, out_blend);
		c_fill = mix(org, c_fill, fill_blend);
		f_col = vec4(vec3(c_out * c_fill * p_text * vign), matte);
	}
	if ( style == 1 )
	{
		f_col.rgb = mix(org, vec3(c_out), out_blend2);
		f_col = vec4(vec3(f_col.rgb * p_text * vign), matte);
	}
	if ( style == 2 )
	{
		f_col.rgb = mix(org, vec3(c_fill), fill_blend2);
		f_col = vec4(vec3(f_col.rgb * p_text * vign), matte);
	}

	gl_FragColor = f_col;
}
