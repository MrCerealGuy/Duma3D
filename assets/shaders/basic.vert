#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aDiffuseColor;
layout(location = 3) in vec2 aTexCoord;
uniform mat4 uMVP;
uniform mat4 uModel;
out vec3 vNormal;
out vec3 vDiffuseColor;
out vec2 vTexCoord;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vDiffuseColor = aDiffuseColor;
    vTexCoord = aTexCoord;
}
