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

### MSYS2 UCRT64 / MinGW-w64 / Ninja

In an MSYS2 UCRT64 shell with the UCRT64 GCC, CMake and Ninja packages installed:

```bash
cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-ninja --parallel
ctest --test-dir build-ninja --output-on-failure
./build-ninja/bin/Release/aegis_game.exe
```

Use a fresh directory when changing generators or compilers. Subsequent builds reuse Raylib and unchanged object files; actual incremental build time depends on the toolchain and which files changed, so sub-second builds are not guaranteed. For headless tests without downloading Raylib, configure with `-DAEGIS_BUILD_GAME=OFF`.

## Controls

- In the main menu choose **Launch / VAB**, **Settings**, or **Quit**. Settings apply immediately; VSync behavior depends on the platform/driver. Audio volume controls Raylib's master volume; this milestone has no audio assets. UI scale is constrained on small windows to keep controls visible.
- Press and hold a part tile, drag into the workspace, and release to place it on a clear grid location or magnetically attach it near a free node. Releasing over the sidebar, outside the workspace, or at a blocked location discards the palette ghost. The catalog scrolls with the mouse wheel.
- **Right-click or Esc** cancels a ghost or restores a part being moved. When no ghost is active, Esc returns to the main menu. Right-click also pans when no ghost is active; middle-mouse pans at any time. The wheel zooms smoothly around the cursor.
- Toggle **Symmetry** to preview and place a reflected pair atomically. A centered part is placed once; both copies must be clear. Symmetry does not automatically pair or move existing parts.
- Drag a placed part to move it, or click it without dragging to select it. Failed or cancelled moves restore the original. **Delete** or **Backspace** removes the selection, or the part under the cursor if nothing is selected. Moving or removing parts does not enforce connectivity of the rest of the craft.
- Use the **ROT** buttons on a selection, or **Q / E** on a selection or while dragging, to rotate by 90 degrees. Rotation respects grid collisions, including across the horizontal reference line. **CLEAR** removes the entire build; **MASS** toggles an approximate center-of-mass marker based on each part's bounding-box center.
- **Flight** opens the transition scene; Esc returns to the VAB.

## Model and boundaries

One grid unit is 0.5 m, rendered as 20 pixels at 1× zoom (40 px/m). Parts can be placed freely on the integer grid on either side of, or across, the horizontal reference line. The line is not a floor and has no snap or occupied nodes of its own. Magnetic snapping works within 1.5 grid units of an unoccupied, opposite-facing part node. Placement rejects AABB overlaps. Parts rotate in 90-degree steps; symmetry reflects both the orientation and the shape/attachment nodes of radial parts.

The catalog includes a command pod and probe, four tank sizes, five engines, and structural, aerodynamic and utility parts. Their dimensions and some utility masses are approximations for this project's grid; they are not a complete or exact reproduction of every SFS variant. The tank values follow the four-wide 5/10/15/20 t full-mass progression, with 90% fuel. Main-engine masses and thrust are based on the SFS community's easy/hard-mode figures ([engines](https://spaceflight-simulator.fandom.com/wiki/Engines), [tanks](https://spaceflight-simulator.fandom.com/wiki/Fuel_Tanks), [RCS](https://spaceflight-simulator.fandom.com/wiki/RCS_Thrusters)); tonne-force is converted to kN at 9.80665 kN per tonne-force. The values are illustrative, not a claim of official or version-independent accuracy.

Telemetry assumes full tanks, sums dry and fuel mass in tonnes and **main-engine** static thrust in kN, and computes TWR at 9.80665 m/s². The multidirectional RCS rating is shown in the catalog but excluded from main-engine thrust/TWR. Thrust is summed by magnitude regardless of orientation, so the TWR is an estimate. Parts are currently construction assets only: separation, parachutes, docking, landing legs, RCS, and fuel consumption do not operate in flight. There is no flight physics, staging, save/load, persistence, or audio content yet.
