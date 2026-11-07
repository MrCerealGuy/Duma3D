#include "Application.hpp"

#include <exception>
#include <string>

int main(int argc, char** argv)
{
    const std::filesystem::path modelPath = argc > 1
        ? std::filesystem::path(argv[1])
        : std::filesystem::path("assets/sketchfab/levels/map-78/source/78.glb");
    try
    {
        Duma3D::Demos::Demo_3::Application application(modelPath);
        return application.run();
    }
    catch (const std::exception& error)
    {
        const std::string message = std::string("Could not load the GLB level:\n") + error.what();
        MessageBoxA(nullptr, message.c_str(), "Duma3D Demo 3", MB_ICONERROR | MB_OK);
        return 1;
    }
}
