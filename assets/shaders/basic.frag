#version 330 core
in vec3 vNormal;
in vec3 vDiffuseColor;
in vec2 vTexCoord;
uniform sampler2D uDiffuseTexture;
uniform vec3 uAlbedo;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
out vec4 FragColor;
void main() {
    vec3 normal = normalize(vNormal);
    float diffuse = max(dot(normal, normalize(-uLightDirection)), 0.0);
    vec3 lighting = uAmbientColor + uLightColor * diffuse;
    vec3 textureColor = texture(uDiffuseTexture, vTexCoord).rgb;
    FragColor = vec4(uAlbedo * vDiffuseColor * textureColor * lighting, 1.0);
}
