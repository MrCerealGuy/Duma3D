#pragma once

#include "Engine/Graphics/Camera.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Graphics/Scene/GltfLoader.hpp"

#include <windows.h>

#include <filesystem>
#include <string>

namespace Duma3D::Demos::Demo_3
{
    class Application
    {
    public:
        explicit Application(std::filesystem::path modelPath);
        ~Application();
        int run();

    private:
        bool createWindow();
        bool createOpenGLContext();
        void destroyOpenGL();
        void update(float deltaTime);
        void updateHud();
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
        bool keys_[256]{};
        Engine::Graphics::Camera camera_;
        Engine::Graphics::Scene::Scene scene_;
        Engine::Graphics::Scene::ModelBounds bounds_;
        Engine::Graphics::Renderer renderer_;
    };
}
