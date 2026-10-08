#pragma once

#include <windows.h>
#include <string>
#include <vector>

class Application {
public:
    Application(int width, int height, std::wstring title);
    ~Application();

    int run();

public:
    struct Vec3 { float x, y, z; };
    struct Mat4 { float m[16]; };

private:
    bool createWindow();
    bool createOpenGLContext();
    bool loadOpenGLFunctions();
    bool createShaderProgram();
    void destroyOpenGL();
    void render(float dt);
    void update(float dt);
    void processMouse();
    void handleKeyDown(WPARAM key);
    void handleKeyUp(WPARAM key);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND hwnd_ = nullptr;
    HDC hdc_ = nullptr;
    HGLRC glrc_ = nullptr;
    HGLRC tempContext_ = nullptr;
    int width_ = 1280;
    int height_ = 720;
    std::wstring title_;

    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int program_ = 0;
    int mvpLocation_ = -1;

    bool running_ = true;
    bool keys_[256]{};
    bool firstMouse_ = true;
    POINT lastMouse_{};
    float yaw_ = -90.0f;
    float pitch_ = 0.0f;
    Vec3 cameraPos_{0.0f, 0.0f, 3.0f};
    Vec3 cameraFront_{0.0f, 0.0f, -1.0f};
    Vec3 cameraUp_{0.0f, 1.0f, 0.0f};
};
