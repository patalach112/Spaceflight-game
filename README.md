# AEGIS: Spaceflight Simulator

A clean-room C++20 / Raylib vehicle assembly milestone. The project starts at the main menu and includes settings, a vector-rendered VAB, and an explicitly non-simulating flight transition. It does not use assets or code from Spaceflight Simulator and is not affiliated with its creators. This milestone is a foundation, not a complete or physics-accurate 1:1 recreation.

## Project layout

```text
CMakeLists.txt
include/
  GameState.hpp      State machine and runtime settings
  Game.hpp           Application and scene ownership
  UIComponents.hpp   Vector UI components and theme
  Renderer.hpp       Camera, workspace and part rendering
  PartCatalog.hpp    Part definitions, attachment nodes and instances
  VABSystem.hpp      Placement, collision, symmetry and telemetry
src/
  main.cpp           Entry point
  Game.cpp           Scene flow, input and interaction
  UIComponents.cpp
  Renderer.cpp
  PartCatalog.cpp
  VABSystem.cpp
tests/
  VABSystemTests.cpp Headless construction regression tests
```

## Windows build and run (PowerShell)

Install CMake 3.24 or newer, Git, and either Visual Studio 2022 with **Desktop development with C++** or a MinGW-w64 toolchain with a C++20-capable GCC and `mingw32-make` on `PATH`. The first configure needs internet access to download Raylib 5.5 unless a compatible Raylib CMake package is already installed. Run from the project root:

### Visual Studio 2022 / MSVC

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\aegis.exe
```

### MinGW-w64 / GCC

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-mingw --parallel
ctest --test-dir build-mingw --output-on-failure
.\build-mingw\aegis.exe
```

Use a fresh build directory when changing generators or compilers. CMake first looks for an installed Raylib 5.5 package; otherwise it fetches the tagged source and builds it. To run just the renderer-independent tests without fetching Raylib, configure with `-DAEGIS_BUILD_GAME=OFF`.

## Controls

- Main menu: **Launch / VAB**, **Settings**, or **Quit**. Settings apply immediately; VSync behavior depends on the platform/driver. The audio setting controls Raylib's master volume; this milestone has no audio assets.
- Choose a category and drag its part tile into the workspace, or click a tile and then click in the workspace. Release near a free, compatible node or the floor to place; cyan is valid and amber indicates a blocked snap. Red ghosts cannot be placed.
- Toggle **Symmetry** before placing a palette part to preview and place a reflected pair atomically. A centered part is placed once. Both copies must have separate support and clear space. Symmetry does not automatically pair or move previously placed parts.
- Click a placed part to select it, drag it to move it, and press **Delete** or **Backspace** to remove the selection. With no selection, Delete removes the part under the cursor. A failed move restores the original. Placing or removing parts does not enforce structural connectivity of the rest of the craft.
- Drag with middle or right mouse to pan; use the wheel for smooth zoom centered on the cursor. The orange line is the symmetry axis; the horizontal line is the floor. **Esc** leaves the VAB for the main menu. **Flight** opens the transition scene; Esc returns to the VAB.

## Model and boundaries

One grid unit is 0.5 m, rendered as 20 pixels at 1× zoom (40 px/m). Integer-grid axis-aligned parts magnetically snap within 1.5 grid units of the continuous cursor-derived position to unoccupied, opposite-facing part nodes, or rest on a floor anchor. Placement rejects AABB overlaps and below-floor geometry. Parts have fixed orientation in this milestone. The palette contains a command pod, fuel tank, liquid engine and decoupler. Catalog mass and fuel capacity are in tonnes and thrust is in kN; telemetry assumes tanks are full, sums static thrust, and computes TWR at 9.80665 m/s². These are representative values, not SFS data. There is no flight physics, staging, rotation, save/load, persistence, or audio content yet.
