#version 430 core
in  vec3 vNormal;
out vec4 FragColor;
uniform vec3 uColor;
void main()
{
    vec3 light_dir = normalize(vec3(1.0, 2.0, 1.0));
    float diff     = max(dot(vNormal, light_dir), 0.15);
    FragColor      = vec4(uColor * diff, 1.0);
}