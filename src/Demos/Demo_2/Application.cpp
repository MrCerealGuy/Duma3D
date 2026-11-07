#include "Application.hpp"

#include <windows.h>
#include <gl/GL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <utility>

#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif

using PFNWGLCREATECONTEXTATTRIBSARBPROC = HGLRC (WINAPI *)(HDC, HGLRC, const int*);
using PFNWGLSWAPINTERVALEXTPROC = BOOL (WINAPI *)(int);

namespace Duma3D::Demos::Demo_2
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

    Application::Application(int width, int height, std::wstring title, std::filesystem::path levelPath)
        : width_(width), height_(height), title_(std::move(title)),
          levelDefinition_(Engine::Game::World::IndoorLevelLoader::load(levelPath)),
          levelState_(Engine::Game::World::IndoorLevelBuilder::initialState(levelDefinition_)),
          level_(Engine::Game::World::IndoorLevelBuilder::build(levelDefinition_, levelState_))
    {
        title_ += L" | " + std::wstring(levelDefinition_.name.begin(), levelDefinition_.name.end());
        std::vector<Engine::Game::EnemySpawn> enemySpawns;
        enemySpawns.reserve(levelDefinition_.enemies.size());
        for (const auto& enemy : levelDefinition_.enemies)
        {
            const auto room = std::find_if(levelDefinition_.rooms.begin(), levelDefinition_.rooms.end(),
                [&enemy](const auto& item) { return item.id == enemy.room; });
            enemySpawns.push_back({{room->x + enemy.offsetX, 0.0f, room->z + enemy.offsetZ},
                enemy.type == "demon" ? Engine::Game::EnemyType::Demon : Engine::Game::EnemyType::Grunt});
        }
        combat_.initialize(std::move(enemySpawns));
        combat_.attachScene(level_.graphics);
        Engine::Game::World::IndoorLevelBuilder::placeAtSpawn(levelDefinition_, camera_);
        Engine::Game::World::IndoorLevelBuilder::registerInteractions(
            levelDefinition_, levelState_, interactions_, camera_);
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
        windowClass.lpszClassName = L"Duma3DDemo2Window";
        windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;

        RECT rect{0, 0, width_, height_};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        hwnd_ = CreateWindowExW(0, windowClass.lpszClassName, title_.c_str(),
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr,
            windowClass.hInstance, this);
        if (!hwnd_) return false;
        SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        hudBackground_ = CreateSolidBrush(RGB(15, 16, 19));
        hudFont_ = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN, L"Consolas");
        hudHwnd_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
            16, 16, 470, 245, hwnd_, nullptr, windowClass.hInstance, nullptr);
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

    void Application::update(float dt)
    {
        elapsedTime_ += dt;
        shotCooldown_ = std::max(0.0f, shotCooldown_ - dt);
        audio_.update(dt);
        if (Engine::Game::World::IndoorLevelBuilder::advanceDoors(levelDefinition_, levelState_, dt))
            rebuildLevel();
        const bool sputtering = Engine::Game::World::IndoorLevelBuilder::updateLampFlicker(
            levelDefinition_, level_.graphics, elapsedTime_);
        if (sputtering && !lampSputtering_)
            audio_.play("assets/audio/lamp_flicker.wav", Engine::Audio::Priority::Flicker);
        lampSputtering_ = sputtering;

        const Engine::Game::Physics::GroundHeightSampler ground = [](float, float) { return 0.0f; };
        const float canMove = combat_.health() > 0.0f ? 1.0f : 0.0f;
        const Engine::Game::FirstPersonInput input{
            (static_cast<float>(keys_['W']) - static_cast<float>(keys_['S'])) * canMove,
            (static_cast<float>(keys_['D']) - static_cast<float>(keys_['A'])) * canMove,
            0.0f,
            keys_[VK_SHIFT],
            jumpRequested_ && canMove > 0.0f
        };
        player_.update(level_.colliders, camera_, input, dt, ground);
        jumpRequested_ = false;
        combat_.update(dt, camera_.position(), level_.colliders, level_.graphics);
        combat_.updateWeapon(camera_, level_.graphics);
        if (combat_.health() <= 0.0f)
            statusMessage_ = L"Du bist gefallen";
        updateHud();
    }

    void Application::updateHud()
    {
        if (hudHwnd_)
        {
            const Engine::Game::Interactable* target = interactions_.target(camera_);
            const std::wstring levelName(levelDefinition_.name.begin(), levelDefinition_.name.end());
            std::wstring text = L"DUMA3D // " + levelName + L"\r\n";
            text += L"HP " + std::to_wstring(static_cast<int>(combat_.health())) + L"    Gegner " +
                std::to_wstring(combat_.enemiesAlive()) + L"\r\nPistole  " +
                std::to_wstring(combat_.magazine()) + L" / ∞\r\n\r\n";
            text += L"[WASD] Bewegen   [Maus] Umsehen\r\n[Shift] Sprint   [Space] Springen\r\n";
            text += L"[LMB] Feuern   [R] Nachladen\r\n[E] Interagieren   [Esc] Beenden\r\n\r\n";
            if (target) text += target->prompt + L"\r\n";
            text += statusMessage_;
            SetWindowTextW(hudHwnd_, text.c_str());
        }
    }

    void Application::rebuildLevel()
    {
        level_ = Engine::Game::World::IndoorLevelBuilder::build(levelDefinition_, levelState_);
        combat_.attachScene(level_.graphics);
        if (!renderer_.updateScene(level_.graphics))
        {
            OutputDebugStringW(L"Duma3D Demo 2: Szenenupdate fehlgeschlagen.\n");
            running_ = false;
        }
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
        if (key == VK_ESCAPE) running_ = false;
        if (key == VK_SPACE && !wasDown) jumpRequested_ = true;
        if (key == 'R' && !wasDown)
        {
            combat_.reload();
            statusMessage_ = L"Pistole nachgeladen";
        }
        if (key == 'E' && !wasDown)
        {
            const Engine::Game::InteractionResult result = interactions_.interact(camera_);
            if (result.activated)
            {
                statusMessage_ = result.message;
                if (result.worldChanged) rebuildLevel();
                switch (result.sound)
                {
                    case Engine::Game::InteractionSound::DoorOpen:
                        audio_.play("assets/audio/door_open.wav", Engine::Audio::Priority::Door);
                        break;
                    case Engine::Game::InteractionSound::DoorClose:
                        audio_.play("assets/audio/door_close.wav", Engine::Audio::Priority::Door);
                        break;
                    case Engine::Game::InteractionSound::Switch:
                        audio_.play("assets/audio/relay_switch.wav", Engine::Audio::Priority::Switch);
                        break;
                    case Engine::Game::InteractionSound::Teleporter:
                        audio_.play("assets/audio/teleporter.wav", Engine::Audio::Priority::Teleporter);
                        break;
                    case Engine::Game::InteractionSound::Denied:
                        audio_.play("assets/audio/access_denied.wav", Engine::Audio::Priority::Alert);
                        break;
                    case Engine::Game::InteractionSound::None:
                        break;
                }
            }
            else
                statusMessage_ = L"Nichts in Reichweite";
        }
        updateHud();
    }

    void Application::handleFire()
    {
        if (shotCooldown_ > 0.0f || combat_.health() <= 0.0f)
            return;
        const Engine::Game::ShotResult result = combat_.fire(
            camera_.position(), camera_.front(), level_.colliders);
        if (!result.fired)
        {
            statusMessage_ = L"Magazin leer - [R] Nachladen";
            updateHud();
            return;
        }
        shotCooldown_ = 0.28f;
        statusMessage_ = result.killed ? L"Gegner ausgeschaltet" : result.hit ? L"Treffer" : L"Schuss daneben";
        audio_.play("assets/audio/pistol.wav", Engine::Audio::Priority::Weapon);
        if (result.killed)
            audio_.play("assets/audio/enemy_death.wav", Engine::Audio::Priority::EnemyDeath);
        else if (result.hit)
            audio_.play("assets/audio/enemy_hit.wav", Engine::Audio::Priority::Hit);
        updateHud();
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
            case WM_LBUTTONDOWN: if (app) app->handleFire(); return 0;
            case WM_INPUT:
                if (app) app->handleRawInput(reinterpret_cast<HRAWINPUT>(lParam));
                return DefWindowProcW(hwnd, message, wParam, lParam);
            case WM_SIZE:
                if (app)
                {
                    app->width_ = LOWORD(lParam);
                    app->height_ = HIWORD(lParam);
                    if (app->hudHwnd_) MoveWindow(app->hudHwnd_, 16, 16, 470, 245, TRUE);
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
        if (!createWindow() || !createOpenGLContext() || !renderer_.initialize(hdc_, level_.graphics))
        {
            MessageBoxW(hwnd_, L"Duma3D Demo 2 konnte OpenGL nicht initialisieren. Siehe Debug-Ausgabe.",
                L"Duma3D Demo 2", MB_ICONERROR);
            return 1;
        }
        if (!audio_.loop("assets/audio/sector_ambience.wav"))
            OutputDebugStringW(L"Duma3D Demo 2: Maschinenraum-Ambience konnte nicht gestartet werden.\n");
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
            float dt = std::chrono::duration<float>(now - previous).count();
            previous = now;
            dt = std::min(dt, 0.05f);
            update(dt);
            renderer_.render(level_.graphics, camera_, width_, height_);
        }
        ShowCursor(TRUE);
        return 0;
    }
}
