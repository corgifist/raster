
uniform sampler2D adsk_results_pass8;
uniform float x_blur, adsk_result_w, adsk_result_h;

void main()
{
   vec2 coords = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h );
   int f0int = int(x_blur);
   vec4 accu = vec4(0);
   float energy = 0.0;
   vec4 blur_bgx = vec4(0.0);
   
   for( int x = -f0int; x <= f0int; x++)
   {
      vec2 currentCoord = vec2(coords.x+float(x)/adsk_result_w, coords.y);
      vec4 aSample = texture2D(adsk_results_pass8, currentCoord).rgba;
      float anEnergy = 1.0 - ( abs(float(x)) / x_blur);
      energy += anEnergy;
      accu+= aSample * anEnergy;
   }
   
   blur_bgx = 
      energy > 0.0 ? (accu / energy) : 
                     texture2D(adsk_results_pass8, coords).rgba;
                     
   gl_FragColor = vec4( blur_bgx );
}
