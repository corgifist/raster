float fSDFRoundedRect(vec2 position) {
   position = abs(position) - vec2(0.5) + uSDFRoundedRectRadius;
   return length(max(position, 0.0)) + min(max(position.x, position.y), 0.0) - uSDFRoundedRectRadius;
}