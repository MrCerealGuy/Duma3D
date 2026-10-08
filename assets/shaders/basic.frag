#version 330 core
in vec3 vNormal;
in vec3 vDiffuseColor;
uniform vec3 uAlbedo;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
out vec4 FragColor;
void main() {
    vec3 normal = normalize(vNormal);
    float diffuse = max(dot(normal, normalize(-uLightDirection)), 0.0);
    vec3 lighting = uAmbientColor + uLightColor * diffuse;
    FragColor = vec4(uAlbedo * vDiffuseColor * lighting, 1.0);
}
