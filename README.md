# Hybrid PBR / NPR Shading Viewer

A real-time 3D viewer with one shader that mixes physically based rendering (PBR) and
non-photorealistic rendering (NPR, also called stylized or toon shading) inside a single material.

Each lighting term (diffuse, specular, shadow, ambient) can be fully physical, fully stylized, or a
weighted blend of the two, set with sliders while the program runs. The viewer shows the pure PBR,
pure NPR and hybrid versions of the same model side by side under a light you can move.

Final project for CSCI 420 Computer Graphics at the University of Southern California, Fall 2026.

> **Status: early development.** The window, the control panel, an orbit camera and a test sphere
> work. The sphere already uses the hybrid shader with its blend sliders and is shown as
> PBR, Hybrid and NPR side by side. The PBR side has its real Cook-Torrance shading, while the
> NPR side and the ambient light are still simple placeholder formulas. The build steps have
> been checked on Windows; macOS and Linux are not tested yet.

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
internet connection the first time you configure, because CMake downloads GLFW, GLM, Dear ImGui and
Assimp then. Assimp brings its own copy of zlib, so you don't need zlib installed. glad and
stb_image are already in `third_party/`.

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
and `assets` folders and double-click it; nothing needs to be installed.

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
buttons, the light direction, intensity and ambient color, exposure and the tone curve with two
ticks (see Tone mapping below), the base color, metallic, roughness and shininess, and the blend
sliders (Shadow, Diffuse, Specular, Ambient, Tone mapping). A blend slider at 0 uses
the physical version of that term and at 1 the stylized version; values in between mix the two. The
small PBR and NPR labels above the sliders mark these two ends, and hovering a slider says the same.
Each slider has a small number box next to it where you can type a value and press Enter.
The fifth blend slider, Tone mapping, works the same way for the screen step described below.
The PBR diffuse and specular are real Cook-Torrance shading. The stylized terms and both ambient
terms are still placeholders, for example the stylized diffuse is just two flat bands and the
stylized highlight a Phong spot. The Shadow slider has no visible effect yet, because there are no
shadows until the shadow map is added.
The window opens maximized.

### Loading a model

The Model menu in the panel lists "Sphere", "Sphere grid" and every model file in
`assets/models/` (glTF `.gltf` and `.glb`, and also `.obj` and `.fbx`). Pick one and it replaces
what is shown. A `.gltf` file needs its `.bin` file and its images next to it; a `.glb` file holds
everything in one file. To try your own model, copy it into `assets/models/` and start the viewer
again, or give its path when you start the program:

```
build\Release\viewer.exe C:\path\to\model.glb
```

The model is moved to the middle of the grid and scaled so its largest side is as big as the test
sphere, with its bottom on the grid. It is drawn with its own base color, metallic and roughness
values and textures.

"Sphere grid" shows 7 x 7 spheres for checking the PBR shading: metallic goes from 0 in the bottom
row to 1 in the top row, and roughness from 0 on the left to 1 on the right.

Which Material controls change what: on "Sphere" the Base color, Metallic, Roughness and Shininess
controls all apply. On "Sphere grid" only Base color (every sphere takes it) and Shininess apply,
because each sphere has its own metallic and roughness. A model file uses only Shininess; its
color, metallic and roughness come from the file. Shininess only sizes the stylized highlight.

Each texture uses the UV set (first or second) and the wrap and filter settings the file gives it.
Texture transforms (the glTF KHR_texture_transform extension) are not applied yet; the console
prints a warning when the base color or metallic-roughness texture has one. If a file cannot be
loaded, the reason shows in red under the Model menu and in the console, and the model that was
shown before stays.

The View mode menu at the top of the panel picks which versions of the model you see next to each
other. There are three views: PBR uses only the physical terms (all blend sliders at 0), NPR uses
only the stylized terms (all at 1), and Hybrid uses the slider values, so the sliders only change
the Hybrid view. "PBR | Hybrid | NPR" shows all three, and the other modes show one view or a pair.
PBR is always on the left and NPR on the right. All views share one camera and one light, so
dragging the mouse anywhere left of the panel turns all of them together. Narrow views get a wider
field of view from top to bottom, so the model still fits across each one.

The Debug view menu under it shows one value on its own in every view: Default is the normal
picture, then base color, metallic, roughness, the surface normal (as a color, x red, y green, z
blue), shadow visibility (white everywhere until shadows are added), and the diffuse, specular or
ambient term alone. Metallic and roughness are used by the PBR side; the stylized side does not
use them yet.

Each view also shows a faint grid on the floor under the model, with the X axis line in red and
the Z axis line in blue, and a small set of axes in its bottom left corner: X red, Y green, Z blue.
The corner axes turn with the camera, so they show which way you are looking. The Show grid and
Show axes checkboxes under the Debug view menu turn them on and off.

The shader is split into four files in `shaders/`: `common.glsl` (shared values), `pbr_terms.glsl`
(physical side), `npr_terms.glsl` (stylized side) and `hybrid_main.glsl` (mixes the two). The
program joins them in that order. If a file has an error, `R` keeps the old picture and the console
shows the file name and line.

Planned: middle mouse drag to pan, `1` `2` `3` `4` as shortcuts for the view modes, and `Tab` to
show or hide the panel. The panel will also get the ramp.

### Tone mapping

The lighting can get brighter than a screen can show, so the picture goes through one last step
before it reaches the window. The Tone mapping section of the panel has the Exposure slider (it
scales the brightness, 1 leaves it as it is) and the Tone curve menu: Off just cuts everything
above 1 off, while Reinhard and ACES fit (a film-like curve) squeeze bright values back into range.
The background, the grid and the axes skip exposure and the curve, so they look the same whatever
you pick here. The base color and ambient color you pick in the panel are read as normal screen
colors, the same way color textures are, and turned into light values before the shading.

The PBR view always gets exposure and the curve (in a debug view only while the debug tick below
is on). The NPR uses tone mapping tick decides whether the NPR view gets them too; untick it to see
the NPR colors exactly as computed (only cut off at 1). The Hybrid view sits in between: the Tone
mapping slider in the Blend section shows the Hybrid view the way PBR is shown at 0 and the way NPR
is shown at 1. It only changes how the Hybrid view is shown, not its shading. While the tick is
on, both ends are the same, so that slider changes nothing.

The Exposure and tone curve on debug view tick decides whether a debug view gets this step too. It
sets itself each time you pick a debug view: off for base color, metallic, roughness, normal and
shadow, on for the diffuse, specular and ambient terms (which are light and can go above 1). You
can change it afterwards. Metallic, roughness, normal and shadow are numbers rather than colors, so
they are always shown as stored, without the sRGB step. The Default view always
gets the full step, so the tick is grayed out there.

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
| [stb_image](https://github.com/nothings/stb) | 2.30 | Image loading, copied into `third_party/stb` | MIT or public domain |
| [Assimp](https://github.com/assimp/assimp) | 6.0.5 | Model loading (glTF, OBJ and FBX) | BSD 3-Clause |
| [zlib](https://zlib.net/) | 1.2.13 | Unpacking compressed data for Assimp, built from the copy inside Assimp | zlib |

### Assets

Every model, texture and environment map is listed with its author, source and license in
[assets/CREDITS.md](assets/CREDITS.md). All assets are CC0 or CC BY.

### References

- Karis, "Real Shading in Unreal Engine 4," SIGGRAPH 2013 course.
  https://blog.selfshadow.com/publications/s2013-shading-course/
- de Vries, LearnOpenGL, "PBR: Theory," "PBR: Lighting" and "Specular IBL."
  https://learnopengl.com/PBR/Theory and https://learnopengl.com/PBR/Lighting
- Khronos Group, "glTF 2.0 Specification," Appendix B: BRDF Implementation.
  https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html
- Khronos Group, "PBR in glTF." https://www.khronos.org/gltf/pbr
- Walter, Marschner, Li and Torrance, "Microfacet Models for Refraction through Rough Surfaces,"
  Eurographics Symposium on Rendering 2007.
  https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.html
- Schlick, "An Inexpensive BRDF Model for Physically-based Rendering," Computer Graphics Forum
  13(3), 1994. https://doi.org/10.1111/1467-8659.1330233
- Heitz, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs," Journal of
  Computer Graphics Techniques 3(2), 2014. https://jcgt.org/published/0003/02/03/
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
