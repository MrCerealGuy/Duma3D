#pragma once

#include "Engine/Graphics/Camera.hpp"
#include "Engine/Game/FirstPersonController.hpp"
#include "Engine/Game/Collision/CollisionBox.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/Scene/GltfLoader.hpp"

#include <windows.h>

#include <filesystem>
#include <string>
#include <vector>

namespace Duma3D::Demos::Demo_3
{
    class Application
    {
    public:
        explicit Application(std::filesystem::path modelPath);
        ~Application();
        int run();

    private:
        struct WalkableTriangle
        {
            Engine::Math::Vec3 a;
            Engine::Math::Vec3 b;
            Engine::Math::Vec3 c;
            float minimumX;
            float maximumX;
            float minimumZ;
            float maximumZ;
        };

        bool createWindow();
        bool createOpenGLContext();
        void destroyOpenGL();
        void update(float deltaTime);
        void updateHud();
        float sampleGroundHeight(float worldX, float worldZ) const;
        void buildWalkGeometry();
        void handleRawInput(HRAWINPUT input);
        void handleKeyDown(WPARAM key);
        void handleKeyUp(WPARAM key);
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

        std::filesystem::path modelPath_;
        HWND hwnd_ = nullptr;
        HWND hudHwnd_ = nullptr;
        HFONT hudFont_ = nullptr;
        HBRUSH hudBackground_ = nullptr;
        HDC hdc_ = nullptr;
        HGLRC glrc_ = nullptr;
        HGLRC tempContext_ = nullptr;
        int width_ = 1280;
        int height_ = 720;
        bool running_ = true;
        bool jumpRequested_ = false;
        bool keys_[256]{};
        Engine::Graphics::Camera camera_;
        Engine::Game::FirstPersonController playerController_{
            {1.8f, 2.0f, {0.14f, 0.9f, 0.82f, 12.7f, 4.25f}}};
        Engine::Graphics::Scene::Scene scene_;
        Engine::Graphics::Scene::ModelBounds bounds_;
        std::vector<Engine::Game::Collision::Box> colliders_;
        std::vector<WalkableTriangle> walkableSurfaces_;
        Engine::Graphics::Renderer renderer_;
    };
}
