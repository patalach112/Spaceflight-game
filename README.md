# AEGIS: Spaceflight Simulator

A clean-room C++20 / Raylib vehicle assembly milestone with a deep-blue aerospace UI. AEGIS is not affiliated with Spaceflight Simulator and is not yet a complete or physics-accurate recreation. The flight scene remains a transition, not a simulation.

## Project layout

```text
CMakeLists.txt
assets/              DejaVu Sans font and license
cmake/               Runtime dependency copy script
include/             Engine API, state, rendering and construction headers
src/                 Engine implementation and lightweight main.cpp
tests/              Headless construction and interaction tests
```

`aegis_engine` is a shared library containing the construction model, interaction controller and (when enabled) application/rendering code. `aegis_game` is a small executable calling the exported entry point. The `AEGIS_API` macro exports/imports the public engine interface. Build outputs copy the engine and known runtime DLLs next to each executable; the game also receives `assets/DejaVuSans.ttf` next to it. Keep that assets directory with the executable when distributing it. The bundled DejaVu Sans license is in `assets/DejaVu-LICENSE.txt`.

## Windows build and run (PowerShell)

Install CMake 3.24 or newer, Git, and either Visual Studio 2022 with **Desktop development with C++**, or MinGW-w64 with a C++20-capable GCC and `mingw32-make` on `PATH`. The first configure downloads Raylib 5.5 unless a compatible Raylib CMake package is already installed. Run from the project root:

### Visual Studio 2022 / MSVC

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
.\build\bin\Release\aegis_game.exe
```

### MinGW-w64 / GCC

```powershell
cmake -S . -B build-mingw -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build-mingw --parallel
ctest --test-dir build-mingw --output-on-failure
.\build-mingw\bin\Release\aegis_game.exe
```

Use a fresh directory when changing generators or compilers. Subsequent builds reuse Raylib and unchanged object files; actual incremental build time depends on the toolchain and which files changed, so sub-second builds are not guaranteed. For headless tests without downloading Raylib, configure with `-DAEGIS_BUILD_GAME=OFF`.

## Controls

- In the main menu choose **Launch / VAB**, **Settings**, or **Quit**. Settings apply immediately; VSync behavior depends on the platform/driver. Audio volume controls Raylib's master volume; this milestone has no audio assets. UI scale is constrained on small windows to keep controls visible.
- Click a category and click its part tile to pick up a ghost. Move the cursor into the workspace and **left-click again** to place it on any clear grid location above the floor, or near a compatible node for magnetic snapping. A successful placement releases the cursor; a blocked location stays red and leaves the ghost active for another attempt.
- **Right-click or Esc** cancels a ghost or restores a part being moved. When no ghost is active, Esc returns to the main menu. Right-click also pans when no ghost is active; middle-mouse pans at any time. The wheel zooms smoothly around the cursor.
- Toggle **Symmetry** to preview and place a reflected pair atomically. A centered part is placed once; both copies must be clear. Symmetry does not automatically pair or move existing parts.
- Click a placed part to select it, drag it to move it, and press **Delete** or **Backspace** to remove the selection. With no selection, Delete removes the part under the cursor. A failed move restores the original. Moving or removing parts does not enforce connectivity of the rest of the craft.
- **Flight** opens the transition scene; Esc returns to the VAB.

## Model and boundaries

One grid unit is 0.5 m, rendered as 20 pixels at 1× zoom (40 px/m). Axis-aligned parts can be placed freely on the integer grid, with magnetic snapping within 1.5 grid units to an unoccupied, opposite-facing part node or the floor. Placement rejects AABB overlaps and below-floor geometry. Parts have fixed orientation. Telemetry assumes full tanks, sums dry and fuel mass in tonnes and static thrust in kN, and computes TWR at 9.80665 m/s². These are representative values, not SFS data. There is no flight physics, staging, rotation, save/load, persistence, or audio content yet.
