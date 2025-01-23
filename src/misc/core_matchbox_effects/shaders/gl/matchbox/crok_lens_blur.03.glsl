// blur blur bokeh
uniform sampler2D adsk_results_pass2;
uniform float adsk_result_w, adsk_result_h;
uniform float amount;

void main()
{
  vec2 coords = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h );
  float blur = amount * 1.15;
  int f0int = int(blur);
  vec4 accu = vec4(0);
  float energy = 0.0;
  vec4 blur_colorx = vec4(0.0);
  vec4 c_org = texture2D(adsk_results_pass2, coords).rgba;

   for( int x = -f0int; x <= f0int; x++)
   {
      vec2 currentCoord = vec2(coords.x+float(x)/adsk_result_w, coords.y);
      vec4 aSample = texture2D(adsk_results_pass2, currentCoord).rgba;
      float anEnergy = 1.0 - ( abs(float(x)) / blur);
      energy += anEnergy;
      accu+= aSample * anEnergy;
   }

   blur_colorx =
      energy > 0.0 ? (accu / energy) :
                     texture2D(adsk_results_pass2, coords).rgba;
  gl_FragColor = vec4( blur_colorx );
}
