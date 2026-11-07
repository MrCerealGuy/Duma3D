#include "Application.hpp"

#include <filesystem>
#include <exception>
#include <string>
#include <windows.h>

int main(int argc, char** argv)
{
    const std::filesystem::path levelPath = argc > 1
        ? std::filesystem::path(argv[1])
        : std::filesystem::path("assets/levels/sector_09.d3l");
    try
    {
        Duma3D::Demos::Demo_2::Application application(1280, 720, L"Duma3D Demo 2", levelPath);
        return application.run();
    }
    catch (const std::exception& error)
    {
        const std::string message = std::string("Could not load the Demo_2 level:\n") + error.what();
        MessageBoxA(nullptr, message.c_str(), "Duma3D Demo 2", MB_ICONERROR | MB_OK);
        return 1;
    }
}
