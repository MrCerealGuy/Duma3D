#pragma once

#include "DemoScene.hpp"
#include "Engine/Audio/AudioPlayer.hpp"
#include "Engine/Game/FirstPersonController.hpp"
#include "Engine/Graphics/Renderer.hpp"

#include <windows.h>

#include <string>

namespace Duma3D::Demos::Demo_2
{
    class Application
    {
    public:
        Application(int width, int height, std::wstring title);
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
        void rebuildLevel();
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

        HWND hwnd_ = nullptr;
        HWND hudHwnd_ = nullptr;
        HFONT hudFont_ = nullptr;
        HBRUSH hudBackground_ = nullptr;
        HDC hdc_ = nullptr;
        HGLRC glrc_ = nullptr;
        HGLRC tempContext_ = nullptr;
        int width_;
        int height_;
        std::wstring title_;
        bool running_ = true;
        bool jumpRequested_ = false;
        bool lampSputtering_ = false;
        float elapsedTime_ = 0.0f;
        bool keys_[256]{};
        Engine::Graphics::Camera camera_{{0.0f, 1.7f, 0.0f}};
        Engine::Game::FirstPersonController player_;
        Engine::Game::InteractionSystem interactions_;
        LevelState levelState_;
        Engine::Game::World::ChunkData level_;
        Engine::Graphics::Renderer renderer_;
        Engine::Audio::AudioPlayer audio_;
        std::wstring statusMessage_ = L"Relais in der Relaiskammer aktivieren";
    };
}
