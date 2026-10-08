#pragma once

#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Scene/Scene.hpp"

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
    bool gravityMode_ = true;
    bool jumpRequested_ = false;
    float verticalVelocity_ = 0.0f;
    bool keys_[256]{};
    Engine::Graphics::Camera camera_{{0.0f, 1.7f, 9.0f}};
    Engine::Graphics::Renderer renderer_;
    Engine::Scene::Scene scene_;
};
