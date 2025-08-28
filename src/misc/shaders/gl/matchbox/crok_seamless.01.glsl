uniform sampler2D adsk_texture_grid;
uniform float adsk_result_w, adsk_result_h;

void main()
{
   gl_FragColor = texture(adsk_texture_grid, gl_FragCoord.xy / vec2(adsk_result_w, adsk_result_h));
}
