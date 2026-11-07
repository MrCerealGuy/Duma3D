# Duma3D

Duma3D is an experimental 3D engine for Windows. The project builds the engine as a reusable library and includes separate demos. `Demo_1` showcases procedural terrain and walkable houses; `Demo_2` is an enclosed Doom-inspired level built from connected rooms and corridors, with lighting contrast, interactive doors, a power switch, teleporters, machinery ambience, and synthesized sound effects.

See [BUILDING.md](BUILDING.md) for prerequisites and instructions to configure, build, and run the project from Visual Studio Code or the command line.

See [ENGINE_SPECIFICATIONS.md](ENGINE_SPECIFICATIONS.md) for the engine architecture, rendering and asset pipeline, world and chunk system, and the current demo capabilities.

Demo_2 levels are loaded at runtime using the [Duma3D Indoor Level Format](LEVEL_FORMAT.md).

## Controls

- The demo starts in walking mode, with the controls shown in the upper-left corner.
- **WASD:** Move and walk through open house entrances
- **Mouse:** Look around using relative raw mouse input
- **G:** Switch between flight and walking modes
- **Flight mode:** **Space** / **Ctrl** move up / down
- **Walking mode:** **Space** jumps; gravity, terrain, and house walls constrain movement
- **Shift:** Move faster
- **Esc:** Quit

## Demo_2 controls

- **WASD:** Walk
- **Mouse:** Look around
- **Shift:** Move faster
- **Space:** Jump
- **E:** Use the door, switch, or teleporter in front of you
- **Left mouse button:** Fire the starting pistol
- **R:** Reload the pistol
- **Esc:** Quit

## Repository structure

```text
Duma3D/
├── .gitattributes
├── .vscode/
│   ├── launch.json
│   └── tasks.json
├── assets/
│   ├── levels/
│   │   └── sector_09.d3l
│   ├── textures/
│   ├── models/
│   ├── audio/
│   └── shaders/
├── include/Engine/
│   ├── Assets/
│   ├── Graphics/
│   │   └── Scene/
│   ├── Math/
│   └── Game/
│       ├── Collision/
│       ├── Physics/
│       └── World/
├── src/
│   ├── Engine/
│   │   ├── Assets/
│   │   ├── Graphics/
│   │   │   └── Scene/
│   │   ├── Math/
│   │   ├── Game/
│   │   │   ├── FirstPersonController.cpp
│   │   │   ├── InteractionSystem.cpp
│   │   │   ├── Physics/
│   │   │   └── World/
│   └── Demos/
│       ├── Demo_1/
│       └── Demo_2/
├── BUILDING.md
├── ENGINE_SPECIFICATIONS.md
├── LEVEL_FORMAT.md
├── tools/
│   ├── GenerateDemoAudio.cpp
│   └── GenerateDemoTextures.cpp
├── CMakeLists.txt
├── Duma3D.code-workspace
└── README.md
```
