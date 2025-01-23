// blur Matte
uniform sampler2D adsk_results_pass3;
uniform float adsk_result_w, adsk_result_h;
uniform float amount;

void main()
{
   vec2 coords = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h );
   float blur = amount * 1.15;
   int f0int = int(blur);
   vec4 accu = vec4(0);
   float energy = 0.0;
   vec4 blur_colory = vec4(0.0);

   for( int y = -f0int; y <= f0int; y++)
   {
      vec2 currentCoord = vec2(coords.x, coords.y+float(y)/adsk_result_h);
      vec4 aSample = texture2D(adsk_results_pass3, currentCoord).rgba;
      float anEnergy = 1.0 - ( abs(float(y)) / blur);
      energy += anEnergy;
      accu+= aSample * anEnergy;
   }

   blur_colory =
      energy > 0.0 ? (accu / energy) :
                     texture2D(adsk_results_pass3, coords).rgba;

   gl_FragColor = vec4( blur_colory );
}
