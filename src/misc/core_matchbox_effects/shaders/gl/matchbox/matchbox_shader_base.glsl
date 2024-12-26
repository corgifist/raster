#version 320 es

#ifdef GL_ES
precision highp float;
#endif

layout(location = 0) out vec4 gFragColor;
#define texture2D texture
#define gl_FragColor gFragColor
#ifndef saturate
#define saturate(v) clamp(v,0.,1.)
#endif

float adsk_getLuminance( in vec3 color ) {
    return (0.2126*color.r + 0.7152*color.g + 0.0722*color.b);
}

bool adsk_isSceneLinear() {
    return true;
}

vec3  adsk_hsv2rgb( vec3 c ) {
	vec4 K=vec4(1.,2./3.,1./3.,3.);
	return c.z*mix(K.xxx,saturate(abs(fract(c.x+K.xyz)*6.-K.w)-K.x),c.y);
}


MATCHBOX_CODE_GOES_HERE