# Duma3D

Duma3D is an experimental 3D engine for Windows. The project builds the engine as a reusable library and includes `Demo_1`, a separate sample application showcasing rendering, textures, lighting, shadows, procedural worlds, and walkable interiors.

See [BUILDING.md](BUILDING.md) for prerequisites and instructions to configure, build, and run the project from Visual Studio Code or the command line.

See [ENGINE_SPECIFICATIONS.md](ENGINE_SPECIFICATIONS.md) for the engine architecture, rendering and asset pipeline, world and chunk system, and the current demo capabilities.

## Controls

- The demo starts in walking mode, with the controls shown in the upper-left corner.
- **WASD:** Move and walk through open house entrances
- **Mouse:** Look around using relative raw mouse input
- **G:** Switch between flight and walking modes
- **Flight mode:** **Space** / **Ctrl** move up / down
- **Walking mode:** **Space** jumps; gravity, terrain, and house walls constrain movement
- **Shift:** Move faster
- **Esc:** Quit

## Repository structure

```text
Duma3D/
├── .vscode/
│   ├── launch.json
│   └── tasks.json
├── assets/
│   ├── textures/
│   ├── models/
│   └── shaders/
├── include/Engine/
│   ├── Assets/
│   ├── Graphics/
│   ├── Math/
│   ├── Scene/
│   └── World/
├── src/
│   ├── Engine/
│   │   ├── Assets/
│   │   ├── Graphics/
│   │   ├── Math/
│   │   ├── Scene/
│   │   └── World/
│   └── Demos/
│       └── Demo_1/
├── BUILDING.md
├── ENGINE_SPECIFICATIONS.md
├── CMakeLists.txt
├── Duma3D.code-workspace
└── README.md
```
