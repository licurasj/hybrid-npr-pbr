# Hybrid PBR / NPR Shading Viewer

A real-time 3D viewer with one shader that mixes physically based rendering (PBR) and
non-photorealistic rendering (NPR, also called stylized or toon shading) inside a single material.

Each lighting term (diffuse, specular, shadow, ambient) can be fully physical, fully stylized, or a
weighted blend of the two, set with sliders while the program runs. The viewer shows the pure PBR,
pure NPR and hybrid versions of the same model side by side under a light you can move.

Final project for CSCI 420 Computer Graphics at the University of Southern California, Fall 2026.

> **Status: early development.** The window, the control panel and a Phong-lit test sphere with an
> orbit camera work. The PBR and NPR shading described below is not written yet. The build steps
> have been checked on Windows; macOS and Linux are not tested yet.

## What it does

- **Physical side.** Cook-Torrance microfacet BRDF (GGX distribution, Smith geometry term, Schlick
  Fresnel) with Lambert diffuse, driven by base color, metallic and roughness textures. HDR
  rendering with tone mapping. Environment reflections that blur as roughness increases.
- **Stylized side.** Ramp (toon) diffuse that looks up a 1D color ramp, and a hard-edged highlight
  whose size still follows roughness.
- **Shadows.** One shadow-mapped directional light. On the stylized side the shadow goes into the
  ramp lookup, so shadow edges form the same bands as the shading.
- **Hybrid.** One blend weight per lighting term, changeable at run time.
- **Test materials.** Opaque textured surfaces: cloth, leather, metal trim, stone and wood.

Skin, hair, glass and anything that needs subsurface scattering or transmission are out of scope.

## Requirements

All platforms need a GPU and driver with OpenGL 4.1 core profile, CMake 3.24 or newer, Git, and an
internet connection the first time you configure, because CMake downloads GLFW, GLM and Dear ImGui
then. glad is already in `third_party/`.

### Windows

1. Install [Visual Studio 2022 or newer](https://visualstudio.microsoft.com/) (the free Community
   edition works) with the **Desktop development with C++** workload.
2. Install [CMake](https://cmake.org/download/) and [Git](https://git-scm.com/download/win). Let both
   installers add themselves to the PATH.
3. Make sure your graphics driver is up to date.

### macOS

1. Install the Xcode command line tools: `xcode-select --install`
2. Install CMake and Git, for example with [Homebrew](https://brew.sh/): `brew install cmake git`

macOS supports OpenGL up to 4.1, which is the version we use. Apple marks OpenGL as deprecated, so
the build may show warnings that mention deprecation. They are harmless.

### Linux

Install a C++ compiler, CMake, Git and the window system packages GLFW needs. On Ubuntu or Debian:

```
sudo apt install build-essential cmake git libwayland-dev libxkbcommon-dev xorg-dev
```

## Build and run

Get the code, then configure and build from the folder that holds `CMakeLists.txt`:

```
git clone <repository url>
cd hybrid-npr-pbr
cmake -S . -B build
cmake --build build --config Release
```

On Windows use PowerShell or the Command Prompt; CMake finds Visual Studio by itself. Then start the
program.

On Windows:

```
build\Release\viewer.exe
```

On macOS and Linux:

```
./build/viewer
```

You can also double-click `viewer.exe`. A console window opens next to the viewer and shows shader
errors and other messages.

If you received a ready-made `viewer.exe` for Windows, keep it in the same folder as the `shaders`
folder and double-click it; nothing needs to be installed.

The program reads its shaders from the `shaders/` folder of the copy it was built from, so editing a
shader and pressing `R` shows the change right away. If you move or rename the project folder,
delete the `build` folder and run the two `cmake` commands again.

If the window stays empty or the program stops at once, read the console. "Could not open a window
with OpenGL 4.1" means the driver does not offer OpenGL 4.1; updating it usually helps.

## Controls

| Input | Action |
|---|---|
| Left mouse drag | Orbit the camera around the model |
| Mouse wheel | Zoom |
| Right mouse drag | Move the light direction |
| `R` | Reload shaders from disk |
| `Esc` | Quit |

The control panel on the right shows the frame rate and OpenGL version, Reset camera and Reset light
buttons, the light direction and the Phong material settings. The window opens maximized.

Planned: middle mouse drag to pan, `1` `2` `3` `4` to show PBR only, NPR only, hybrid only or all
three side by side, and `Tab` to show or hide the panel. The panel will get the four blend weights,
exposure and tone mapping, the ramp, the model and material, and debug views that show one input or
one lighting term on its own.

## Documentation

- [Architecture overview](docs/architecture.md): how the program is put together
- [Work plan](docs/work-plan.md): the parts of the project and their order
- [Milestone guide](docs/milestone-guide.md): files, steps and formulas for each part

## Credits

### Libraries

| Library | Version | Use | License |
|---|---|---|---|
| [GLFW](https://www.glfw.org/) | 3.5.1 | Window and input | zlib |
| [GLM](https://github.com/g-truc/glm) | 1.0.3 | Vector and matrix math | MIT |
| [Dear ImGui](https://github.com/ocornut/imgui) | 1.93.0 | Control panel | MIT |
| [glad](https://github.com/Dav1dde/glad) | 2.0.8 | OpenGL 4.1 core function loader, generated into `third_party/glad` | Generated files: WTFPL or CC0, with Apache 2.0 for the parts from the Khronos registry |
| [stb_image](https://github.com/nothings/stb) | not added yet | Image loading | Public domain or MIT |
| [tinygltf](https://github.com/syoyo/tinygltf) | not added yet | glTF model loading | MIT |

### Assets

Every model, texture and environment map is listed with its author, source and license in
[assets/CREDITS.md](assets/CREDITS.md). All assets are CC0 or CC BY.

### References

- Karis, "Real Shading in Unreal Engine 4," SIGGRAPH 2013 course.
  https://blog.selfshadow.com/publications/s2013-shading-course/
- LearnOpenGL, "PBR Theory" and "Specular IBL." https://learnopengl.com/PBR/Theory
- Khronos Group, "PBR in glTF." https://www.khronos.org/gltf/pbr
- Gooch, Gooch, Shirley and Cohen, "A Non-Photorealistic Lighting Model for Automatic Technical
  Illustration," SIGGRAPH 1998. https://doi.org/10.1145/280814.280950
- Mitchell, Francke and Eng, "Illustrative Rendering in Team Fortress 2," NPAR 2007.
  https://www.cs.princeton.edu/courses/archive/fall07/cos597B/papers/mitchell-team-fortress.pdf
- kwandrus, "Non-Photorealistic GLSL Shaders."
  https://github.com/kwandrus/Non-Photorealistic-GLSL-Shaders
- LZJU-1, "NPR-PBR," a Unity project that combines toon and PBR shading on a character. The closest
  existing project we found. https://github.com/LZJU-1/NPR-PBR

## Authors

Yonghao Jin and Marios Tsotras
