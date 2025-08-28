#version 310 es

#ifdef GL_ES
precision highp float;
#endif

struct Vertex {
    vec4 pos_width;
    vec4 color;
};
uniform vec2 u_viewport_size;
uniform vec2 u_aa_radius;
layout(std430, binding=0) buffer VertexData {
    Vertex vertices[];
};

out vec4 v_col;
out float v_u;
out float v_v;
out float v_line_width;
out float v_line_length;

out vec2 v_aa_radius;

void main()
{
    float u_width = u_viewport_size[0];
    float u_height = u_viewport_size[1];
    float u_aspect_ratio = u_height / u_width;
    
    // Get indices of current and next vertex
    // TODO(maciej): Double check the vertex addressing
    int line_id_0 = (gl_VertexID / 6) * 2;
    int line_id_1 = line_id_0 + 1;
    int quad_id = gl_VertexID % 6;
    ivec2 quad[6] = ivec2[6](ivec2(0, -1), ivec2(0, 1), ivec2(1,  1),
                            ivec2(0, -1), ivec2(1, 1), ivec2(1, -1) );
    
    Vertex line_vertices[2];
    line_vertices[0] = vertices[line_id_0];
    line_vertices[1] = vertices[line_id_1];
    
    vec4 clip_pos_a = vec4( line_vertices[0].pos_width.xyz, 1.0 );
    vec4 clip_pos_b = vec4( line_vertices[1].pos_width.xyz, 1.0 );
    
    vec2 ndc_pos_a = clip_pos_a.xy / clip_pos_a.w;
    vec2 ndc_pos_b = clip_pos_b.xy / clip_pos_b.w;
    
    vec2 line_vector          = ndc_pos_b - ndc_pos_a;
    vec2 viewport_line_vector = line_vector * u_viewport_size;
    vec2 dir                  = normalize( vec2( line_vector.x, line_vector.y * u_aspect_ratio ) );
    
    float extension_length = (u_aa_radius.y);
    float line_length      = length( viewport_line_vector ) + 2.0 * extension_length;
    float line_width_a     = max( line_vertices[0].pos_width.w, 1.0 ) + u_aa_radius.x;
    float line_width_b     = max( line_vertices[1].pos_width.w, 1.0 ) + u_aa_radius.x;
    
    vec2 normal    = vec2( -dir.y, dir.x );
    vec2 normal_a  = vec2( line_width_a / u_width, line_width_a / u_height ) * normal;
    vec2 normal_b  = vec2( line_width_b / u_width, line_width_b / u_height ) * normal;
    vec2 extension = vec2( extension_length / u_width, extension_length / u_height ) * dir;
    
    vec2 quad_pos = vec2(quad[ quad_id ]);
    
    v_line_width = (1.0 - quad_pos.x) * line_width_a + quad_pos.x * line_width_b;
    v_line_length = 0.5 * line_length;
    v_v = (2.0 * quad_pos.x - 1.0) * v_line_length;
    v_u = (quad_pos.y) * v_line_width;
    
    vec2 zw_part = (1.0 - quad_pos.x) * clip_pos_a.zw + quad_pos.x * clip_pos_b.zw;
    vec2 dir_y = quad_pos.y * ((1.0 - quad_pos.x) * normal_a + quad_pos.x * normal_b);
    vec2 dir_x = quad_pos.x * line_vector + (2.0 * quad_pos.x - 1.0) * extension;
    
    v_col = line_vertices[int(quad_pos.x)].color;
    v_col.a = min( line_vertices[int(quad_pos.x)].pos_width.w * v_col.a, 1.0f );
    
    gl_Position = vec4( (ndc_pos_a + dir_x + dir_y) * zw_part.y, zw_part );

    v_aa_radius = u_aa_radius;
}