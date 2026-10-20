# OpenGL Solar System Simulation
An interactive 3D Solar System simulation built with **modern OpenGL Core Profile 3.3**, **GLFW**, **GLAD**, **GLM**, and **stb_image**.
This project demonstrates real-time 3D rendering, shader-based lighting, texture mapping, camera movement, reusable mesh generation, and hierarchical transformations such as planets orbiting the Sun and the Moon orbiting Earth.
---
## Features
### 3D Rendering
- Uses the modern OpenGL rendering pipeline.
- Uses custom vertex and fragment shaders.
- Generates a procedural UV sphere mesh for planets, the Sun, the Moon, asteroids, and the star background.
- Reuses one sphere mesh for multiple objects to improve efficiency.
- Supports texture mapping using `stb_image`.
- Uses per-fragment lighting with ambient and diffuse light.
### Solar System Simulation
- Renders the Sun, Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, and Neptune.
- Each planet is defined using a data-driven `Body` structure.
- Planets rotate on their own axes.
- Planets revolve around the Sun.
- Earth has a Moon that orbits relative to Earth’s position.
- Saturn includes a custom ring mesh with a slight tilt.
- Circular orbit paths are drawn to visualize planetary movement.
### Additional Space Effects
- A textured star background is rendered using a large skysphere.
- An asteroid belt is generated using hundreds of small rotating asteroid objects.
- Shooting stars are spawned randomly and rendered as fading line trails.
- Loading messages are displayed in the window title while textures are being loaded.
### Camera System
The project uses a first-person style camera system.
- Mouse movement controls yaw and pitch.
- `W`, `A`, `S`, and `D` keys move the camera.
- Camera movement uses delta time for smoother motion.
- `TAB` toggles mouse capture on and off.
- `ESC` closes the simulation window.
### Transformations
The simulation uses the standard 3D graphics transformation pipeline:
```text
Model → View → Projection
```
Each object uses a combination of:
- Translation for orbit position.
- Rotation for spin and movement.
- Scaling for object size.
### Projection
- Uses a perspective projection camera.
- Uses a dynamic aspect ratio based on the current framebuffer size.
- Handles window resizing safely.
---
## Technologies Used
- C++
- OpenGL 3.3 Core Profile
- GLFW
- GLAD
- GLM
- stb_image
- Visual Studio
---
## Getting Started
### 1. Clone the Repository
```bash
git clone https://github.com/xbze3/solar-system-simulation-opengl
```
### 2. Open the Project
1. Navigate to the project folder.
2. Open the Visual Studio solution file (`.slnx`).
3. Make sure the project is selected as the startup project.
### 3. Build and Run
1. Select either **Debug** or **Release** mode.
2. Click **Local Windows Debugger** or press **F5**.
3. The simulation window should open and display the Solar System.
---
## Dependencies
All required dependencies are included locally in the project:
- **GLFW** - window creation and input handling.
- **GLAD** - OpenGL function loading.
- **GLM** - vector and matrix mathematics.
- **stb_image** - texture loading.
No additional dependency installation should be required if the project is opened and built in Visual Studio with the included files.
---
## Required Assets
The project uses texture files stored in the `textures` folder. These include textures for:
- Stars
- Sun
- Mercury
- Venus
- Earth
- Mars
- Jupiter
- Saturn
- Uranus
- Neptune
- Moon
- Asteroid
Make sure the `textures` folder remains in the correct location relative to the project files.
---
## Controls
| Key / Input | Action                |
| ----------- | --------------------- |
| `W`         | Move forward          |
| `S`         | Move backward         |
| `A`         | Move left             |
| `D`         | Move right            |
| Mouse       | Look around           |
| `TAB`       | Toggle cursor capture |
| `ESC`       | Exit the simulation   |
---
## Implementation Highlights
### Data-Driven Planet System
Planets are stored using a `Body` structure:
```cpp
struct Body {
    std::string  name;
    float        scaledSize;
    float        orbitRadius;
    float        rotationSpeed;
    float        revolutionSpeed;
    glm::vec3    fallbackColor;
    unsigned int texture;
};
```
This makes it easier to add or modify planets without rewriting the rendering logic.
### Reusable Mesh Strategy
A single UV sphere mesh is generated once and reused for the Sun, planets, Moon, asteroids, and skysphere. This reduces duplicated geometry and keeps the rendering system cleaner.
### Orbit Mechanics
Each planet’s position is calculated using circular motion:
```cpp
x = cos(angle) * orbitRadius;
z = sin(angle) * orbitRadius;
```
The angle changes over time based on each planet’s revolution speed.
### Moon System
The Moon’s position is calculated relative to Earth’s current position. This demonstrates hierarchical transformation because the Moon’s motion depends on the movement of another object.
### Saturn Ring
Saturn’s ring is generated using a custom ring mesh with an inner and outer radius. It is rendered separately from Saturn and tilted slightly to create a more realistic visual effect.
### Asteroid Belt
The asteroid belt is generated using a collection of asteroid objects. Each asteroid has its own orbit radius, orbit angle, speed, size, and vertical offset.
### Shooting Stars
Shooting stars are spawned randomly around the scene. Each shooting star has a start position, direction, speed, trail length, lifetime, and fading effect.
### Orbit Visualization
Circular line loops are used to display each planet’s orbital path around the Sun.
---
## Notes
- The simulation is designed for learning and demonstration purposes.
- Planet sizes, distances, and speeds are visually scaled and are not scientifically accurate.
- The project focuses on graphics programming concepts such as lighting, transformation, texture mapping, and camera control.
---
## Troubleshooting
### The project does not open
- Make sure you open the `.slnx` file in Visual Studio.
- Ensure that the full project folder was downloaded or cloned.
### Textures are missing or appear white
- Check that the `textures` folder exists.
- Make sure the texture file names match the names used in the source code.
- Keep the texture folder in the expected project directory.
### The build fails
- Rebuild the solution in Visual Studio.
- Make sure the project is using a compatible C++ compiler.
- Confirm that GLFW, GLAD, GLM, and stb_image are included correctly.
### The window opens but nothing displays
- Ensure your computer supports OpenGL 3.3 or higher.
- Update your graphics drivers if necessary.
- Try running the project in Release mode.
---
## Future Improvements
Possible improvements include:
- Add more realistic planetary scaling.
- Add elliptical orbits.
- Add zoom controls.
- Add labels for planets.
- Add more moons for other planets.
- Add improved lighting effects.
- Add a user interface for controlling simulation speed.
- Add sound effects or background music.
---
## License
This project is intended for educational use.
,
