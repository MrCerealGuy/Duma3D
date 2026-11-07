#include "Application.hpp"

#include <windows.h>
#include <gl/GL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif

using PFNWGLCREATECONTEXTATTRIBSARBPROC = HGLRC (WINAPI *)(HDC, HGLRC, const int*);
using PFNWGLSWAPINTERVALEXTPROC = BOOL (WINAPI *)(int);

namespace Duma3D::Demos::Demo_3
{
    namespace
    {
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
    }

    Application::Application(std::filesystem::path modelPath) : modelPath_(std::move(modelPath))
    {
        std::string error;
        if (!Engine::Graphics::Scene::GltfLoader::loadGlb(modelPath_, scene_, bounds_, error))
            throw std::runtime_error(error);
        scene_.setDirectionalLight({{-0.35f, -1.0f, -0.25f}, {0.9f, 0.88f, 0.82f}, {0.32f, 0.32f, 0.35f}});
        camera_.setPosition(bounds_.suggestedStart);
    }

    Application::~Application()
    {
        if (glrc_ && hdc_)
            wglMakeCurrent(hdc_, glrc_);
        renderer_.shutdown();
        destroyOpenGL();
        if (hudFont_) DeleteObject(hudFont_);
        if (hudBackground_) DeleteObject(hudBackground_);
    }

    bool Application::createWindow()
    {
        WNDCLASSW windowClass{};
        windowClass.style = CS_OWNDC;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = L"Duma3DDemo3Window";
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;

        const std::wstring filename = modelPath_.filename().wstring();
        const std::wstring title = L"Duma3D Demo 3 | " + filename;
        RECT rect{0, 0, width_, height_};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        hwnd_ = CreateWindowExW(0, windowClass.lpszClassName, title.c_str(),
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr,
            windowClass.hInstance, this);
        if (!hwnd_) return false;
        SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        hudBackground_ = CreateSolidBrush(RGB(15, 16, 19));
        hudFont_ = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN, L"Consolas");
        hudHwnd_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"",
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
            16, 16, 460, 145, hwnd_, nullptr, windowClass.hInstance, nullptr);
        if (!hudBackground_ || !hudFont_ || !hudHwnd_) return false;
        SendMessageW(hudHwnd_, WM_SETFONT, reinterpret_cast<WPARAM>(hudFont_), TRUE);
        updateHud();

        RAWINPUTDEVICE mouseInput{};
        mouseInput.usUsagePage = 0x01;
        mouseInput.usUsage = 0x02;
        mouseInput.hwndTarget = hwnd_;
        if (!RegisterRawInputDevices(&mouseInput, 1, sizeof(mouseInput))) return false;
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
        if (!format || !SetPixelFormat(hdc_, format, &pixelFormat)) return false;
        tempContext_ = wglCreateContext(hdc_);
        if (!tempContext_ || !wglMakeCurrent(hdc_, tempContext_)) return false;

        const auto createContext = getWglFunction<PFNWGLCREATECONTEXTATTRIBSARBPROC>("wglCreateContextAttribsARB");
        if (createContext)
        {
            const int attributes[]{WGL_CONTEXT_MAJOR_VERSION_ARB, 3, WGL_CONTEXT_MINOR_VERSION_ARB, 3,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0};
            glrc_ = createContext(hdc_, nullptr, attributes);
        }
        if (glrc_)
        {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(tempContext_);
            tempContext_ = nullptr;
            if (!wglMakeCurrent(hdc_, glrc_)) return false;
        }
        else
        {
            glrc_ = tempContext_;
            tempContext_ = nullptr;
        }
        const auto setSwapInterval = getWglFunction<PFNWGLSWAPINTERVALEXTPROC>("wglSwapIntervalEXT");
        if (setSwapInterval) setSwapInterval(1);
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

    void Application::update(float deltaTime)
    {
        const float speed = keys_[VK_SHIFT] ? 24.0f : 8.0f;
        const float forward = static_cast<float>(keys_['W']) - static_cast<float>(keys_['S']);
        const float right = static_cast<float>(keys_['D']) - static_cast<float>(keys_['A']);
        const float vertical = static_cast<float>(keys_[VK_SPACE]) - static_cast<float>(keys_[VK_CONTROL]);
        camera_.moveOnGround(forward * speed * deltaTime, right * speed * deltaTime);
        camera_.moveUp(vertical * speed * deltaTime);
    }

    void Application::updateHud()
    {
        if (!hudHwnd_) return;
        const Engine::Math::Vec3 size{bounds_.maximum.x - bounds_.minimum.x,
            bounds_.maximum.y - bounds_.minimum.y, bounds_.maximum.z - bounds_.minimum.z};
        std::wstring text = L"DUMA3D // GLB LEVEL VIEWER\r\n";
        text += std::to_wstring(scene_.objects().size()) + L" mesh instances  |  " +
            std::to_wstring(scene_.materials().size()) + L" materials\r\n";
        text += L"Bounds: " + std::to_wstring(static_cast<int>(size.x)) + L" x " +
            std::to_wstring(static_cast<int>(size.y)) + L" x " +
            std::to_wstring(static_cast<int>(size.z)) + L" m\r\n";
        text += L"View start: " + std::to_wstring(static_cast<int>(bounds_.suggestedStart.x)) + L", " +
            std::to_wstring(static_cast<int>(bounds_.suggestedStart.y)) + L", " +
            std::to_wstring(static_cast<int>(bounds_.suggestedStart.z)) + L"\r\n\r\n";
        text += L"[WASD] Move   [Space/Ctrl] Up/Down\r\n[Shift] Fast   [Mouse] Look   [Esc] Quit";
        SetWindowTextW(hudHwnd_, text.c_str());
    }

    void Application::handleRawInput(HRAWINPUT inputHandle)
    {
        RAWINPUT input{};
        UINT inputSize = sizeof(input);
        if (GetRawInputData(inputHandle, RID_INPUT, &input, &inputSize, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1) ||
            input.header.dwType != RIM_TYPEMOUSE || (input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
            return;
        camera_.rotate(static_cast<float>(input.data.mouse.lLastX) * 0.12f,
            -static_cast<float>(input.data.mouse.lLastY) * 0.12f);
    }

    void Application::handleKeyDown(WPARAM key)
    {
        if (key < 256) keys_[key] = true;
        if (key == VK_ESCAPE) running_ = false;
    }

    void Application::handleKeyUp(WPARAM key)
    {
        if (key < 256) keys_[key] = false;
    }

    LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* app = reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        switch (message)
        {
            case WM_KEYDOWN: if (app) app->handleKeyDown(wParam); return 0;
            case WM_KEYUP: if (app) app->handleKeyUp(wParam); return 0;
            case WM_INPUT:
                if (app) app->handleRawInput(reinterpret_cast<HRAWINPUT>(lParam));
                return DefWindowProcW(hwnd, message, wParam, lParam);
            case WM_SIZE:
                if (app)
                {
                    app->width_ = LOWORD(lParam);
                    app->height_ = HIWORD(lParam);
                }
                return 0;
            case WM_CTLCOLORSTATIC:
                if (app && app->hudBackground_)
                {
                    SetTextColor(reinterpret_cast<HDC>(wParam), RGB(224, 224, 216));
                    SetBkColor(reinterpret_cast<HDC>(wParam), RGB(15, 16, 19));
                    return reinterpret_cast<LRESULT>(app->hudBackground_);
                }
                return DefWindowProcW(hwnd, message, wParam, lParam);
            case WM_CLOSE: if (app) app->running_ = false; return 0;
            case WM_DESTROY: PostQuitMessage(0); return 0;
            default: return DefWindowProcW(hwnd, message, wParam, lParam);
        }
    }

    int Application::run()
    {
        if (!createWindow() || !createOpenGLContext() || !renderer_.initialize(hdc_, scene_))
        {
            MessageBoxW(hwnd_, L"Duma3D Demo 3 konnte OpenGL nicht initialisieren. Siehe Debug-Ausgabe.",
                L"Duma3D Demo 3", MB_ICONERROR);
            return 1;
        }
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
                if (message.message == WM_QUIT) running_ = false;
            }
            const auto now = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(now - previous).count();
            previous = now;
            deltaTime = std::min(deltaTime, 0.05f);
            update(deltaTime);
            renderer_.render(scene_, camera_, width_, height_);
        }
        ShowCursor(TRUE);
        return 0;
    }
}
