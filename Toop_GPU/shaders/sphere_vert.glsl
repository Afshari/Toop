#version 430 core
layout(location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
out vec3 vNormal;
void main()
{
    vNormal     = normalize(aPos);
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}