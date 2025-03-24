#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;

uniform mat3 uKernel;
uniform sampler2D uBase;

void main()
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = gl_FragCoord.xy / uResolution.xy;

    vec4 color = vec4(0);
    
    const float direction[3] = float[3](-1.0, 0.0, 1.0);    
    for (int x = 0; x < 3; x++)
    {
        for (int y = 0; y < 3; y++)
        {
            vec2 offset = vec2(direction[x], direction[y]) / uResolution.xy;
            color += texture(uBase, uv+offset) * uKernel[x][y];
        }
    }
    gColor = color;
}