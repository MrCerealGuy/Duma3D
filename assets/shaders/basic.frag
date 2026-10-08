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
uniform int uPointLightCount;
uniform vec3 uPointLightPositions[4];
uniform vec3 uPointLightColors[4];
uniform float uPointLightIntensities[4];
uniform vec3 uPointLightAttenuations[4];
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
    vec3 diffuseLighting = uAmbientColor + uLightColor * diffuse;
    vec3 specularLighting = vSpecularColor * uLightColor * specular;
    for (int i = 0; i < uPointLightCount; ++i) {
        vec3 lightOffset = uPointLightPositions[i] - vWorldPosition;
        float distanceToLight = length(lightOffset);
        vec3 pointDirection = lightOffset / max(distanceToLight, 0.0001);
        vec3 attenuationFactors = uPointLightAttenuations[i];
        float denominator = attenuationFactors.x + attenuationFactors.y * distanceToLight +
            attenuationFactors.z * distanceToLight * distanceToLight;
        float attenuation = uPointLightIntensities[i] / max(denominator, 0.0001);
        float pointDiffuse = max(dot(normal, pointDirection), 0.0);
        vec3 pointHalfway = normalize(pointDirection + viewDirection);
        float pointSpecular = pointDiffuse > 0.0
            ? pow(max(dot(normal, pointHalfway), 0.0), max(vShininess, 1.0))
            : 0.0;
        diffuseLighting += uPointLightColors[i] * pointDiffuse * attenuation;
        specularLighting += vSpecularColor * uPointLightColors[i] * pointSpecular * attenuation;
    }
    vec3 textureColor = texture(uDiffuseTexture, vTexCoord).rgb;
    vec3 diffuseColor = uAlbedo * vDiffuseColor * textureColor * diffuseLighting;
    FragColor = vec4(diffuseColor + specularLighting, 1.0);
}
