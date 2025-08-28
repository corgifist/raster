float fSDFTransform(vec2 uv) {
  uv -= 0.5;

  uv += uSDFUvPosition;

  float s = sin(uSDFUvAngle);
  float c = cos(uSDFUvAngle);

  mat2 m = mat2(c, -s, s, c);
  uv += 0.5;
  uv = m * uv;
  uv *= uSDFUvSize;
  uv -= 0.5;

  uv += 0.5;
  return SDF_TRANSFORM_FUNCTION_PLACEHOLDER(uv);
}