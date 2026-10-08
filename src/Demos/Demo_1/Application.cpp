#include "Application.hpp"
#include "DemoScene.hpp"

#include <windows.h>
#include <gl/GL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <random>
#include <utility>

#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif

using PFNWGLCREATECONTEXTATTRIBSARBPROC = HGLRC (WINAPI *)(HDC, HGLRC, const int*);
using PFNWGLSWAPINTERVALEXTPROC = BOOL (WINAPI *)(int);

namespace
{
    constexpr float playerRadius = 0.28f;
    constexpr float playerEyeHeight = 1.7f;
    constexpr float playerHeight = 1.8f;
    constexpr int worldChunkRadius = 2;

    template <typename Function>
    Function getWglFunction(const char* name)
    {
        const PROC address = wglGetProcAddress(name);
        std::uintptr_t rawAddress = 0;
        static_assert(sizeof(address) == sizeof(rawAddress));
        std::memcpy(&rawAddress, &address, sizeof(address));
        if (rawAddress <= 3 || rawAddress == std::numeric_limits<std::uintptr_t>::max())
            return nullptr;

        Function function = nullptr;
        static_assert(sizeof(function) == sizeof(address));
        std::memcpy(&function, &address, sizeof(function));
        return function;
    }

    bool collidesWithScene(const Engine::Scene::Scene& scene, Engine::Math::Vec3 position)
    {
        const float feet = position.y - playerEyeHeight;
        const float head = feet + playerHeight;
        for (const Engine::Scene::CollisionBox& box : scene.colliders())
        {
            if (head <= box.minimum.y || feet >= box.maximum.y)
                continue;

            const float closestX = std::clamp(position.x, box.minimum.x, box.maximum.x);
            const float closestZ = std::clamp(position.z, box.minimum.z, box.maximum.z);
            const float offsetX = position.x - closestX;
            const float offsetZ = position.z - closestZ;
            if (offsetX * offsetX + offsetZ * offsetZ < playerRadius * playerRadius)
                return true;
        }
        return false;
    }
}

Application::Application(int width, int height, std::wstring title)
    : width_(width),
      height_(height),
      title_(std::move(title)),
      world_(
          {Duma3D::Demos::Demo_1::worldChunkSize, worldChunkRadius, std::random_device{}()},
          Duma3D::Demos::Demo_1::generateDemoChunk)
{
    world_.updateForPosition(0.0f, 0.0f);
    scene_ = world_.scene();
}

Application::~Application()
{
    if (glrc_ && hdc_)
        wglMakeCurrent(hdc_, glrc_);
    renderer_.shutdown();
    destroyOpenGL();
    if (hudFont_)
        DeleteObject(hudFont_);
    if (hudBackground_)
        DeleteObject(hudBackground_);
}

bool Application::createWindow()
{
    WNDCLASSW windowClass{};
    windowClass.style = CS_OWNDC;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = L"Duma3DWindow";
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return false;

    RECT rect{0, 0, width_, height_};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    hwnd_ = CreateWindowExW(
        0,
        windowClass.lpszClassName,
        title_.c_str(),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        windowClass.hInstance,
        this
    );
    if (!hwnd_)
        return false;

    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    hudBackground_ = CreateSolidBrush(RGB(20, 27, 38));
    hudFont_ = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_MODERN, L"Consolas");
    hudHwnd_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
        16, 16, 420, 190, hwnd_, nullptr, windowClass.hInstance, nullptr);
    if (!hudBackground_ || !hudFont_ || !hudHwnd_)
        return false;
    SendMessageW(hudHwnd_, WM_SETFONT, reinterpret_cast<WPARAM>(hudFont_), TRUE);
    updateHud();
    RAWINPUTDEVICE mouseInput{};
    mouseInput.usUsagePage = 0x01;
    mouseInput.usUsage = 0x02;
    mouseInput.hwndTarget = hwnd_;
    if (!RegisterRawInputDevices(&mouseInput, 1, sizeof(mouseInput)))
        return false;

    hdc_ = GetDC(hwnd_);
    return hdc_ != nullptr;
}

bool Application::createOpenGLContext()
{
    PIXELFORMATDESCRIPTOR pixelFormat{};
    pixelFormat.nSize = sizeof(pixelFormat);
    pixelFormat.nVersion = 1;
    pixelFormat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixelFormat.iPixelType = PFD_TYPE_RGBA;
    pixelFormat.cColorBits = 32;
    pixelFormat.cDepthBits = 24;
    pixelFormat.cStencilBits = 8;
    pixelFormat.iLayerType = PFD_MAIN_PLANE;

    const int format = ChoosePixelFormat(hdc_, &pixelFormat);
    if (!format || !SetPixelFormat(hdc_, format, &pixelFormat))
        return false;

    tempContext_ = wglCreateContext(hdc_);
    if (!tempContext_ || !wglMakeCurrent(hdc_, tempContext_))
        return false;

    const auto createContextAttributes = getWglFunction<PFNWGLCREATECONTEXTATTRIBSARBPROC>(
        "wglCreateContextAttribsARB"
    );
    if (createContextAttributes)
    {
        const int attributes[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };
        glrc_ = createContextAttributes(hdc_, nullptr, attributes);
    }

    if (glrc_)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(tempContext_);
        tempContext_ = nullptr;
        if (!wglMakeCurrent(hdc_, glrc_))
            return false;
    }
    else
    {
        glrc_ = tempContext_;
        tempContext_ = nullptr;
    }

    const auto setSwapInterval = getWglFunction<PFNWGLSWAPINTERVALEXTPROC>("wglSwapIntervalEXT");
    if (setSwapInterval)
        setSwapInterval(1);

    return true;
}

void Application::destroyOpenGL()
{
    if (glrc_)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(glrc_);
        glrc_ = nullptr;
    }
    if (tempContext_)
    {
        wglDeleteContext(tempContext_);
        tempContext_ = nullptr;
    }
    if (hdc_ && hwnd_)
    {
        ReleaseDC(hwnd_, hdc_);
        hdc_ = nullptr;
    }
    if (hwnd_)
    {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void Application::update(float dt)
{
    const float baseSpeed = keys_[VK_SHIFT] ? 6.0f : 3.0f;
    const float speed = baseSpeed * dt;
    if (!gravityMode_)
    {
        if (keys_['W']) camera_.moveForward(speed);
        if (keys_['S']) camera_.moveForward(-speed);
        if (keys_['D']) camera_.moveRight(speed);
        if (keys_['A']) camera_.moveRight(-speed);
        if (keys_[VK_SPACE]) camera_.moveUp(speed);
        if (keys_[VK_CONTROL]) camera_.moveUp(-speed);
        return;
    }

    float forward = static_cast<float>(keys_['W']) - static_cast<float>(keys_['S']);
    float right = static_cast<float>(keys_['D']) - static_cast<float>(keys_['A']);
    const float inputLength = std::sqrt(forward * forward + right * right);
    if (inputLength > 1.0f)
    {
        forward /= inputLength;
        right /= inputLength;
    }

    const Engine::Math::Vec3 startingPosition = camera_.position();
    camera_.moveOnGround(forward * speed, 0.0f);
    if (collidesWithScene(scene_, camera_.position()))
        camera_.setPosition(startingPosition);
    const Engine::Math::Vec3 afterForward = camera_.position();
    camera_.moveOnGround(0.0f, right * speed);
    if (collidesWithScene(scene_, camera_.position()))
        camera_.setPosition(afterForward);

    Engine::Math::Vec3 position = camera_.position();
    const float groundHeight = Duma3D::Demos::Demo_1::sampleDemoTerrainHeight(
        position.x, position.z, world_.config().seed);
    float feet = position.y - playerEyeHeight;
    const bool onGround = feet <= groundHeight + 0.02f && verticalVelocity_ <= 0.0f;
    if (jumpRequested_ && onGround)
        verticalVelocity_ = 6.0f;
    jumpRequested_ = false;

    verticalVelocity_ -= 18.0f * dt;
    float nextFeet = feet + verticalVelocity_ * dt;
    bool hitCeiling = false;
    if (verticalVelocity_ > 0.0f)
    {
        for (const Engine::Scene::CollisionBox& box : scene_.colliders())
        {
            const float closestX = std::clamp(position.x, box.minimum.x, box.maximum.x);
            const float closestZ = std::clamp(position.z, box.minimum.z, box.maximum.z);
            const float offsetX = position.x - closestX;
            const float offsetZ = position.z - closestZ;
            if (offsetX * offsetX + offsetZ * offsetZ >= playerRadius * playerRadius ||
                feet + playerHeight > box.minimum.y ||
                nextFeet + playerHeight <= box.minimum.y)
                continue;

            nextFeet = box.minimum.y - playerHeight;
            verticalVelocity_ = 0.0f;
            hitCeiling = true;
            break;
        }
    }
    if (!hitCeiling && nextFeet <= groundHeight)
    {
        nextFeet = groundHeight;
        verticalVelocity_ = 0.0f;
    }
    position.y = nextFeet + playerEyeHeight;
    camera_.setPosition(position);
}

void Application::updateWorldChunks()
{
    const Engine::Math::Vec3& position = camera_.position();
    if (!world_.updateForPosition(position.x, position.z))
        return;

    const Engine::Scene::Scene& nextScene = world_.scene();
    if (!renderer_.updateScene(nextScene))
    {
        OutputDebugStringW(L"Duma3D: Chunk-Szene konnte nicht an die GPU übertragen werden.\n");
        running_ = false;
        return;
    }
    scene_ = nextScene;
}

void Application::toggleMovementMode()
{
    gravityMode_ = !gravityMode_;
    verticalVelocity_ = 0.0f;
    jumpRequested_ = false;
    if (gravityMode_)
    {
        Engine::Math::Vec3 position = camera_.position();
        position.y = Duma3D::Demos::Demo_1::sampleDemoTerrainHeight(
            position.x, position.z, world_.config().seed) + playerEyeHeight;
        camera_.setPosition(position);
    }

    updateHud();
}

void Application::updateHud()
{
    if (!hwnd_)
        return;
    const wchar_t* mode = gravityMode_ ? L"Laufmodus" : L"Flugmodus";
    const std::wstring modeTitle = title_ + L" [" + mode + L"]";
    SetWindowTextW(hwnd_, modeTitle.c_str());
    if (hudHwnd_)
    {
        const wchar_t* movement = gravityMode_
            ? L"[Leertaste] Springen"
            : L"[Leertaste] Steigen  [Strg] Sinken";
        std::wstring legend = L"Duma3D  |  " + std::wstring(mode) +
            L"\r\n\r\n[WASD] Bewegen\r\n[Maus] Umsehen\r\n";
        legend += movement;
        legend += L"\r\n[Umschalt] Sprint\r\n[G] Modus wechseln\r\n[Esc] Beenden";
        SetWindowTextW(hudHwnd_, legend.c_str());
    }
}

void Application::handleRawInput(HRAWINPUT inputHandle)
{
    RAWINPUT input{};
    UINT inputSize = sizeof(input);
    if (GetRawInputData(inputHandle, RID_INPUT, &input, &inputSize, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1) ||
        input.header.dwType != RIM_TYPEMOUSE || (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
        return;

    constexpr float mouseSensitivity = 0.12f;
    const float deltaX = static_cast<float>(input.data.mouse.lLastX) * mouseSensitivity;
    const float deltaY = -static_cast<float>(input.data.mouse.lLastY) * mouseSensitivity;
    camera_.rotate(deltaX, deltaY);
}

void Application::handleKeyDown(WPARAM key)
{
    const bool wasDown = key < 256 && keys_[key];
    if (key < 256)
        keys_[key] = true;
    if (key == 'G' && !wasDown)
        toggleMovementMode();
    if (key == VK_SPACE && !wasDown && gravityMode_)
        jumpRequested_ = true;
    if (key == VK_ESCAPE)
        running_ = false;
}

void Application::handleKeyUp(WPARAM key)
{
    if (key < 256)
        keys_[key] = false;
}

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto* app = reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message)
    {
        case WM_KEYDOWN:
            if (app) app->handleKeyDown(wParam);
            return 0;
        case WM_KEYUP:
            if (app) app->handleKeyUp(wParam);
            return 0;
        case WM_INPUT:
            if (app) app->handleRawInput(reinterpret_cast<HRAWINPUT>(lParam));
            return DefWindowProcW(hwnd, message, wParam, lParam);
        case WM_SIZE:
            if (app)
            {
                app->width_ = LOWORD(lParam);
                app->height_ = HIWORD(lParam);
                if (app->hudHwnd_)
                    MoveWindow(app->hudHwnd_, 16, 16, 420, 190, TRUE);
            }
            return 0;
        case WM_CTLCOLORSTATIC:
            if (app && app->hudBackground_)
            {
                SetTextColor(reinterpret_cast<HDC>(wParam), RGB(238, 242, 248));
                SetBkColor(reinterpret_cast<HDC>(wParam), RGB(20, 27, 38));
                return reinterpret_cast<LRESULT>(app->hudBackground_);
            }
            return DefWindowProcW(hwnd, message, wParam, lParam);
        case WM_CLOSE:
            if (app) app->running_ = false;
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

int Application::run()
{
    if (!createWindow() || !createOpenGLContext() || !renderer_.initialize(hdc_, scene_))
    {
        MessageBoxW(hwnd_, L"Duma3D konnte OpenGL nicht initialisieren. Siehe Debug-Ausgabe.", L"Duma3D", MB_ICONERROR);
        return 1;
    }

    Engine::Math::Vec3 startPosition = camera_.position();
    startPosition.y = Duma3D::Demos::Demo_1::sampleDemoTerrainHeight(
        startPosition.x, startPosition.z, world_.config().seed) + playerEyeHeight;
    camera_.setPosition(startPosition);
    updateHud();

    ShowWindow(hwnd_, SW_SHOWMAXIMIZED);
    UpdateWindow(hwnd_);
    ShowCursor(FALSE);

    auto previous = std::chrono::steady_clock::now();
    MSG message{};
    while (running_)
    {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
            if (message.message == WM_QUIT)
                running_ = false;
        }

        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - previous).count();
        previous = now;
        dt = std::min(dt, 0.05f);
        update(dt);
        updateWorldChunks();
        renderer_.render(scene_, camera_, width_, height_);
    }

    ShowCursor(TRUE);
    return 0;
}
