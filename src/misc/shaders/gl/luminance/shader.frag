#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;

uniform vec2 uResolution;
uniform vec3 uCoeffs;

uniform sampler2D uColor;

float process(float Y) {
    if ( Y <= (216./24389.)) {       // The CIE standard states 0.008856 but 216/24389 is the intent for 0.008856451679036
        return Y * (24389./27.);  // The CIE standard states 903.3, but 24389/27 is the intent, making 903.296296296296296
    } else {
        return pow(Y,(1./3.)) * 116. - 16.;
    }
}

void main() {
    vec4 texel = texture(uColor, gl_FragCoord.xy / uResolution);
    float y = (uCoeffs.r * texel.r + uCoeffs.g * texel.g + uCoeffs.b * texel.b);
    gColor = vec4(vec3(y), 1.0);
}