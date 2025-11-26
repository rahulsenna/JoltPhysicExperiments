#version 410 core
in vec3 frag_normal;
in vec3 frag_color;
in vec3 frag_pos;

uniform vec3 light_pos;
uniform vec3 view_pos;

out vec4 out_color;

void main() {
    vec3 norm = normalize(frag_normal);
    vec3 light_dir = normalize(light_pos - frag_pos);
    
    float ambient = 0.3;
    float diffuse = max(dot(norm, light_dir), 0.0) * 0.7;
    
    vec3 view_dir = normalize(view_pos - frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float specular = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0) * 0.5;
    
    vec3 result = (ambient + diffuse + specular) * frag_color;
    out_color = vec4(result, 1.0);
}