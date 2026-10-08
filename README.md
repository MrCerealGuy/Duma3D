# Duma3D

Duma3D ist ein experimenteller 3D-Engine-Starter für Windows. Der aktuelle Stand öffnet ein Win32-Fenster, erstellt über WGL einen OpenGL-3.3-Core-Kontext und rendert eine einfache Szene mit einer prozeduralen Bodenebene, Würfeln und UV-Kugel sowie einem texturierten OBJ-Modell aus frei beweglicher Kamera.

## Technischer Pfad

- Win32 für Fenster und Eingaben
- WGL für den OpenGL-Kontext
- ein kleiner, direkt in der Engine implementierter OpenGL-Funktionslader
- VSync über WGL, wenn der Grafiktreiber die Erweiterung unterstützt
- eigene minimale Vektor- und Matrixfunktionen
- indizierte Meshes mit Vertex- und Indexpuffern
- GLSL-Shader unter `assets/shaders`
- Assetpfade werden relativ zum Verzeichnis der EXE aufgelöst
- keine externen C++-Bibliotheken und keine Downloads beim CMake-Konfigurieren

GLAD und GLM werden nicht verwendet. Der MinGW-Build bindet die MinGW-Laufzeitbibliotheken statisch ein, damit die erzeugte EXE keine separaten MinGW-DLLs benötigt.

## Voraussetzungen

- Windows mit OpenGL-Treiber
- CMake 3.20 oder neuer
- MinGW-w64 GCC; eingerichtet und verwendet wurde Code::Blocks MinGW mit GCC 14.2
- Visual Studio Code mit den Erweiterungen **CMake Tools** (`ms-vscode.cmake-tools`) und **C/C++** (`ms-vscode.cpptools`)

## In Visual Studio Code bauen und starten

Öffne `Duma3D.code-workspace` in VS Code. Die Workspace-Einstellungen wählen den Generator `MinGW Makefiles` und verweisen auf die MinGW-Installation unter `C:\Program Files\CodeBlocks\MinGW\bin`.

1. Führe einmal **CMake: Configure** über die Befehlspalette (`Ctrl+Shift+P`) aus.
2. Wähle in **Run and Debug** die Konfiguration **Duma3D (MinGW/GDB)**.
3. Drücke **F5**. VS Code baut `Duma3D` vor dem Start und startet die Anwendung mit GDB.

Der Build-Schritt ist in `.vscode/tasks.json` definiert; die Debugkonfiguration liegt in `.vscode/launch.json`. Zum manuellen Bauen kannst du in VS Code **Terminal → Run Build Task** und **build Duma3D** wählen.

Die Konfigurationsdateien enthalten lokale Pfade für CMake, MinGW und GDB. Wenn diese Programme an anderen Orten installiert sind, passe die Pfade in `Duma3D.code-workspace`, `.vscode/tasks.json` und `.vscode/launch.json` an.

## Manuell über die Konsole bauen

In einer MinGW-Konsole im Projektverzeichnis:

```bat
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

Danach `build\Duma3D.exe` ausführen. Die Assets werden in `build\assets` neben die EXE kopiert. Das Programm findet sie auch dann, wenn es aus einem anderen Arbeitsverzeichnis gestartet wird.

## Steuerung

- Die Demo startet im Laufmodus; die Tastenlegende bleibt oben links eingeblendet.
- WASD: Kamera bewegen und durch die offenen Hauseingänge gehen
- Maus: Kamera drehen (relative Raw-Input-Bewegung)
- G: zwischen Flug- und Laufmodus umschalten
- Flugmodus: Leertaste / Strg bewegen die Kamera nach oben / unten
- Laufmodus: Leertaste springen; Schwerkraft, Gelände und Hauswände begrenzen die Bewegung
- Umschalt: schneller bewegen
- ESC: beenden

## Aktueller Umfang

Der Renderer, die Meshdaten und die Szenenobjekte liegen in eigenen Engine-Modulen. OBJ-Vertices werden anhand ihrer Attribute dedupliziert; GPU-Indexpuffer verwenden 16 Bit, wenn alle Indizes passen, sonst 32 Bit. Haupt- und Schattenpass überspringen Objekte, deren Mesh-Begrenzungskugel außerhalb des jeweiligen Kamera- oder Lichtfrustums liegt; das Lichtfrustum wird aus den Szenenobjekten berechnet. Die Demo-Szene wird bei jedem Programmstart prozedural aufgebaut und variiert Hausmaße, Tür- und Raumaufteilung, Fenster, Giebel- oder Flachdächer, Farben, Raumlichter und Baumstandorte. Sie enthält hügeliges Gelände, zwei begehbare Häuser mit jeweils einem vorderen und hinteren Raum sowie Wege und Bäume. Die Innenwand verläuft parallel zur Eingangswand; die Türöffnung verbindet beide Räume. Als prozedurale Grundmeshes gibt es einen Würfel und eine UV-Kugel mit glatten Normalen. Die Beleuchtung kombiniert Umgebungslicht, ein gerichtetes Licht mit 3×3-PCF-Schatten, bis zu vier abschwächende Punktlichter, Blinn-Phong-Glanz und Selbstleuchten. Farbtexturen werden als sRGB dekodiert; Beleuchtung und Reinhard-Tonemapping erfolgen linear, danach wird die Ausgabe wieder sRGB-kodiert. Der Wavefront-Importer liest Positionen, Normalen, UV-Texturkoordinaten, Flächen und Smoothing-Gruppen (`s`), trianguliert Polygonflächen und berechnet fehlende Normalen flach oder gruppenweise. MTL-Diffusfarben (`Kd`), Glanzfarben (`Ks`), Selbstleuchtfarben (`Ke`), Glanzschärfe (`Ns`) und einfache `map_Kd`-Pfade werden pro Materialabschnitt übernommen; diffuse Texturen werden mit generierten Mipmaps und trilinearer Filterung dargestellt und bei identischen Pfaden gemeinsam auf der GPU gehalten. Der Texturloader unterstützt P3- und P6-PPM sowie PNG, JPEG und BMP über Windows Imaging Component; erweiterte MTL-Map-Optionen werden noch nicht unterstützt. Die Win32-Plattforminitialisierung bleibt in `src/Engine/Application.cpp`.

## Struktur

```text
Duma3D/
├── .vscode/
│   ├── launch.json
│   └── tasks.json
├── assets/
│   ├── models/
│   │   ├── pyramid.mtl
│   │   ├── pyramid.obj
│   │   ├── pyramid.ppm
│   │   └── pyramid-top.ppm
│   └── shaders/
│       ├── basic.vert
│       ├── basic.frag
│       ├── shadow.vert
│       └── shadow.frag
├── include/
│   └── Engine/
│       ├── Assets/
│       │   └── AssetPath.hpp
│       ├── Application.hpp
│       ├── Graphics/
│       │   ├── Camera.hpp
│       │   └── Renderer.hpp
│       ├── Math/
│       │   └── Math.hpp
│       └── Scene/
│           ├── Mesh.hpp
│           └── Scene.hpp
├── src/
│   ├── main.cpp
│   └── Engine/
│       ├── Assets/
│       │   └── AssetPath.cpp
│       ├── Application.cpp
│       ├── Graphics/
│       │   ├── Camera.cpp
│       │   └── Renderer.cpp
│       ├── Math/
│       │   └── Math.cpp
│       └── Scene/
│           ├── Mesh.cpp
│           └── Scene.cpp
├── CMakeLists.txt
├── Duma3D.code-workspace
└── README.md
```
