// bloom shader
// based on http://myheroics.wordpress.com/2008/09/04/glsl-bloom-shader/

uniform sampler2D pFront;
uniform float pSize;
uniform float p1;
uniform float p2;

uniform float adsk_result_w, adsk_result_h;

void main()
{
   vec4 sum = vec4(0);
   vec2 texcoord = gl_FragCoord.xy / vec2(adsk_result_w, adsk_result_h);
   int j;
   int i;

   for( i= -4 ;i < 4; i++)
   {
        for (j = -3; j < 3; j++)
        {
            sum += texture2D(pFront, texcoord + vec2(j, i)*p1*0.01) * p2;
        }
   }
        {
            gl_FragColor = sum*sum*0.0075*pSize + texture2D(pFront, texcoord);
        }
}
