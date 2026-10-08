#pragma once

#include "Engine/Graphics/Camera.hpp"
#include "Engine/Scene/Scene.hpp"

#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Engine::Graphics
{
    class Renderer
    {
    public:
        bool initialize(HDC deviceContext, const Scene::Scene& scene);
        void render(const Scene::Scene& scene, const Camera& camera, int width, int height);
        void shutdown();

    private:
        struct GpuSection
        {
            unsigned int texture = 0;
            int firstIndex = 0;
            int indexCount = 0;
            bool hasDiffuseTexture = false;
        };

        struct GpuMesh
        {
            unsigned int vertexArray = 0;
            unsigned int vertexBuffer = 0;
            unsigned int indexBuffer = 0;
            unsigned int indexType = 0x1405;
            std::size_t indexStride = sizeof(std::uint32_t);
            int indexCount = 0;
            Math::Vec3 boundsCenter{0.0f, 0.0f, 0.0f};
            float boundsRadius = 0.0f;
            bool hasBounds = false;
            std::vector<GpuSection> sections;
        };

        bool loadOpenGLFunctions();
        bool createShaderProgram();
        bool createShadowShaderProgram();
        bool createShadowMap();
        void renderShadowMap(const Scene::Scene& scene, const Math::Mat4& lightSpaceMatrix);
        void destroyMeshes();

        HDC m_deviceContext = nullptr;
        unsigned int m_program = 0;
        unsigned int m_shadowProgram = 0;
        unsigned int m_shadowFramebuffer = 0;
        unsigned int m_shadowTexture = 0;
        int m_mvpLocation = -1;
        int m_modelLocation = -1;
        int m_textureLocation = -1;
        int m_albedoLocation = -1;
        int m_lightDirectionLocation = -1;
        int m_lightColorLocation = -1;
        int m_ambientColorLocation = -1;
        int m_cameraPositionLocation = -1;
        int m_lightSpaceLocation = -1;
        int m_shadowTextureLocation = -1;
        int m_shadowMvpLocation = -1;
        int m_pointLightCountLocation = -1;
        std::array<int, Scene::Scene::maximumPointLights> m_pointLightPositionLocations{};
        std::array<int, Scene::Scene::maximumPointLights> m_pointLightColorLocations{};
        std::array<int, Scene::Scene::maximumPointLights> m_pointLightIntensityLocations{};
        std::array<int, Scene::Scene::maximumPointLights> m_pointLightAttenuationLocations{};
        std::vector<GpuMesh> m_meshes;
        std::vector<unsigned int> m_materialTextures;
        std::vector<unsigned int> m_textures;
        bool m_initialized = false;
    };
}
