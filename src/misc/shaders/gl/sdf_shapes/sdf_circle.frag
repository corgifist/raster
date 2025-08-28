float fSDFCircle(vec2 uv)
{
    return length(uv) - uSDFCircleRadius;
}