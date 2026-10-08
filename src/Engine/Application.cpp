#include "Engine/Application.hpp"

#include <windows.h>
#include <gl/GL.h>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <string>
#include <utility>

// OpenGL 2.0/3.0+ enum values are not provided by the legacy Windows
// OpenGL header. We define only the values used by Duma3D.
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif

#ifndef WGL_CONTEXT_MAJOR_VERSION_ARB
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif

#ifndef APIENTRY
#define APIENTRY __stdcall
#endif

typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
typedef void (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(int, unsigned int*);
typedef void (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(unsigned int);
typedef void (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(int, const unsigned int*);
typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(int, unsigned int*);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(unsigned int, unsigned int);
typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(unsigned int, long long, const void*, unsigned int);
typedef unsigned int (APIENTRY *PFNGLCREATESHADERPROC)(unsigned int);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(unsigned int, int, const char* const*, const int*);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(unsigned int);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(unsigned int, unsigned int, int*);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(unsigned int, int, int*, char*);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(unsigned int);
typedef unsigned int (APIENTRY *PFNGLCREATEPROGRAMPROC)();
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(unsigned int, unsigned int);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(unsigned int);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(unsigned int, unsigned int, int*);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(unsigned int, int, int*, char*);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(unsigned int);
typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(unsigned int);
typedef int (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(unsigned int, const char*);
typedef void (APIENTRY *PFNGLUNIFORMMATRIX4FVPROC)(int, int, unsigned char, const float*);
typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(unsigned int);
typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(unsigned int, int, unsigned int, unsigned char, int, const void*);

typedef void (APIENTRY *PFNGLDELETEBUFFERSPROC)(int, const unsigned int*);

static PFNGLGENVERTEXARRAYSPROC glGenVertexArraysPtr;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArrayPtr;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArraysPtr;
static PFNGLGENBUFFERSPROC glGenBuffersPtr;
static PFNGLBINDBUFFERPROC glBindBufferPtr;
static PFNGLBUFFERDATAPROC glBufferDataPtr;
static PFNGLCREATESHADERPROC glCreateShaderPtr;
static PFNGLSHADERSOURCEPROC glShaderSourcePtr;
static PFNGLCOMPILESHADERPROC glCompileShaderPtr;
static PFNGLGETSHADERIVPROC glGetShaderivPtr;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLogPtr;
static PFNGLDELETESHADERPROC glDeleteShaderPtr;
static PFNGLCREATEPROGRAMPROC glCreateProgramPtr;
static PFNGLATTACHSHADERPROC glAttachShaderPtr;
static PFNGLLINKPROGRAMPROC glLinkProgramPtr;
static PFNGLGETPROGRAMIVPROC glGetProgramivPtr;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLogPtr;
static PFNGLUSEPROGRAMPROC glUseProgramPtr;
static PFNGLDELETEPROGRAMPROC glDeleteProgramPtr;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocationPtr;
static PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fvPtr;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArrayPtr;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointerPtr;
static PFNGLDELETEBUFFERSPROC glDeleteBuffersPtr;

static void* getGLProc(const char* name) {
    void* p = reinterpret_cast<void*>(wglGetProcAddress(name));
    if (p == nullptr || p == reinterpret_cast<void*>(0x1) || p == reinterpret_cast<void*>(0x2) || p == reinterpret_cast<void*>(0x3) || p == reinterpret_cast<void*>(-1)) {
        HMODULE module = GetModuleHandleW(L"opengl32.dll");
        p = reinterpret_cast<void*>(GetProcAddress(module, name));
    }
    return p;
}

static bool loadProc(void** target, const char* name) {
    *target = getGLProc(name);
    return *target != nullptr;
}

static Application::Mat4 identity() {
    Application::Mat4 r{};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

static Application::Mat4 multiply(const Application::Mat4& a, const Application::Mat4& b) {
    Application::Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int row = 0; row < 4; ++row)
            for (int k = 0; k < 4; ++k)
                r.m[c * 4 + row] += a.m[k * 4 + row] * b.m[c * 4 + k];
    return r;
}

static Application::Mat4 perspective(float fov, float aspect, float znear, float zfar) {
    const float pi = 3.14159265359f;
    const float f = 1.0f / std::tan(fov * pi / 360.0f);
    Application::Mat4 r{};
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (zfar + znear) / (znear - zfar);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zfar * znear) / (znear - zfar);
    return r;
}

static Application::Mat4 lookAt(Application::Vec3 eye, Application::Vec3 center, Application::Vec3 up) {
    auto normalize = [](Application::Vec3 v) {
        float l = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
        return Application::Vec3{v.x/l, v.y/l, v.z/l};
    };
    auto sub = [](Application::Vec3 a, Application::Vec3 b) { return Application::Vec3{a.x-b.x,a.y-b.y,a.z-b.z}; };
    auto cross = [](Application::Vec3 a, Application::Vec3 b) { return Application::Vec3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; };
    auto dot = [](Application::Vec3 a, Application::Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; };
    Application::Vec3 f = normalize(sub(center, eye));
    Application::Vec3 s = normalize(cross(f, up));
    Application::Vec3 u = cross(s, f);
    Application::Mat4 r = identity();
    r.m[0]=s.x; r.m[4]=s.y; r.m[8]=s.z;
    r.m[1]=u.x; r.m[5]=u.y; r.m[9]=u.z;
    r.m[2]=-f.x; r.m[6]=-f.y; r.m[10]=-f.z;
    r.m[12]=-dot(s,eye); r.m[13]=-dot(u,eye); r.m[14]=dot(f,eye);
    return r;
}

static std::string readText(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

Application::Application(int width, int height, std::wstring title) : width_(width), height_(height), title_(std::move(title)) {}

Application::~Application() { destroyOpenGL(); }

bool Application::createWindow() {
    WNDCLASSW wc{};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"Duma3DWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

    RECT r{0,0,width_,height_};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    hwnd_ = CreateWindowExW(0, wc.lpszClassName, title_.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                            CW_USEDEFAULT, CW_USEDEFAULT, r.right-r.left, r.bottom-r.top,
                            nullptr, nullptr, wc.hInstance, this);
    if (!hwnd_) return false;
    SetWindowLongPtrW(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    hdc_ = GetDC(hwnd_);
    return hdc_ != nullptr;
}

bool Application::createOpenGLContext() {
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(hdc_, &pfd);
    if (!pf || !SetPixelFormat(hdc_, pf, &pfd)) return false;

    tempContext_ = wglCreateContext(hdc_);
    if (!tempContext_ || !wglMakeCurrent(hdc_, tempContext_)) return false;

    auto createAttribs = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
    if (createAttribs) {
        const int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };
        glrc_ = createAttribs(hdc_, nullptr, attribs);
    }

    if (glrc_) {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(tempContext_);
        tempContext_ = nullptr;
        if (!wglMakeCurrent(hdc_, glrc_)) return false;
    } else {
        glrc_ = tempContext_;
        tempContext_ = nullptr;
    }

    return true;
}

bool Application::loadOpenGLFunctions() {
#define LOAD(x, name) if (!loadProc(reinterpret_cast<void**>(&x), name)) return false
    LOAD(glGenVertexArraysPtr, "glGenVertexArrays");
    LOAD(glBindVertexArrayPtr, "glBindVertexArray");
    LOAD(glDeleteVertexArraysPtr, "glDeleteVertexArrays");
    LOAD(glGenBuffersPtr, "glGenBuffers");
    LOAD(glBindBufferPtr, "glBindBuffer");
    LOAD(glBufferDataPtr, "glBufferData");
    LOAD(glDeleteBuffersPtr, "glDeleteBuffers");
    LOAD(glCreateShaderPtr, "glCreateShader");
    LOAD(glShaderSourcePtr, "glShaderSource");
    LOAD(glCompileShaderPtr, "glCompileShader");
    LOAD(glGetShaderivPtr, "glGetShaderiv");
    LOAD(glGetShaderInfoLogPtr, "glGetShaderInfoLog");
    LOAD(glDeleteShaderPtr, "glDeleteShader");
    LOAD(glCreateProgramPtr, "glCreateProgram");
    LOAD(glAttachShaderPtr, "glAttachShader");
    LOAD(glLinkProgramPtr, "glLinkProgram");
    LOAD(glGetProgramivPtr, "glGetProgramiv");
    LOAD(glGetProgramInfoLogPtr, "glGetProgramInfoLog");
    LOAD(glUseProgramPtr, "glUseProgram");
    LOAD(glDeleteProgramPtr, "glDeleteProgram");
    LOAD(glGetUniformLocationPtr, "glGetUniformLocation");
    LOAD(glUniformMatrix4fvPtr, "glUniformMatrix4fv");
    LOAD(glEnableVertexAttribArrayPtr, "glEnableVertexAttribArray");
    LOAD(glVertexAttribPointerPtr, "glVertexAttribPointer");
#undef LOAD
    return true;
}

bool Application::createShaderProgram() {
    std::string vert = readText("assets/shaders/basic.vert");
    std::string frag = readText("assets/shaders/basic.frag");
    if (vert.empty() || frag.empty()) return false;

    auto compile = [](unsigned int type, const std::string& source) -> unsigned int {
        unsigned int shader = glCreateShaderPtr(type);
        const char* src = source.c_str();
        glShaderSourcePtr(shader, 1, &src, nullptr);
        glCompileShaderPtr(shader);
        int ok = 0;
        glGetShaderivPtr(shader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048]{}; int len=0;
            glGetShaderInfoLogPtr(shader, sizeof(log), &len, log);
            OutputDebugStringA(log);
            glDeleteShaderPtr(shader);
            return 0;
        }
        return shader;
    };

    unsigned int vs = compile(GL_VERTEX_SHADER, vert);
    unsigned int fs = compile(GL_FRAGMENT_SHADER, frag);
    if (!vs || !fs) return false;

    program_ = glCreateProgramPtr();
    glAttachShaderPtr(program_, vs);
    glAttachShaderPtr(program_, fs);
    glLinkProgramPtr(program_);
    glDeleteShaderPtr(vs); glDeleteShaderPtr(fs);

    int ok = 0; glGetProgramivPtr(program_, GL_LINK_STATUS, &ok);
    if (!ok) return false;
    mvpLocation_ = glGetUniformLocationPtr(program_, "uMVP");
    return mvpLocation_ >= 0;
}

void Application::destroyOpenGL() {
    if (glrc_) {
        wglMakeCurrent(hdc_, glrc_);
        if (program_ && glDeleteProgramPtr) glDeleteProgramPtr(program_);
        if (vbo_ && glDeleteBuffersPtr) glDeleteBuffersPtr(1, &vbo_);
        if (vao_ && glDeleteVertexArraysPtr) glDeleteVertexArraysPtr(1, &vao_);
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(glrc_);
        glrc_ = nullptr;
    }
    if (tempContext_) { wglDeleteContext(tempContext_); tempContext_ = nullptr; }
    if (hdc_ && hwnd_) { ReleaseDC(hwnd_, hdc_); hdc_ = nullptr; }
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
}

void Application::update(float dt) {
    float speed = 3.0f * dt;
    float len = std::sqrt(cameraFront_.x*cameraFront_.x + cameraFront_.y*cameraFront_.y + cameraFront_.z*cameraFront_.z);
    Vec3 front{cameraFront_.x/len, cameraFront_.y/len, cameraFront_.z/len};
    Vec3 right{front.z, 0.0f, -front.x};
    if (keys_['W']) { cameraPos_.x += front.x*speed; cameraPos_.y += front.y*speed; cameraPos_.z += front.z*speed; }
    if (keys_['S']) { cameraPos_.x -= front.x*speed; cameraPos_.y -= front.y*speed; cameraPos_.z -= front.z*speed; }
    if (keys_['D']) { cameraPos_.x += right.x*speed; cameraPos_.z += right.z*speed; }
    if (keys_['A']) { cameraPos_.x -= right.x*speed; cameraPos_.z -= right.z*speed; }
}

void Application::processMouse() {
    POINT p; GetCursorPos(&p); ScreenToClient(hwnd_, &p);
    if (firstMouse_) { lastMouse_=p; firstMouse_=false; }
    float dx = static_cast<float>(p.x-lastMouse_.x) * 0.12f;
    float dy = static_cast<float>(lastMouse_.y-p.y) * 0.12f;
    lastMouse_=p;
    yaw_ += dx; pitch_ = std::clamp(pitch_+dy, -89.0f, 89.0f);
    const float rad = 3.14159265359f/180.0f;
    cameraFront_.x = std::cos(yaw_*rad)*std::cos(pitch_*rad);
    cameraFront_.y = std::sin(pitch_*rad);
    cameraFront_.z = std::sin(yaw_*rad)*std::cos(pitch_*rad);
}

void Application::render(float) {
    glViewport(0,0,width_,height_);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f,0.10f,0.15f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Mat4 model = identity();
    Mat4 view = lookAt(cameraPos_, Vec3{cameraPos_.x+cameraFront_.x,cameraPos_.y+cameraFront_.y,cameraPos_.z+cameraFront_.z}, cameraUp_);
    Mat4 proj = perspective(70.0f, static_cast<float>(width_)/static_cast<float>(height_), 0.1f, 100.0f);
    Mat4 mvp = multiply(proj, multiply(view, model));
    glUseProgramPtr(program_);
    glUniformMatrix4fvPtr(mvpLocation_, 1, GL_FALSE, mvp.m);
    glBindVertexArrayPtr(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    SwapBuffers(hdc_);
}

void Application::handleKeyDown(WPARAM key) { if (key < 256) keys_[key] = true; if (key == VK_ESCAPE) running_ = false; }
void Application::handleKeyUp(WPARAM key) { if (key < 256) keys_[key] = false; }

LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* app = reinterpret_cast<Application*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
        case WM_KEYDOWN: if (app) app->handleKeyDown(wParam); return 0;
        case WM_KEYUP: if (app) app->handleKeyUp(wParam); return 0;
        case WM_MOUSEMOVE: if (app) app->processMouse(); return 0;
        case WM_SIZE: if (app) { app->width_ = LOWORD(lParam); app->height_ = HIWORD(lParam); if (app->height_ == 0) app->height_=1; } return 0;
        case WM_CLOSE: if (app) app->running_=false; return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
        default: return DefWindowProcW(hwnd,msg,wParam,lParam);
    }
}

int Application::run() {
    if (!createWindow() || !createOpenGLContext() || !loadOpenGLFunctions() || !createShaderProgram()) {
        MessageBoxW(hwnd_, L"Duma3D konnte OpenGL nicht initialisieren. Siehe Debug-Ausgabe.", L"Duma3D", MB_ICONERROR);
        return 1;
    }

    const float vertices[] = {
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,0.5f,-0.5f,
         0.5f,0.5f,-0.5f, -0.5f,0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f,0.5f,  0.5f,-0.5f,0.5f,  0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f, -0.5f,0.5f,0.5f, -0.5f,-0.5f,0.5f,
        -0.5f,0.5f,0.5f, -0.5f,0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f, -0.5f,-0.5f,0.5f, -0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f,  0.5f,0.5f,-0.5f,  0.5f,-0.5f,-0.5f,
         0.5f,-0.5f,-0.5f,  0.5f,-0.5f,0.5f,  0.5f,0.5f,0.5f,
        -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f,0.5f,
         0.5f,-0.5f,0.5f, -0.5f,-0.5f,0.5f, -0.5f,-0.5f,-0.5f,
        -0.5f,0.5f,-0.5f,  0.5f,0.5f,-0.5f,  0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f, -0.5f,0.5f,0.5f, -0.5f,0.5f,-0.5f
    };

    glGenVertexArraysPtr(1,&vao_); glBindVertexArrayPtr(vao_);
    glGenBuffersPtr(1,&vbo_); glBindBufferPtr(GL_ARRAY_BUFFER,vbo_);
    glBufferDataPtr(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);
    glVertexAttribPointerPtr(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),nullptr);
    glEnableVertexAttribArrayPtr(0);

    ShowCursor(FALSE);
    SetCapture(hwnd_);
    auto previous = std::chrono::steady_clock::now();
    MSG msg{};
    while (running_) {
        while (PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) running_=false;
        }
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now-previous).count();
        previous=now;
        dt=std::min(dt,0.05f);
        update(dt); render(dt);
    }
    ReleaseCapture(); ShowCursor(TRUE);
    return 0;
}
