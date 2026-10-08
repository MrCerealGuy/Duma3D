#pragma once

#include "Engine/Graphics/Camera.hpp"
#include "Engine/Scene/Scene.hpp"

#include <windows.h>

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
        };

        struct GpuMesh
        {
            unsigned int vertexArray = 0;
            unsigned int vertexBuffer = 0;
            unsigned int indexBuffer = 0;
            std::vector<GpuSection> sections;
        };

        bool loadOpenGLFunctions();
        bool createShaderProgram();
        void destroyMeshes();

        HDC m_deviceContext = nullptr;
        unsigned int m_program = 0;
        int m_mvpLocation = -1;
        int m_modelLocation = -1;
        int m_textureLocation = -1;
        int m_albedoLocation = -1;
        int m_lightDirectionLocation = -1;
        int m_lightColorLocation = -1;
        int m_ambientColorLocation = -1;
        std::vector<GpuMesh> m_meshes;
        bool m_initialized = false;
    };
}
