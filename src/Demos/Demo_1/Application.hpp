#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Game/FirstPersonController.hpp"
#include "Engine/Graphics/Scene/Scene.hpp"
#include "Engine/Game/World/ChunkWorld.hpp"

#include <windows.h>
#include <string>

class Application {
public:
    Application(int width, int height, std::wstring title);
    ~Application();

    int run();

private:
    bool createWindow();
    bool createOpenGLContext();
    void destroyOpenGL();
    void update(float dt);
    void updateWorldChunks();
    void toggleMovementMode();
    void updateHud();
    void handleRawInput(HRAWINPUT inputHandle);
    void handleKeyDown(WPARAM key);
    void handleKeyUp(WPARAM key);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND hwnd_ = nullptr;
    HWND hudHwnd_ = nullptr;
    HFONT hudFont_ = nullptr;
    HBRUSH hudBackground_ = nullptr;
    HDC hdc_ = nullptr;
    HGLRC glrc_ = nullptr;
    HGLRC tempContext_ = nullptr;
    int width_ = 1280;
    int height_ = 720;
    std::wstring title_;

    bool running_ = true;
    bool jumpRequested_ = false;
    bool keys_[256]{};
    Engine::Graphics::Camera camera_{{0.0f, 1.7f, 9.0f}};
    Engine::Game::FirstPersonController playerController_;
    Engine::Graphics::Renderer renderer_;
    Engine::Game::World::ChunkWorld world_;
    Engine::Graphics::Scene::Scene scene_;
};
