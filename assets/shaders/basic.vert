#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aDiffuseColor;
layout(location = 3) in vec2 aTexCoord;
layout(location = 4) in vec3 aSpecularColor;
layout(location = 5) in float aShininess;
layout(location = 6) in vec3 aEmissiveColor;
uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat4 uLightSpaceMatrix;
out vec3 vNormal;
out vec3 vDiffuseColor;
out vec2 vTexCoord;
out vec3 vWorldPosition;
out vec3 vSpecularColor;
out float vShininess;
out vec3 vEmissiveColor;
out vec4 vLightSpacePosition;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vec4 worldPosition = uModel * vec4(aPos, 1.0);
    vWorldPosition = worldPosition.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vDiffuseColor = aDiffuseColor;
    vTexCoord = aTexCoord;
    vSpecularColor = aSpecularColor;
    vShininess = aShininess;
    vEmissiveColor = aEmissiveColor;
    vLightSpacePosition = uLightSpaceMatrix * worldPosition;
}
