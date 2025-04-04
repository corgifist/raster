#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

uniform vec2 uResolution;

uniform sampler2D uColorTexture;

uniform int uColor;
uniform int uMethod;
uniform bool uPreserveLuminance;

#define AVERAGE             0
#define DOUBLE_RED_AVERAGE  1
#define DOUBLE_AVERAGE      2
#define BLUE_LIMIT          3

// Ported from https://github.com/olive-editor/olive/blob/master/app/shaders/despill.frag
// Many thanks to Olive Video Editor community!

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    
    vec4 original_col = texture(uColorTexture, uv);
    vec4 tex_col = original_col;
    float color_average = 0.0;

    if(uColor == 0) { // Green screen
        switch (uMethod) {
        case AVERAGE:
            color_average = dot(tex_col.rb, vec2(0.5)); // (tex_col.r + tex_col.b) / 2.0
            tex_col.g = tex_col.g > color_average ? color_average: tex_col.g;
            break;
        case DOUBLE_RED_AVERAGE:
            color_average = dot(tex_col.rb, vec2(2.0, 1.0) / 3.0); // (2.0 * tex_col.r + tex_col.b) / 3.0
            tex_col.g = tex_col.g > color_average ? color_average : tex_col.g;
            break;
        case DOUBLE_AVERAGE:
            color_average = dot(tex_col.br, vec2(2.0, 1.0) / 3.0); // (2.0 * tex_col.b + tex_col.r) / 3.0
            tex_col.g = tex_col.g > color_average ? color_average : tex_col.g;
            break;
        case BLUE_LIMIT:
            tex_col.g = tex_col.g > tex_col.b ? tex_col.b : tex_col.g;
            break;
        }
    } else { // Blue screen
        switch (uMethod) {
        case AVERAGE:
            color_average = dot(tex_col.rg, vec2(0.5)); // (tex_col.r + tex_col.g) / 2.0
            tex_col.b = tex_col.b > color_average ? color_average : tex_col.b;
            break;
        case DOUBLE_RED_AVERAGE:
            color_average = dot(tex_col.rg, vec2(2.0, 1.0) / 3.0); // (2.0 * tex_col.r + tex_col.g) / 3.0
            tex_col.b = tex_col.b > color_average ? color_average : tex_col.b;
            break;
        case DOUBLE_AVERAGE:
            color_average = dot(tex_col.gr, vec2(2.0, 1.0) / 3.0); // (2.0 * tex_col.g+ tex_col.r) / 3.0
            tex_col.b = tex_col.b > color_average ? color_average : tex_col.b;
            break;
        case BLUE_LIMIT:
            tex_col.b = tex_col.b > tex_col.g ? tex_col.g : tex_col.b;
            break;
        }
    }

    if (uPreserveLuminance) {
        vec4 diff = original_col - tex_col;
        float luma = dot(abs(diff.rgb), vec3(0.2126, 0.7152, 0.0722));
        tex_col.rgb += vec3(luma);
    }

    gColor = tex_col;

    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, uResolution.x / uResolution.y, 1.0);
}