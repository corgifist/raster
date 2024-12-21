#version 310 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gColor;
layout(location = 1) out vec4 gUV;

// Main texture input
uniform sampler2D uTexture;
uniform vec4 uColorKey;
uniform bool uMaskOnly;
uniform float uUpperTolerance;
uniform float uLowerTolerance;

uniform vec2 uResolution;

uniform sampler2D uGarbageTexture;
uniform sampler2D uCoreTexture;
uniform bool uGarbageAvailable;
uniform bool uCoreAvailable;
uniform bool uInvert;

uniform float uHighlights;
uniform float uShadows;

// Assume D65 white point
float Xn = 95.0489;
float Yn = 100.0;
float Zn = 108.8840;
float delta = 0.20689655172; // 6/29

// Ported from https://github.com/olive-editor/olive/blob/master/app/shaders/chromakey.frag
// Many thanks to Olive Video Editor community

vec4 toLinear(vec4 sRGB)
{
	bvec4 cutoff = lessThan(sRGB, vec4(0.04045));
	vec4 higher = pow((sRGB + vec4(0.055))/vec4(1.055), vec4(2.4));
	vec4 lower = sRGB/vec4(12.92);

	return mix(higher, lower, cutoff);
}

float func(float t) {
    if (t > pow(delta, 3.0)){
        return pow(t, 1.0/3.0);
    } else{
        return (t / (3.0 * delta * delta)) + 4.0/29.0;
    }
}

vec4 CIExyz_to_Lab(vec4 CIE) {
    vec4 lab;
    lab.r = 116.0 * func(CIE.g / Yn) - 16.0;
    lab.g = 500.0 * (func(CIE.r / Xn) -  func(CIE.g / Yn));
    lab.b = 200.0 * (func(CIE.g / Yn) -  func(CIE.b / Zn));
    lab.w = CIE.w;

    return lab;
}

float colorclose(vec4 col, vec4 key, float tola,float tolb) { 
    // Decides if a color is close to the specified hue
    float temp = sqrt(((key.g-col.g)*(key.g-col.g))+((key.b-col.b)*(key.b-col.b))+((key.r-col.r)*(key.r-col.r)));
    if (temp < tola) {return (0.0);} 
    if (temp < tolb) {return ((temp-tola)/(tolb-tola));} 
    return (1.0); 
}

vec4 SceneLinearToCIEXYZ_d65(vec4 srgb) {
    vec4 linear = toLinear(srgb);
    float X =  0.4124 * linear.r + 0.3576 * linear.g + 0.1805 * linear.b;
    float Y =  0.2126 * linear.r + 0.7152 * linear.g + 0.0722 * linear.b;
    float Z =  0.0193 * linear.r + 0.1192 * linear.g + 0.9505 * linear.b;
    return vec4(X, Y, Z , 1.);
}


void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    vec4 col = texture(uTexture, uv);

    vec4 unassoc = col;
    if (unassoc.a > 0.0) {
        unassoc.rgb /= unassoc.a;
    }

    // Perform color conversion
    vec4 cie_xyz = SceneLinearToCIEXYZ_d65(unassoc);
    vec4 lab = CIExyz_to_Lab(cie_xyz);

    vec4 cie_xyz_key = SceneLinearToCIEXYZ_d65(uColorKey);
    vec4 lab_key = CIExyz_to_Lab(cie_xyz_key);

    float mask = colorclose(lab, lab_key, uLowerTolerance * 100.0, uUpperTolerance * 100.0);

    mask = clamp(mask, 0.0, 1.0);

     if (uGarbageAvailable) {
        // Force anything we want to remove to be 0.0
        vec4 garbage = texture(uGarbageTexture, uv);
        // Assumes garbage is achromatic
        mask -= garbage.r;
        mask = clamp(mask, 0.0, 1.0);
    } 

    if (uCoreAvailable) {
        // Force anything we want to keep to be 1.0
        vec3 core = texture(uCoreTexture, uv).rgb;
        // Assumes core is achromatic
        mask += core.r;
        mask = clamp(mask, 0.0, 1.0);
    } 

    // Crush blacks and push whites
    mask = uShadows * (uHighlights * mask - 1.0) + 1.0;
    mask = clamp(mask, 0.0, 1.0);

    // Invert
    if (uInvert) {
        mask = 1.0 - mask;
    }

    col *= mask;

    if (!uMaskOnly) {
        gColor = col;
    } else {
        gColor = vec4(vec3(mask), 1.0);
    }
    uv -= 0.5;
    uv.x *= uResolution.x / uResolution.y;
    gUV = vec4(uv, uResolution.x / uResolution.y, 1.0);
}