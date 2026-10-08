# Duma3D

Duma3D ist eine experimentelle 3D-Engine für Windows. Das Projekt baut die Engine als wiederverwendbare Bibliothek und enthält mit `Demo_1` ein separates Beispielprogramm, das Rendering, Texturen, Beleuchtung, Schatten, prozedurale Szenen und begehbare Räume vorführt.

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

Die Engine wird als wiederverwendbare statische Bibliothek `Duma3DEngine` gebaut. Demo-Szenen, Eingabesteuerung und Einstiegspunkte liegen unabhängig davon unter `src/Demos/<DemoName>`; `Demo_1` ist das aktuelle Beispielprogramm und wird gegen die Engine-Bibliothek gelinkt.

Das Terrain verwendet pro Programmstart eine zufällig erzeugte Verteilung aus organischen Gras-, Erd-, Fels- und Pflasterflächen. Mehrere räumliche Rauschfelder und zufällige Materialflecken variieren Größe und Form der Geländeoberflächen.

Zusammengefasste prozedurale Meshes ergänzen die Landschaft mit zufällig platzierten Grasbüscheln, Steinclustern, trockenen Ästen und herabgefallenen Herbstblättern. Spielerstart, Häuser und Eingangswege bleiben dabei weitgehend frei.

Der Renderer, die Meshdaten und die Szenenobjekte liegen in eigenen Engine-Modulen. OBJ-Vertices werden anhand ihrer Attribute dedupliziert; GPU-Indexpuffer verwenden 16 Bit, wenn alle Indizes passen, sonst 32 Bit. Haupt- und Schattenpass überspringen Objekte, deren Mesh-Begrenzungskugel außerhalb des jeweiligen Kamera- oder Lichtfrustums liegt; das Lichtfrustum wird aus den Szenenobjekten berechnet. Die Demo-Szene wird bei jedem Programmstart prozedural aufgebaut und variiert Hausmaße, Tür- und Raumaufteilung, Fenster, Giebel- oder Flachdächer, Farben, Raumlichter und Baumstandorte. Kachelbare PPM-Oberflächentexturen geben Gelände, Putzfassaden, Dächer, Holzböden und -details, Steinwege, Rinde und Baumkronen sichtbare Struktur. Die Szene enthält hügeliges Gelände, zwei begehbare Häuser mit jeweils einem vorderen und hinteren Raum sowie Wege und Bäume. Die Innenwand verläuft parallel zur Eingangswand; die Türöffnung verbindet beide Räume. Als prozedurale Grundmeshes gibt es einen Würfel und eine UV-Kugel mit glatten Normalen. Die Beleuchtung kombiniert Umgebungslicht, ein gerichtetes Licht mit 3×3-PCF-Schatten, bis zu vier abschwächende Punktlichter, Blinn-Phong-Glanz und Selbstleuchten. Farbtexturen werden als sRGB dekodiert; Beleuchtung und Reinhard-Tonemapping erfolgen linear, danach wird die Ausgabe wieder sRGB-kodiert. Der Wavefront-Importer liest Positionen, Normalen, UV-Texturkoordinaten, Flächen und Smoothing-Gruppen (`s`), trianguliert Polygonflächen und berechnet fehlende Normalen flach oder gruppenweise. MTL-Diffusfarben (`Kd`), Glanzfarben (`Ks`), Selbstleuchtfarben (`Ke`), Glanzschärfe (`Ns`) und einfache `map_Kd`-Pfade werden pro Materialabschnitt übernommen; diffuse Texturen werden mit generierten Mipmaps und trilinearer Filterung dargestellt und bei identischen Pfaden gemeinsam auf der GPU gehalten. Der Texturloader unterstützt P3- und P6-PPM sowie PNG, JPEG und BMP über Windows Imaging Component; erweiterte MTL-Map-Optionen werden noch nicht unterstützt. Die Win32-Anwendung und die konkrete Demo-Szene liegen unter `src/Demos/Demo_1`.

## Struktur

```text
Duma3D/
├── .vscode/
│   ├── launch.json
│   └── tasks.json
├── assets/
│   ├── textures/
│   │   ├── grass.ppm
│   │   ├── plaster_*.ppm
│   │   ├── roof_*.ppm
│   │   ├── wood_floor.ppm
│   │   ├── dark_wood.ppm
│   │   ├── cobblestone.ppm
│   │   ├── bark.ppm
│   │   ├── soil.ppm
│   │   ├── rock.ppm
│   │   ├── autumn_leaves.ppm
│   │   └── foliage.ppm
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
│       ├── Graphics/
│       │   ├── Camera.hpp
│       │   └── Renderer.hpp
│       ├── Math/
│       │   └── Math.hpp
│       └── Scene/
│           ├── Mesh.hpp
│           └── Scene.hpp
├── src/
│   └── Engine/
│       ├── Assets/
│       │   └── AssetPath.cpp
│       ├── Graphics/
│       │   ├── Camera.cpp
│       │   └── Renderer.cpp
│       ├── Math/
│       │   └── Math.cpp
│       └── Scene/
│           ├── Mesh.cpp
│           └── Scene.cpp
│   └── Demos/
│       └── Demo_1/
│           ├── Application.hpp
│           ├── Application.cpp
│           ├── DemoScene.hpp
│           ├── DemoScene.cpp
│           └── main.cpp
├── CMakeLists.txt
├── Duma3D.code-workspace
└── README.md
```
