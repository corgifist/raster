#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;

uniform bool uGrayscale;
uniform sampler2D uColor;

float character(int n, vec2 p)
{
	p = floor(p*vec2(-4.0, 4.0) + 2.5);
    if (clamp(p.x, 0.0, 4.0) == p.x)
	{
        if (clamp(p.y, 0.0, 4.0) == p.y)	
		{
        	int a = int(round(p.x) + 5.0 * round(p.y));
			if (((n >> a) & 1) == 1) return 1.0;
		}	
    }
	return 0.0;
}

void main()  {
	vec2 pix = gl_FragCoord.xy;
	vec3 col = texture(uColor, floor(pix/8.0)*8.0/uResolution.xy).rgb;	
	
	float gray = 0.3 * col.r + 0.59 * col.g + 0.11 * col.b;
	    
	int n =  4096;
    
    // limited character set
    if (gray > 0.2) n = 65600;    // :
	if (gray > 0.3) n = 163153;   // *
	if (gray > 0.4) n = 15255086; // o 
	if (gray > 0.5) n = 13121101; // &
	if (gray > 0.6) n = 15252014; // 8
	if (gray > 0.7) n = 13195790; // @
	if (gray > 0.8) n = 11512810; // #
    
	vec2 p = mod(pix/4.0, 2.0) - vec2(1.0);
    
	if (uGrayscale)	col = vec3(character(n, p));
	else col = col*character(n, p);
	
	gColor= vec4(col, 1.0);
}