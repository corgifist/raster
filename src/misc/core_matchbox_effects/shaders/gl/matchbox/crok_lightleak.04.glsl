
// atmosphere

// http://www.kamend.com/2012/06/perlin-noise-and-glsl/

uniform float adsk_time, adsk_result_w, adsk_result_h;
uniform float f_speed;

uniform float zoom, noise, brightness, contrast;


const float speed_x = 0.0;
const float speed_y = 0.0;
const float gain = 1.0;

float amplitude = 1.0;
vec2 a_center = vec2(0.0, 0.0);

const vec3 lumc = vec3(0.2125, 0.7154, 0.0721);


vec4 mod289(vec4 x)
{
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}
 
vec4 permute(vec4 x)
{
    return mod289(((x*34.0)+1.0)*x);
}
 
vec4 taylorInvSqrt(vec4 r)
{
    return 1.79284291400159 - 0.85373472095314 * r;
}
 
vec2 fade(vec2 t) {
    return t*t*t*(t*(t*6.0-15.0)+10.0);
}
 
// Classic Perlin noise
float cnoise(vec2 P)
{
    vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
    vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
    Pi = mod289(Pi); // To avoid truncation effects in permutation
    vec4 ix = Pi.xzxz;
    vec4 iy = Pi.yyww;
    vec4 fx = Pf.xzxz;
    vec4 fy = Pf.yyww;
     
    vec4 i = permute(permute(ix) + iy);
     
    vec4 gx = fract(i * (1.0 / 41.0)) * 2.0 - 1.0 ;
    vec4 gy = abs(gx) - 0.5 ;
    vec4 tx = floor(gx + 0.5);
    gx = gx - tx;
     
    vec2 g00 = vec2(gx.x,gy.x);
    vec2 g10 = vec2(gx.y,gy.y);
    vec2 g01 = vec2(gx.z,gy.z);
    vec2 g11 = vec2(gx.w,gy.w);
     
    vec4 norm = taylorInvSqrt(vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
    g00 *= norm.x;  
    g01 *= norm.y;  
    g10 *= norm.z;  
    g11 *= norm.w;  
     
    float n00 = dot(g00, vec2(fx.x, fy.x));
    float n10 = dot(g10, vec2(fx.y, fy.y));
    float n01 = dot(g01, vec2(fx.z, fy.z));
    float n11 = dot(g11, vec2(fx.w, fy.w));
     
    vec2 fade_xy = fade(Pf.xy);
    vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
    float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
    return 2.3 * n_xy;
}
 
// Classic Perlin noise, periodic variant
float pnoise(vec2 P, vec2 rep)
{
    vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
    vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
    Pi = mod(Pi, rep.xyxy); // To create noise with explicit period
    Pi = mod289(Pi);        // To avoid truncation effects in permutation
    vec4 ix = Pi.xzxz;
    vec4 iy = Pi.yyww;
    vec4 fx = Pf.xzxz;
    vec4 fy = Pf.yyww;
     
    vec4 i = permute(permute(ix) + iy);
     
    vec4 gx = fract(i * (1.0 / 41.0)) * 2.0 - 1.0 ;
    vec4 gy = abs(gx) - 0.5 ;
    vec4 tx = floor(gx + 0.5);
    gx = gx - tx;
     
    vec2 g00 = vec2(gx.x,gy.x);
    vec2 g10 = vec2(gx.y,gy.y);
    vec2 g01 = vec2(gx.z,gy.z);
    vec2 g11 = vec2(gx.w,gy.w);
     
    vec4 norm = taylorInvSqrt(vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11)));
    g00 *= norm.x;  
    g01 *= norm.y;  
    g10 *= norm.z;  
    g11 *= norm.w;  
     
    float n00 = dot(g00, vec2(fx.x, fy.x));
    float n10 = dot(g10, vec2(fx.y, fy.y));
    float n01 = dot(g01, vec2(fx.z, fy.z));
    float n11 = dot(g11, vec2(fx.w, fy.w));
     
    vec2 fade_xy = fade(Pf.xy);
    vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
    float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
    return 2.3 * n_xy;
}
 
float fbm(vec2 P, int octaves, float lacunarity, float gain)
{
    float sum = 0.0;
    float amp = 0.5 * amplitude;
    vec2 pp = P;
     
    int i;
     
	 
	for(int i=0 ; i < 3; i++)
    {
        amp *= gain; 
        sum += amp * cnoise(pp);
        pp *= lacunarity;
    }
    return sum;
 
}

 
float pattern( in vec2 p, out vec2 q, out vec2 r , in float time)
{
    float l = 2.3 * noise;
    float g = 0.4;
	int oc = 1; 
     
    float s_x = speed_x + 1.0;
    float s_y = speed_y + 1.0;

    q.x = fbm( p + vec2(1.0 + (s_x * -1.2 * time),1.0 + (s_y * -1.5 * time)),oc,l,g);
    q.y = fbm( p + vec2(1.2 * time,1.5 * time) ,oc,l,g);
     
    r.x = fbm( p + 4.0 * q + vec2(s_x *9.7,s_y * 3.8),oc,l,g );
    r.y = fbm( p + 4.0 * q + vec2(time * -9.7,time * -3.8) ,oc,l,g);
     
    return fbm( p + 1.0 * r ,oc,l,g);
}
 
void main() {
    float time = adsk_time *.0025 * f_speed;
    vec2 resolution = vec2(adsk_result_w, adsk_result_h);
	vec3 avg_lum = vec3(0.5, 0.5, 0.5);
	vec2 q = 2.0 * (((gl_FragCoord.xy / resolution.xy) - 0.5) * 0.5 * zoom) + (a_center * vec2(-1.0));

    vec2 p = -1.0 + 2.0 * q;
    vec2 qq;
	vec2 r;
	
    float col = pattern(p,qq,r,time);
	vec3 color = vec3(col);
	
	color = vec3(dot(color.rgb, lumc));
	color = mix(avg_lum, color, 0.8 * contrast);
	color = color - 1.0 + brightness;
	color = color * 2.0*gain;
	
          
    gl_FragColor = vec4(color,color);
}
