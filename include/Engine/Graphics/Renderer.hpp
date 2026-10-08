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
        struct GpuMesh
        {
            unsigned int vertexArray = 0;
            unsigned int vertexBuffer = 0;
            int vertexCount = 0;
        };

        bool loadOpenGLFunctions();
        bool createShaderProgram();
        void destroyMeshes();

        HDC m_deviceContext = nullptr;
        unsigned int m_program = 0;
        int m_mvpLocation = -1;
        std::vector<GpuMesh> m_meshes;
        bool m_initialized = false;
    };
}
