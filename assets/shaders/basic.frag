#version 330 core
in vec3 vNormal;
in vec3 vDiffuseColor;
in vec2 vTexCoord;
in vec3 vWorldPosition;
in vec3 vSpecularColor;
in float vShininess;
in vec4 vLightSpacePosition;
uniform sampler2D uDiffuseTexture;
uniform sampler2D uShadowMap;
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
vec3 linearToSrgb(vec3 color) {
    color = max(color, vec3(0.0));
    vec3 lowerRange = 12.92 * color;
    vec3 upperRange = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
    return mix(upperRange, lowerRange, lessThanEqual(color, vec3(0.0031308)));
}
vec3 reinhardToneMap(vec3 color) {
    color = max(color, vec3(0.0));
    return color / (vec3(1.0) + color);
}
float directionalShadow(vec3 normal, vec3 lightDirection) {
    vec3 projected = vLightSpacePosition.xyz / vLightSpacePosition.w;
    projected = projected * 0.5 + 0.5;
    if (projected.z > 1.0 || any(lessThan(projected.xy, vec2(0.0))) ||
        any(greaterThan(projected.xy, vec2(1.0))))
        return 0.0;

    float bias = max(0.0015 * (1.0 - dot(normal, lightDirection)), 0.0003);
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    float shadow = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closestDepth = texture(uShadowMap, projected.xy + vec2(x, y) * texelSize).r;
            shadow += projected.z - bias > closestDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}
void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDirection = normalize(-uLightDirection);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);
    float specular = diffuse > 0.0
        ? pow(max(dot(normal, halfwayDirection), 0.0), max(vShininess, 1.0))
        : 0.0;
    float shadow = directionalShadow(normal, lightDirection);
    vec3 diffuseLighting = uAmbientColor + uLightColor * diffuse * (1.0 - shadow);
    vec3 specularLighting = vSpecularColor * uLightColor * specular * (1.0 - shadow);
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
    vec3 toneMappedColor = reinhardToneMap(diffuseColor + specularLighting);
    FragColor = vec4(linearToSrgb(toneMappedColor), 1.0);
}
