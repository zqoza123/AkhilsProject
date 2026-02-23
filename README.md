# Particle Simulation

A 2D **physics-based particle simulation** built with OpenGL, GLFW, and GLM. It renders hundreds of circular particles that fall under gravity, collide with each other and the screen boundaries, and change color based on their speed.

## What It Does

- **750 particles** spawn at random positions in an 800×600 window
- Particles fall with **gravity** (−9.8 m/s² scaled to pixels)
- **Collision detection** uses a spatial grid to limit checks to nearby particles
- **Impulse-based collisions** with configurable restitution (bounciness)
- **Velocity-based coloring**: particles shift from red to blue as they move faster
- **VSync disabled** for higher frame rates and smoother physics

## Tech Stack

| Component | Purpose |
|-----------|---------|
| **OpenGL 3.3** | Rendering via GLAD loader |
| **GLFW** | Window creation and input |
| **GLM** | Math (vectors, matrices) |
| **GLSL** | Vertex and fragment shaders |

## Setup (macOS)

1. **Install dependencies** (Homebrew):
   ```bash
   brew install glfw glm
   ```

2. **Build**:
   ```bash
   make
   ```

3. **Run**:
   ```bash
   ./particle_sim
   ```
   Or: `make run`

## Controls

- **ESC** — Close the window
- **F** — Freeze (placeholder, not implemented)

## Project Structure

```
├── include/           # GLAD OpenGL headers
│   ├── glad/
│   └── KHR/
├── src/
│   ├── main.cpp       # Main loop, physics, rendering
│   ├── glad.c         # GLAD implementation
│   └── shaders/
│       ├── vertexShader.glsl
│       └── fragmentShader.glsl
├── .vscode/           # VS Code build & debug config
├── Makefile
└── README.md
```

## Configuration

Edit `main.cpp` to tweak:

- `particles` — Number of particles (default: 750)
- `radius` — Particle size (default: 4.0)
- `restitution` — Bounciness (default: 0.1)
- `solverIterations` — Collision resolution passes (default: 3)
