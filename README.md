# 3D Solar System Simulation

An interactive Solar System simulation and space sandbox built with **C++20** and **OpenGL 3.3 Core Profile**. Explore the planets, follow their motion, change their physical properties, launch projectiles, and experiment with collisions and fictional space scenarios.

The project combines real-time rendering, gravitational simulation, a custom interface, procedural sound, and interactive visual effects.

## Features

### Solar System and Rendering

- Textured Sun, all eight planets, and Earth's Moon.
- Planet rotation, Saturn's rings, a star background, an asteroid belt, and shooting stars.
- Reusable procedural sphere and ring meshes.
- Custom GLSL shaders with ambient and diffuse lighting.
- Atmospheric effects, procedural Earth clouds, and impact marks with fading glow.
- Orbital trails and a trajectory preview for slingshot launches.
- Adjustable lighting for observing planetary surfaces.
- On-screen FPS counter that turns red when the frame rate drops below 30 FPS.
- Built-in benchmark (`F9`) that compares FPS with the craters-and-clouds technique ON and OFF.

### Physics and Interaction

- Pairwise gravitational attraction between active simulation bodies.
- Velocity Verlet integration with physics substeps.
- Collision detection, damage, debris, explosions, and black-hole absorption.
- Editable mass and size for the selected body.
- Pause, adjustable simulation speed, and scene reset.
- Free camera, planet focus, orbit controls, an automatic tour, and cinematic event views.

### Sandbox Tools

- Five selectable weapons: **Nuke, Missiles, Asteroid, Laser, and Planet Killer**.
- Launchable projectiles, rockets, and black holes.
- A battle station that can attack a selected planet.
- Four preset scenarios: Earth–Mars collision, Jupiter ignition, solar supernova, and a rogue black hole.
- Three save slots for the Sun and planets.
- Planet information panels, health and population indicators, help, and notifications.
- Procedural sound effects, ambient audio, and a hidden video Easter egg.

> This is an educational graphics project and a creative sandbox. Sizes, distances, time, and physical behavior are simplified. Destruction, star ignition, and supernova scenarios are fictional demonstrations, not scientific predictions.

## Technologies

| Technology | Purpose |
| --- | --- |
| C++20 | Application logic and simulation |
| OpenGL 3.3 / GLSL | Rendering and shader effects |
| GLFW | Window creation and keyboard/mouse input |
| GLAD | OpenGL function loading |
| GLM | Vector, matrix, and transformation mathematics |
| stb_image | Texture loading |
| stb_easy_font | Interface text |
| miniaudio | Audio output and procedural sound |
| pl_mpeg | MPEG video playback for the Easter egg |
| Visual Studio / MSVC | Included Windows project configuration |

Dependency headers and a Windows GLFW static library are included in the repository.

## Getting Started

### VS Code: macOS and Windows (CMake)

Open the whole repository folder in VS Code, not just `main.cpp`.
Use **Terminal → Run Task → Run Solar System**. This configures CMake,
builds all source files, and starts the app with the correct asset directory.
**Terminal → Run Build Task** builds without starting the app.

Prerequisites:

- macOS: Xcode Command Line Tools (`xcode-select --install`) and CMake
  (`brew install cmake`). Installing GLFW (`brew install glfw`) avoids downloading it at configure time.
  The VS Code tasks include Homebrew paths for both Apple Silicon and Intel Macs.
- Windows: CMake and Git on PATH, and Visual Studio 2022 or newer Build Tools
  with **Desktop development with C++** and a Windows SDK. VS Code is the editor;
  the Build Tools provide the compiler. Use a graphics driver supporting OpenGL 3.3.

If GLFW is not installed, CMake downloads GLFW 3.4 from its official repository
and compiles it for the current platform. Internet access is needed for that first build.
The bundled Windows `.lib` is not used by this CMake build.

The same terminal commands work on macOS and Windows:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel 4
cmake --build build --config Debug --target run
```

The executable is in `build/bin/`. Start it through the `run` target or with
the repository root as the working directory. Do not copy `build/` between
operating systems; build separately on each machine.

Local validation: compiled and launched on Apple Silicon macOS. Windows
configuration is provided but has not been executed on a Windows machine.

### Requirements

- Windows with Visual Studio and the **Desktop development with C++** workload.
- A Windows SDK and a C++20-capable MSVC toolset.
- A graphics driver supporting **OpenGL 3.3 Core Profile** or newer.
- Git, or a downloaded ZIP of the repository.

The included project requests platform toolset **v145**. If it is unavailable, select a compatible installed toolset in the project properties.

### 1. Download the Project

```bash
git clone https://github.com/fvdgufdug/final-project.git
cd final-project
```

Alternatively, choose **Code → Download ZIP** on GitHub and extract the archive.

### 2. Open and Configure Visual Studio

Open `helloWorldGLFW.vcxproj` from the repository root. A solution file is not included; Visual Studio can open the project directly.

**The checked-in project needs configuration before its first build:** it contains machine-specific include paths and currently lists only `main.cpp` and `glad.c` as compilation units.

1. Select an **x64** configuration. If Visual Studio reports duplicate configuration entries, use **Configuration Manager** to keep one valid `Debug|x64` entry; the project XML currently contains a duplicate.
2. In **Solution Explorer → Source Files → Add → Existing Item**, add:

   ```text
   Audio.cpp
   Easter.cpp
   Input.cpp
   Render.cpp
   SaveLoad.cpp
   Scenario.cpp
   UI.cpp
   Weapons.cpp
   World.cpp
   ```

   Keep `main.cpp` and `glad.c` in the project. Compile each source file once.

3. In **Project Properties**, apply the following settings to the configuration you will build:

   | Setting | Value |
   | --- | --- |
   | C/C++ → General → Additional Include Directories | `$(ProjectDir)libraries\include;%(AdditionalIncludeDirectories)` |
   | C/C++ → Language → C++ Language Standard | ISO C++20 |
   | Linker → General → Additional Library Directories | `$(ProjectDir)libraries\lib;%(AdditionalLibraryDirectories)` |
   | Linker → Input → Additional Dependencies | `opengl32.lib;glfw3.lib;%(AdditionalDependencies)` |
   | Debugging → Working Directory | `$(ProjectDir)` |

4. Remove obsolete absolute paths from **VC++ Directories** and any remaining configuration-specific include settings. Preserve inherited system paths.

Use the Windows SDK's `windows.h` and the MSVC runtime's `direct.h` on Windows. The repository also contains small compatibility headers with those names; do not add the repository root as a global include directory that overrides the system headers.

### 3. Build and Run

Build the project, then press **F5** or **Ctrl+F5**. The application opens fullscreen on the primary monitor.

At startup, `main.cpp` changes the working directory to `./helloWorldGLFW`. Keep the asset folder in place and launch with the repository root as the working directory.

## Controls

### Camera and Navigation

| Input | Action |
| --- | --- |
| `W`, `A`, `S`, `D` | Move the free camera |
| Mouse movement | Look around when the cursor is captured |
| `Tab` | Toggle captured mouse / visible cursor |
| Mouse wheel | Change field of view, or zoom toward/away from a selected body |
| Left-click a body or sidebar entry with the cursor visible | Select and focus a body |
| Arrow keys | Orbit the selected body |
| Middle-button drag with the cursor visible | Orbit the selected body |
| Right-click with the cursor visible | Cancel slingshot charging and leave the selected-body view |
| `T` | Toggle the automatic planet tour |
| `C` | Enable or disable cinematic event views |
| `Esc` | Exit |

### Simulation and Display

| Key | Action |
| --- | --- |
| `P` | Pause / resume physics |
| `=` / `+` or numpad `+` | Increase simulation speed, up to 8× |
| `-` or numpad `-` | Decrease simulation speed, down to 0× |
| `R` | Reset the scene |
| `[` / `]` | Decrease / increase the selected body's mass |
| `,` / `.` | Decrease / increase the selected body's size |
| `F` | Toggle crater and cloud effects |
| Hold `Z` / `X` | Rotate the manual light source |
| `Y` | Restore lighting from the Sun |
| `H` / `F1` | Show / hide help |
| `F9` | Run the automatic FPS benchmark; results go to the console, the screen and `helloWorldGLFW/benchmark_results.txt` |
| `M` | Mute / unmute audio |

### Weapons and Projectiles

| Input | Action |
| --- | --- |
| `Q` / `E` | Select the previous / next weapon |
| Click a weapon button with the cursor visible | Choose a weapon |
| Left-click the surface of an already selected body | Fire the selected weapon |
| Left-click with the mouse captured, without hitting a selected body | Launch a projectile |
| Right-click with the mouse captured | Launch a rocket |
| Middle-click with the mouse captured | Launch a black hole |
| Left-drag empty space with the cursor visible, then release | Aim and launch a slingshot projectile |
| `K` | Fire the battle station at the selected planet |

### Scenarios and Saves

| Key | Action |
| --- | --- |
| `7` | Put Earth and Mars on a collision course |
| `8` | Ignite Jupiter in a fictional star scenario |
| `9` | Trigger the solar supernova scenario |
| `0` | Introduce a rogue black hole |
| `1`, `2`, `3` | Choose the active save slot |
| `O` | Save to the active slot |
| `L` | Load the active slot |

Save files are written as `save_slot_1.txt`, `save_slot_2.txt`, and `save_slot_3.txt` in `helloWorldGLFW/` when launched as described above. Saving overwrites the chosen slot.

The save format stores the first nine bodies—the Sun and eight planets—including mass, position, velocity, size, destruction state, health, population, star flags, and impacts. It is not a complete session snapshot: camera settings, spawned projectiles, debris, and all temporary effects are not saved.

## Suggested Demo

1. Launch the application and press `H`/`F1` to view help.
2. Press `T` for a tour of the Solar System, then press it again to stop.
3. Press `Tab`, select Earth in the sidebar, and use the wheel and arrow keys to inspect it.
4. Use `[` / `]` to change its mass and observe the simulation.
5. Press `1`, then `O` to save the current planetary state.
6. Press `7` to start the Earth–Mars collision scenario, or select a weapon and click the selected planet's surface.
7. Press `R` to restore the initial scene, or `L` to load the selected save slot.

## Project Structure

| File or Directory | Responsibility |
| --- | --- |
| `main.cpp` | Initialization, asset loading, main loop, scene rendering, and much of the interface |
| `Globals.h` | Shared structures, simulation state, constants, and function declarations |
| `Render.cpp` | Shader loading, textures, mesh generation, and drawing helpers |
| `World.cpp` | Gravity, integration, collisions, projectiles, debris, and trajectory prediction |
| `Input.cpp` | Keyboard and mouse input, camera controls, picking, and slingshot interaction |
| `Weapons.cpp` | Weapon strikes, impact damage, and delayed attacks |
| `Scenario.cpp` | Preset sandbox scenarios |
| `SaveLoad.cpp` | Save-slot paths and planetary-state serialization |
| `Audio.cpp` | Procedural audio and video audio output |
| `Easter.cpp` | Hidden video playback and its controls |
| `UI.cpp` | Toast notification helper |
| `Benchmark.cpp` | Automatic FPS benchmark (technique ON vs OFF) and its on-screen status text |
| `helloWorldGLFW/shaders/` | Scene, interface, and video shaders |
| `helloWorldGLFW/textures/` | Planet, Moon, asteroid, ring, and background textures |
| `libraries/` | Bundled graphics headers and GLFW library |
| `helloWorldGLFW.vcxproj` | Visual Studio project |

## Implementation Notes

**Rendering:** Objects use model, view, and perspective projection matrices. A shared UV sphere mesh is reused across celestial bodies; Saturn's rings use a separate ring mesh. The scene shader combines texture color with ambient and diffuse lighting, procedural clouds, and impact effects.

**Physics:** Active bodies interact through softened pairwise gravity. Positions and velocities are updated using velocity Verlet integration. Large frame time steps are split into smaller physics steps. The Moon follows a separate scripted orbit around Earth rather than participating as an independent gravitational body.

**Selection:** Mouse coordinates are transformed into a world-space ray. Ray–sphere intersection identifies bodies, while the sidebar offers direct selection.

**Trajectory preview:** Slingshot aiming predicts a path using a copy of the simulation state and a massless test body.

## Troubleshooting

| Problem | What to check |
| --- | --- |
| Toolset `v145` is missing | Install the requested toolset or retarget the project to a compatible installed one. |
| GLFW, GLAD, or GLM headers cannot be found | Replace machine-specific paths with `$(ProjectDir)libraries\include`. |
| Unresolved project functions during linking | Add all nine additional `.cpp` modules listed in the setup instructions. |
| `glfw3.lib` cannot be found or is incompatible | Check the library directory and that the library matches the target architecture and compiler settings. |
| Shaders cannot be opened or textures are missing | Set the working directory to the repository root and preserve `helloWorldGLFW/shaders/` and `helloWorldGLFW/textures/`. |
| Windows build reports `unistd.h` errors | Check that the local compatibility headers are not shadowing Windows/MSVC headers. |
| Camera movement seems constrained | Stop the tour and leave the selected-body view using right-click with the cursor visible. |
| The simulation appears frozen | Press `P` and increase the speed if it is set to 0×. |
| A save does not restore every object | Save slots contain the nine main bodies, not the complete sandbox session. |

## Easter Egg

Enter `Up, Up, Down, Down, Left, Right, Left, Right, B, A` to open the hidden video player. Press `V` or use its close button to stop playback. It reads `helloWorldGLFW/nevergonna.mpg`; the included MP4 is not the playback input.

## Team

**Miras · Bainazar · Tileukhan**


