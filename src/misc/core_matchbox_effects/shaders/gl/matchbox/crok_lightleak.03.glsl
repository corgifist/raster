
// bubbles

// rakesh@picovico.com : www.picovico.com
// http://glsl.heroku.com/e#15514.0

uniform int bubbles;
uniform float Speed;
uniform float Offset;
uniform float Sharpness;
uniform float adsk_time;
uniform float adsk_result_w, adsk_result_h;

const float fRadius = 0.05;

void main(void)
{
	vec2 resolution = vec2(adsk_result_w, adsk_result_h);
	float time = adsk_time*.05 * Speed + Offset+100.;
	vec2 uv = -1.0 + 2.0*gl_FragCoord.xy / resolution.xy;
	uv.x *=  resolution.x / resolution.y;
	vec3 col = vec3(1.0); 
	vec3 color = vec3(0.0);

    	// bubbles
	for( int i=0; i<bubbles; i++ )
	{
        	// bubble seeds
		float pha = sin(float(i)*5.13+1.0)*0.5 + 0.5;
		float siz = pow( sin(float(i)*1.74+5.0)*0.5+ 3., 4.0 );
		float pox = sin(float(i)*3.55+4.1) * resolution.x / resolution.y;
		
        // buble size, position and color
		float rad = fRadius + sin(float(i))*0.12*0.5+0.08*60.0;
		vec2  pos = vec2( pox+sin(siz), -1.0-rad + (2.0+2.0*rad)*mod(pha*(0.2+0.8),1.0)) * vec2(1.0, 1.0);
		float dis = length( uv - pos );
		vec3  col = mix( vec3(0.5), vec3(0.3), 0.5+0.5*sin(float(i)*sin(time*pox*0.03)+1.9));
		       
	    // render
		color += col.xyz *(1.- smoothstep( rad*((0.0-.02)*sin(pox*time)), rad, dis )) * (1.0 - cos(pox*time));
	}

	color = mix(color, col, 0.5);
	gl_FragColor = vec4(color, 0.0);
}