# Build and run

## Requirements

- Windows with an OpenGL-capable graphics driver
- CMake 3.20 or later
- MinGW-w64 GCC (the project was configured and tested with Code::Blocks MinGW and GCC 14.2)
- Visual Studio Code with the **CMake Tools** (`ms-vscode.cmake-tools`) and **C/C++** (`ms-vscode.cpptools`) extensions, if using VS Code

The project has no external C++ library dependencies and CMake does not download dependencies during configuration.

## Visual Studio Code

Open `Duma3D.code-workspace` in VS Code. The workspace selects the `MinGW Makefiles` generator and points to the MinGW installation under `C:\Program Files\CodeBlocks\MinGW\bin`.

1. Run **CMake: Configure** once from the Command Palette (`Ctrl+Shift+P`).
2. In **Run and Debug**, select **Duma3D (MinGW/GDB)**.
3. Press **F5**. VS Code builds the `Duma3D` target and launches it with GDB.

The build task is defined in `.vscode/tasks.json`, and the debugger configuration is in `.vscode/launch.json`. To build manually inside VS Code, choose **Terminal → Run Build Task** and select **build Duma3D**.

The workspace configuration contains local paths for CMake, MinGW, and GDB. If these tools are installed elsewhere, update the paths in `Duma3D.code-workspace`, `.vscode/tasks.json`, and `.vscode/launch.json`.

## Command line

Run these commands from a MinGW terminal in the project directory:

```bat
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Then run `build\Duma3D.exe`. CMake copies the assets to `build\assets` beside the executable. The application resolves asset paths relative to the executable, even when launched from another working directory.

## Regenerate Demo 1 textures

The tileable procedural PPM textures can be regenerated with the included MinGW compiler:

```bat
"C:\Program Files\CodeBlocks\MinGW\bin\c++.exe" -std=c++20 -O2 -static -static-libgcc -static-libstdc++ tools\GenerateDemoTextures.cpp -o build\GenerateDemoTextures.exe
build\GenerateDemoTextures.exe assets\textures
```

Rebuild `Duma3D` afterward to copy the updated textures to `build\assets`.
