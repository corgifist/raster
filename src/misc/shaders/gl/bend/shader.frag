#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform float uAngle;
uniform vec2 uCenter;
uniform sampler2D uColor;

vec2 bend( vec2 uv, float angle )
{
    angle += 0.001;
    const float BLOCK_HEIGHT = 1.;
    const float BLOCK_WIDTH = 1.;

    float bendSign = angle < 0. ? -1. : 1.;
    float bendRadius = BLOCK_HEIGHT / abs( angle );
    
    vec2 p = uv * vec2( bendSign, 1. ) + vec2( bendRadius, 0. );
    
    return vec2( ( length( p ) - bendRadius ) / BLOCK_WIDTH * bendSign, atan( p.y, p.x ) / abs( angle ) );
}

void main()
{
    vec2 center = uCenter * 0.5;
    vec2 uv = gl_FragCoord.xy / uResolution.xy;
    uv -= .5 + center;
    
    vec2 textureUV = bend( uv, radians(uAngle));
    textureUV += .5 + center;
    // Output to screen
    gColor = texture(uColor, textureUV);
    gUV = vec4(textureUV, 0., 1.);
}