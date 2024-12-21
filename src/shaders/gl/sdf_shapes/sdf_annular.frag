float fSDFAnnular(vec2 p) {
  return abs(SDF_ANNULAR_FUNCTION_PLACEHOLDER(p)) - uSDFAnnularIntensity;
}