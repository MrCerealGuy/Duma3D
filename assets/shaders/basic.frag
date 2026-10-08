#version 330 core
in vec3 vNormal;
in vec3 vDiffuseColor;
in vec2 vTexCoord;
in vec3 vWorldPosition;
in vec3 vSpecularColor;
in float vShininess;
uniform sampler2D uDiffuseTexture;
uniform vec3 uAlbedo;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uCameraPosition;
out vec4 FragColor;
void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(-uLightDirection);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    float specular = diffuse > 0.0
        ? pow(max(dot(normal, halfwayDirection), 0.0), max(vShininess, 1.0))
        : 0.0;
    vec3 lighting = uAmbientColor + uLightColor * diffuse;
    vec3 textureColor = texture(uDiffuseTexture, vTexCoord).rgb;
    vec3 diffuseColor = uAlbedo * vDiffuseColor * textureColor * lighting;
    vec3 specularColor = vSpecularColor * uLightColor * specular;
    FragColor = vec4(diffuseColor + specularColor, 1.0);
}
