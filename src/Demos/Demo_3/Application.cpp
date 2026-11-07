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
        buildWalkGeometry();
        playerController_.placeOnGround(camera_, [this](float x, float z)
        {
            return sampleGroundHeight(x, z);
        });
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
        const Engine::Game::FirstPersonInput input{
            static_cast<float>(keys_['W']) - static_cast<float>(keys_['S']),
            static_cast<float>(keys_['D']) - static_cast<float>(keys_['A']),
            static_cast<float>(keys_[VK_SPACE]) - static_cast<float>(keys_[VK_CONTROL]),
            keys_[VK_SHIFT],
            jumpRequested_
        };
        const Engine::Game::Physics::GroundHeightSampler ground = [this](float x, float z)
        {
            return sampleGroundHeight(x, z);
        };
        playerController_.update(colliders_, camera_, input, deltaTime, ground);
        jumpRequested_ = false;
    }

    float Application::sampleGroundHeight(float worldX, float worldZ) const
    {
        constexpr float eyeHeight = 0.82f;
        const float highestGround = camera_.position().y - eyeHeight + 0.24f;
        float result = -std::numeric_limits<float>::infinity();
        for (const WalkableTriangle& triangle : walkableSurfaces_)
        {
            if (worldX < triangle.minimumX || worldX > triangle.maximumX ||
                worldZ < triangle.minimumZ || worldZ > triangle.maximumZ)
                continue;
            const float v0x = triangle.b.x - triangle.a.x;
            const float v0z = triangle.b.z - triangle.a.z;
            const float v1x = triangle.c.x - triangle.a.x;
            const float v1z = triangle.c.z - triangle.a.z;
            const float v2x = worldX - triangle.a.x;
            const float v2z = worldZ - triangle.a.z;
            const float d00 = v0x * v0x + v0z * v0z;
            const float d01 = v0x * v1x + v0z * v1z;
            const float d11 = v1x * v1x + v1z * v1z;
            const float d20 = v2x * v0x + v2z * v0z;
            const float d21 = v2x * v1x + v2z * v1z;
            const float denominator = d00 * d11 - d01 * d01;
            if (std::abs(denominator) < 1.0e-6f) continue;
            const float v = (d11 * d20 - d01 * d21) / denominator;
            const float w = (d00 * d21 - d01 * d20) / denominator;
            if (v < -0.001f || w < -0.001f || v + w > 1.001f) continue;
            const float height = triangle.a.y + v * (triangle.b.y - triangle.a.y) +
                w * (triangle.c.y - triangle.a.y);
            if (height <= highestGround && height > result)
                result = height;
        }
        if (std::isfinite(result)) return result;
        return bounds_.suggestedStart.y - eyeHeight;
    }

    void Application::buildWalkGeometry()
    {
        constexpr float maximumColliderSpan = 90.0f;
        constexpr float maximumWalkTriangleSpan = 50.0f;
        for (const Engine::Graphics::Scene::Mesh& mesh : scene_.meshes())
        {
            if (mesh.vertices.empty()) continue;
            Engine::Math::Vec3 minimum = mesh.vertices.front().position;
            Engine::Math::Vec3 maximum = minimum;
            for (const auto& vertex : mesh.vertices)
            {
                minimum.x = std::min(minimum.x, vertex.position.x);
                minimum.y = std::min(minimum.y, vertex.position.y);
                minimum.z = std::min(minimum.z, vertex.position.z);
                maximum.x = std::max(maximum.x, vertex.position.x);
                maximum.y = std::max(maximum.y, vertex.position.y);
                maximum.z = std::max(maximum.z, vertex.position.z);
            }
            const float spanX = maximum.x - minimum.x;
            const float spanY = maximum.y - minimum.y;
            const float spanZ = maximum.z - minimum.z;
            if (spanX <= maximumColliderSpan && spanY <= maximumColliderSpan && spanZ <= maximumColliderSpan)
                colliders_.push_back({minimum, maximum});

            for (std::size_t index = 0; index + 2 < mesh.indices.size(); index += 3)
            {
                const auto& a = mesh.vertices[mesh.indices[index]];
                const auto& b = mesh.vertices[mesh.indices[index + 1]];
                const auto& c = mesh.vertices[mesh.indices[index + 2]];
                const Engine::Math::Vec3 ab{b.position.x - a.position.x, b.position.y - a.position.y,
                    b.position.z - a.position.z};
                const Engine::Math::Vec3 ac{c.position.x - a.position.x, c.position.y - a.position.y,
                    c.position.z - a.position.z};
                const Engine::Math::Vec3 geometricNormal = Engine::Math::cross(ab, ac);
                const float areaTwice = std::sqrt(Engine::Math::dot(geometricNormal, geometricNormal));
                Engine::Math::Vec3 normal{
                    a.normal.x + b.normal.x + c.normal.x,
                    a.normal.y + b.normal.y + c.normal.y,
                    a.normal.z + b.normal.z + c.normal.z};
                normal = Engine::Math::normalize(normal);
                if (normal.y < 0.0f) normal = {-normal.x, -normal.y, -normal.z};
                if (normal.y < 0.65f || areaTwice < 0.08f) continue;
                const float minX = std::min({a.position.x, b.position.x, c.position.x});
                const float maxX = std::max({a.position.x, b.position.x, c.position.x});
                const float minZ = std::min({a.position.z, b.position.z, c.position.z});
                const float maxZ = std::max({a.position.z, b.position.z, c.position.z});
                if (maxX - minX > maximumWalkTriangleSpan || maxZ - minZ > maximumWalkTriangleSpan)
                    continue;
                walkableSurfaces_.push_back({a.position, b.position, c.position, minX, maxX, minZ, maxZ});
            }
        }
    }

    void Application::updateHud()
    {
        if (!hudHwnd_) return;
        const Engine::Math::Vec3 size{bounds_.maximum.x - bounds_.minimum.x,
            bounds_.maximum.y - bounds_.minimum.y, bounds_.maximum.z - bounds_.minimum.z};
        const bool walking = playerController_.mode() == Engine::Game::MovementMode::Walk;
        const wchar_t* mode = walking ? L"Laufmodus" : L"Flugmodus";
        const std::wstring title = L"Duma3D Demo 3 | " + modelPath_.filename().wstring() + L" [" + mode + L"]";
        SetWindowTextW(hwnd_, title.c_str());
        std::wstring text = L"DUMA3D // GLB LEVEL VIEWER  |  " + std::wstring(mode) + L"\r\n";
        text += std::to_wstring(scene_.objects().size()) + L" mesh instances  |  " +
            std::to_wstring(scene_.materials().size()) + L" materials\r\n";
        text += L"Bounds: " + std::to_wstring(static_cast<int>(size.x)) + L" x " +
            std::to_wstring(static_cast<int>(size.y)) + L" x " +
            std::to_wstring(static_cast<int>(size.z)) + L" m\r\n";
        text += L"View start: " + std::to_wstring(static_cast<int>(bounds_.suggestedStart.x)) + L", " +
            std::to_wstring(static_cast<int>(bounds_.suggestedStart.y)) + L", " +
            std::to_wstring(static_cast<int>(bounds_.suggestedStart.z)) + L"\r\n\r\n";
        text += L"[WASD] Bewegen   [Maus] Umsehen\r\n";
        text += walking ? L"[Space] Springen\r\n" : L"[Space/Ctrl] Hoch/Runter\r\n";
        text += L"[Shift] Sprint   [G] Modus wechseln\r\n[Esc] Beenden";
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
        const bool wasDown = key < 256 && keys_[key];
        if (key < 256) keys_[key] = true;
        if (key == 'G' && !wasDown)
        {
            playerController_.toggleMode(camera_, [this](float x, float z)
            {
                return sampleGroundHeight(x, z);
            });
            jumpRequested_ = false;
            updateHud();
        }
        if (key == VK_SPACE && !wasDown && playerController_.mode() == Engine::Game::MovementMode::Walk)
            jumpRequested_ = true;
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
