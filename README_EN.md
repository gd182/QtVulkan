<h1 align="center">QtVulkan</h1>

<p align="center">
  Real-time 3D viewer with physics simulation built on Qt 6 and Vulkan
</p>

###

<div align="center">
  <img src="https://skillicons.dev/icons?i=cpp" height="40" alt="C++ logo" />
  <img width="12" />
  <img src="https://skillicons.dev/icons?i=cmake" height="40" alt="CMake logo" />
  <img width="12" />
  <img src="https://skillicons.dev/icons?i=git" height="40" alt="git logo" />
  <img width="12" />
  <img src="https://skillicons.dev/icons?i=windows" height="40" alt="windows logo" />
  <img width="12" />
  <img src="https://skillicons.dev/icons?i=qt" height="40" alt="windows logo" />
</div>

###

## About

**QtVulkan** is a desktop application that embeds a Vulkan rendering surface inside a Qt Quick window. It renders a 3D sphere bouncing on a ground plane under a simple physics simulation. The QML sidebar provides live FPS readout, an elasticity slider, and a reset button — all wired to the Vulkan engine via `SceneController`.

## Features

- **Vulkan renderer** — swapchain, depth buffer, render pass, descriptor sets, and a graphics pipeline set up from scratch
- **Physics simulation** — gravity and elastic bounce off a ground plane with a configurable restitution coefficient
- **Orbit camera** — left-click drag to orbit, scroll wheel to zoom
- **QML UI overlay** — live FPS counter, elasticity slider (0.0 – 1.0), reset button
- **3D meshes** — procedural sphere and pyramid generators
- **GLSL shaders** — compiled at build time via `glslc` and copied next to the executable
- **Validation layers** — `VK_LAYER_KHRONOS_validation` and `VK_EXT_debug_utils` enabled in debug builds

## Tech Stack

| Component | Technology |
|-----------|------------|
| Language | C++20 |
| GUI / QML | Qt 6.8 (Quick, Gui, QuickControls2) |
| Graphics API | Vulkan 1.0 |
| Math library | GLM (header-only, via vcpkg) |
| Shaders | GLSL → SPIR-V via glslc |
| Build system | CMake 3.20+ |

## Project Structure

```
QtVulkan/
├── CMakeLists.txt           # Build configuration, shader compilation
├── Main.qml                 # QML window: FPS, elasticity slider, controls hint
├── main.cpp                 # Entry point: QVulkanInstance, VulkanWindow, QML engine
├── build_and_run.bat        # One-click build & launch on Windows
└── src/
    ├── VulkanCore.cpp/h     # Full Vulkan pipeline (device, swapchain, renderpass…)
    ├── VulkanWindow.cpp/h   # QVulkanWindow subclass, render loop
    ├── SceneController.cpp/h # QObject bridge between QML and the Vulkan engine
    ├── Camera.cpp/h         # Orbit camera (azimuth / elevation / radius)
    ├── Physics.cpp/h        # Ball physics: gravity + bounce (BallPhysics)
    ├── vk3dSphere.cpp/h     # Procedural sphere mesh
    ├── vk3dPyramid.cpp/h    # Procedural pyramid mesh
    ├── vk3dObject.cpp/h     # Base 3D object helper
    ├── Transform.cpp/h      # Transform data (position, rotation, scale)
    └── TypesData.h          # Shared vertex / UBO structs
```

## Requirements

- Windows 10/11 (64-bit)
- **Qt 6.8+** with the Quick and QuickControls2 modules
- **Vulkan SDK** (1.0+) — provides `glslc` for shader compilation
- **GLM** — install via vcpkg: `vcpkg install glm`
- **CMake 3.20+**
- A GPU with Vulkan 1.0 support

## Build & Run

### 1. Clone the repository

```bash
git clone <repo-url>
cd QtVulkan
```

### 2. Configure and build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

CMake will automatically compile `sphere.vert` / `sphere.frag` to SPIR-V with `glslc` and place the `.spv` files next to the executable.

### 3. Run

```bash
./build/Release/appQtVulkan.exe
```

Or use the provided helper script:

```bat
build_and_run.bat
```

## Controls

| Action | Input |
|--------|-------|
| Orbit camera | Left mouse button drag |
| Zoom | Scroll wheel |
| Reset physics | `R` or `Space` (or sidebar button) |
| Adjust elasticity | Sidebar slider (0.0 = plastic, 1.0 = elastic) |

## Shaders

Shaders are sourced from `shaders/` relative to the project root and compiled into `shaders/compiled/`:

| File | Role |
|------|------|
| `sphere.vert` | Vertex shader — MVP transform + normal |
| `sphere.frag` | Fragment shader — diffuse + ambient lighting |
