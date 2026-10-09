# Duma3D Engine Specifications

This document describes the current architecture and capabilities of Duma3D. The implementation is experimental and continues to evolve.

## Architecture and platform

- Windows application and input handling use Win32.
- OpenGL contexts are created with WGL. VSync is enabled through WGL when the graphics driver supports the extension.
- The engine contains a small OpenGL function loader, minimal custom vector and matrix math, indexed meshes, and GLSL shaders under `assets/shaders`.
- `Engine::Physics::CharacterController` provides kinematic first-person movement with gravity, jumping, terrain-height sampling, and scene-box collision. A demo supplies its ground-height function and translates input into movement distances.
- The reusable engine is built as the static library `Duma3DEngine`. Demo scenes, input handling, and entry points are kept separately under `src/Demos/<DemoName>`. `Demo_1` is the current sample application.
- Asset paths are resolved relative to the executable directory.
- There are no external C++ library dependencies or downloads during CMake configuration. GLAD and GLM are not used. MinGW builds link their runtime libraries statically.

## World and chunk system

`Engine::World::ChunkWorld` manages a square loaded area of chunks, their coordinates, and a world seed. It caches generated chunk scenes, unloads chunks outside the configured radius, and combines the remaining local scenes for rendering. Content is supplied through a generator callback, allowing different demos to define their own procedural worlds.

Demo 1 generates terrain in 64-by-64-meter chunks using a seamless world-space height function. It keeps a 5-by-5 chunk area around the player loaded. Chunk terrain, vegetation, and optional houses are determined by world seed and chunk coordinate, so revisiting a chunk reproduces its content. A new seed is generated at each application launch. Demo 1 supplies the height sampler used by the engine character controller, so the same terrain function determines both rendered ground and player elevation.

The terrain uses spatial noise fields and randomized patches to distribute grass, soil, rock, and cobblestone surfaces. Combined procedural meshes add grass tufts, stone clusters, dry branches, and fallen leaves while leaving the spawn point, houses, and entrances mostly clear.

## Demo 1 scene

The nearby area contains walkable houses with front and rear rooms, paths, and trees. Each house has an interior partition parallel to its entrance wall and a doorway connecting the rooms. House dimensions, room and doorway layouts, windows, roof styles, colors, room lights, and tree placement vary procedurally.

Procedural base meshes include a cube and a UV sphere with smooth normals. Tiled PPM textures provide visible surface detail for terrain, plaster, roofs, wood floors and trim, paths, bark, and foliage.

## Rendering and lighting

The renderer, mesh data, and scene objects are organized into separate engine modules. OBJ vertices are deduplicated by their attributes. GPU index buffers use 16-bit indices when possible and 32-bit indices otherwise. The main and shadow passes cull objects whose mesh bounding spheres fall outside the camera or light frustum; the light frustum is computed from nearby scene objects.

GPU meshes are cached by their vertex, index, and material-section data across scene updates. Unchanged chunk geometry and shared base meshes reuse existing vertex and index buffers; meshes no longer used are released after the new scene is uploaded. GPU textures are cached by resolved asset path in the same way.

Lighting combines ambient light, a directional light with 3×3 percentage-closer filtering (PCF) shadows, up to four attenuated point lights, Blinn–Phong specular highlights, and emissive color. Color textures are decoded as sRGB. Lighting and Reinhard tone mapping run in linear space before the output is encoded back to sRGB.

## Model and texture support

The Wavefront importer reads positions, normals, UV texture coordinates, faces, and smoothing groups (`s`). It triangulates polygon faces and calculates missing normals per face or smoothing group. MTL diffuse colors (`Kd`), specular colors (`Ks`), emissive colors (`Ke`), shininess (`Ns`), and basic `map_Kd` paths are applied per material section.

Diffuse textures use generated mipmaps and trilinear filtering. Identical texture paths share GPU textures. The texture loader supports P3 and P6 PPM, PNG, JPEG, and BMP through Windows Imaging Component. Extended MTL map options are not yet supported.
