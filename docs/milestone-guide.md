# Milestone guide

A cheat sheet for each part of the project: what gets built, in which files, roughly how, and the
formulas and library calls you'll want. The parts are in the same order as in the work plan
(`work-plan.md`). For how everything fits together, see `architecture.md`.

Treat it as a starting point, not as instructions. If you find a simpler way, go for it. None of
this has been compiled yet, so please double-check library function names against their docs the
first time you use one. The formulas are the standard ones (sources at the end), and our own
function and uniform names are only suggestions.

Symbols used in formulas:

| Symbol | Meaning |
|---|---|
| N | Unit surface normal |
| L | Unit direction from the surface to the light |
| V | Unit direction from the surface to the camera |
| H | Half vector, `normalize(L + V)` |
| R | Reflection of the view direction, `reflect(-V, N)` |
| N.L | Dot product of N and L, clamped to 0 or above unless stated |
| base | Base color, linear |
| radiance | Light color times intensity |

Side labels: **PBR** is the physical side, **NPR** is the stylized side, **Both** is used by both.

---

## Project skeleton (Both)

**What gets done.** The project builds from the README commands and opens a window with an OpenGL
4.1 core context, a clear color and an empty ImGui panel.

**Files.** `CMakeLists.txt`, `src/main.cpp`, `src/ui.h/.cpp`, `src/settings.h`, `third_party/` for
glad and stb.

**How.**

1. CMake: C++17, one executable. `FetchContent_Declare` and `FetchContent_MakeAvailable` for GLFW,
   GLM and Dear ImGui at fixed tags. glad and stb are copied into `third_party/`.
2. Window, in `main.cpp`:
   - `glfwInit()`
   - `glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4)`, `GLFW_CONTEXT_VERSION_MINOR, 1`,
     `GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE`, `GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE`.
     The last one is required on macOS.
   - `glfwCreateWindow`, `glfwMakeContextCurrent`, `glfwSwapInterval(1)`
   - `gladLoadGL(glfwGetProcAddress)` (glad 2; the older glad 1 used `gladLoadGLLoader`)
   - Put `glGetString(GL_VERSION)` in the title with `glfwSetWindowTitle`.
3. ImGui setup: `ImGui::CreateContext()`, `ImGui_ImplGlfw_InitForOpenGL(window, true)`,
   `ImGui_ImplOpenGL3_Init("#version 410 core")`.
4. Frame loop until `glfwWindowShouldClose`:
   - `glfwPollEvents()`
   - `glfwGetFramebufferSize` and `glViewport`. Use the framebuffer size, not the window size.
     They differ on Mac Retina screens.
   - `glClearColor`, `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`
   - `ImGui_ImplOpenGL3_NewFrame()`, `ImGui_ImplGlfw_NewFrame()`, `ImGui::NewFrame()`
   - `ImGui::Begin("Controls")`, `ImGui::End()`
   - `ImGui::Render()`, `ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData())`
   - `glfwSwapBuffers(window)`
5. `settings.h`: an empty `struct Settings` for now. Later milestones add fields.

---

## Phong sphere (Both)

**What gets done.** A generated sphere lit with Phong shading. The mouse orbits the camera and
moves the light. `R` reloads the shaders.

**Files.** `src/shader.h/.cpp`, `src/mesh.h/.cpp`, `src/camera.h/.cpp`, `shaders/mesh.vert`,
`shaders/phong.frag` (temporary; the term skeleton replaces it with the four shader files).

**How.**

1. `shader` module.
   - Read a file into a string. `glCreateShader`, `glShaderSource`, `glCompileShader`.
   - Check with `glGetShaderiv(id, GL_COMPILE_STATUS, ...)` and print `glGetShaderInfoLog`.
   - `glCreateProgram`, `glAttachShader`, `glLinkProgram`, check `GL_LINK_STATUS` with
     `glGetProgramiv` and `glGetProgramInfoLog`.
   - Uniform setters: `glGetUniformLocation`, then `glUniform1i`, `glUniform1f`, `glUniform3fv`,
     `glUniformMatrix4fv`.
   - Reload: build a new program. If it fails, keep the old one and print the error.
2. `mesh` module.
   - Sphere with `stacks` rows and `slices` columns. For row i and column j:
     `theta = pi * i / stacks`, `phi = 2 * pi * j / slices`,
     `position = (sin(theta) cos(phi), cos(theta), sin(theta) sin(phi))`,
     `normal = position`, `uv = (j / slices, i / stacks)`.
   - Two triangles per grid cell in an index list.
   - Upload: `glGenVertexArrays`, `glBindVertexArray`, `glGenBuffers`, `glBindBuffer`,
     `glBufferData`, then `glVertexAttribPointer` and `glEnableVertexAttribArray` for
     attributes 0 position, 1 normal, 2 UV.
   - Draw: `glBindVertexArray`, `glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0)`.
3. `camera` module. Store `yaw`, `pitch`, `distance`, `target`.
   - `eye = target + distance * (cos(pitch) sin(yaw), sin(pitch), cos(pitch) cos(yaw))`
   - Clamp pitch to about plus or minus 89 degrees so the view never flips.
   - `glm::lookAt(eye, target, up)` and `glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f)`.
   - Input: `glfwGetCursorPos`, `glfwGetMouseButton`, `glfwSetScrollCallback`. Skip camera input
     while `ImGui::GetIO().WantCaptureMouse` is true.
   - The light direction uses the same yaw and pitch idea, on the right mouse button.
4. `mesh.vert`.
   - `worldPos = uModel * vec4(position, 1)`
   - `worldNormal = mat3(transpose(inverse(uModel))) * normal`
   - `gl_Position = uProjection * uView * worldPos`
5. `phong.frag` (later replaced by the four shader files of the term skeleton).
   - `R = reflect(-L, N)`
   - `color = ka * base + kd * base * max(N.L, 0) + ks * pow(max(dot(R, V), 0), shininess)`
6. `glEnable(GL_DEPTH_TEST)` once at startup. `glfwGetKey(window, GLFW_KEY_R)` triggers reload.

---

## Term skeleton and comparison view (Both)

**What gets done.** The hybrid shader structure with stand-in formulas, the four weight sliders,
the three views side by side and the debug view menu. After this, the PBR and NPR sides can be
written at the same time in separate files.

**Files.** `shaders/common.glsl`, `shaders/pbr_terms.glsl`, `shaders/npr_terms.glsl`,
`shaders/hybrid_main.glsl`, `src/shader.cpp`, `src/settings.h`, `src/renderer.h/.cpp`,
`src/ui.cpp`.

**How.**

1. Joining files in `shader`: read the four files and pass them to `glShaderSource` as an array of
   strings, with `"#version 410 core\n"` first. Order: common, pbr_terms, npr_terms, hybrid_main.
2. `common.glsl`: the uniforms, and the `Surface` and `Light` structs from the architecture
   overview.
3. The eight term functions, with stand-ins so everything runs today:

   | Function | Stand-in |
   |---|---|
   | `shadowPhysical(vis)` | `vis` |
   | `shadowStylized(vis)` | `step(0.5, vis)` |
   | `diffusePhysical(s, l, shadow)` | `s.baseColor / pi * N.L * l.radiance * shadow` |
   | `diffuseStylized(s, l, shadow)` | `s.baseColor / pi * step(0.5, (N.L * 0.5 + 0.5) * shadow) * l.radiance` |
   | `specularPhysical(s, l, shadow)` | The Phong highlight from the Phong sphere, times `shadow` |
   | `specularStylized(s, l, shadow)` | `step(0.5, that Phong highlight)`, times `shadow` |
   | `ambientPhysical(s)` | `0.03 * s.baseColor` |
   | `ambientStylized(s)` | `uAmbientColor * s.baseColor` |

   Until the shadow map exists, `vis` is 1. In the stylized diffuse, N.L is not clamped before the remap.
4. `hybrid_main.glsl`: build `Surface` and `Light`, call both versions of each term, blend with
   `mix(physical, stylized, weight)` as shown in the architecture overview, add the three results.
5. Debug view: `if (uDebugView == 1) { fragColor = vec4(s.baseColor, 1); return; }` and so on for
   metallic, roughness, normal (`N * 0.5 + 0.5`), visibility and each single term.
6. `settings.h`: `float weights[4]`, `int viewMode`, `int debugView`.
7. Comparison view in `renderer`: the view mode gives the list of views (PBR, Hybrid, NPR or a
   part of it) and so the number of strips, `count`. Then `glEnable(GL_SCISSOR_TEST)`, and for
   strip i from 0 to count - 1:
   - edges `left = i * w / count` and `right = (i + 1) * w / count`, `stripWidth = right - left`
   - skip the strip if `stripWidth` or `h` is below 1
   - `glViewport(left, 0, stripWidth, h)` and `glScissor` with the same numbers
   - projection aspect is `float(stripWidth) / float(h)`; a strip narrower than it is tall gets a
     wider vertical field of view, so the model still fits across it
   - send weights: all 0 for PBR, the slider values for Hybrid, all 1 for NPR
   - draw the model

   After the loop, `glDisable(GL_SCISSOR_TEST)`, or the next frame's clear only reaches the last
   strip.
8. UI: `ImGui::SliderFloat("Diffuse", &settings.weights[1], 0, 1)` for each weight,
   `ImGui::Combo` for view mode and debug view.

---

## HDR and color space (Both)

**What gets done.** The scene is drawn into a float target that can hold values above 1. A last
pass then applies exposure, a tone curve and the conversion to screen colors (sRGB), strip by
strip, so PBR, Hybrid and NPR can each get their own amount of it.

**Files.** `src/framebuffer.h/.cpp` (the HDR target), `src/color.h/.cpp` (one small sRGB helper),
`shaders/fullscreen.vert`, `shaders/tonemap.frag`, `src/renderer.cpp` (`drawToneMap`),
`src/main.cpp` (frame order), `src/texture.cpp` and `src/model.cpp` (sRGB textures),
`src/settings.h` and `src/ui.cpp` (the Tone mapping section and the fifth blend slider).

**A few words first.** *Multisampling* (MSAA) stores several samples per pixel, here 4, so edges
come out smooth. On an edge some samples belong to the model and some to what is behind it. The
usual way is to average the samples into a normal texture first (the *resolve*), but then the
model and the grid or background on an edge are mixed before the last pass can tell them apart.
So we skip that step: the last pass reads every sample on its own, treats it, and averages after.
*Linear* values are proportional to the amount of light, which is what the lighting math needs; *sRGB* is
the curved scale screens and image files use.

**How.**

1. HDR target, in `framebuffer` (`HdrTarget`, `createHdrTarget`):
   - Color is a multisample texture, so a shader can read its samples:
     `glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, HDR_SAMPLES, GL_RGBA16F, w, h, GL_TRUE)`,
     attached with `glFramebufferTexture2D(..., GL_TEXTURE_2D_MULTISAMPLE, tex, 0)`. It has no
     filters or wrap to set, since samples are read one by one.
   - Depth is only needed while drawing, so it stays a renderbuffer:
     `glRenderbufferStorageMultisample(GL_RENDERBUFFER, HDR_SAMPLES, GL_DEPTH_COMPONENT24, w, h)`,
     attached with `glFramebufferRenderbuffer`.
   - The depth buffer needs the same sample count as the color, and when a texture and a
     renderbuffer are mixed like this, the texture has to use fixed sample positions (the last
     `GL_TRUE`), or the framebuffer is incomplete.
   - 4 is the count we ask for. OpenGL 4.1 only promises 1 sample for a multisample texture
     (renderbuffers get at least 4), so we ask for fewer if `GL_MAX_COLOR_TEXTURE_SAMPLES` is
     lower. The driver may also give more, which works as long as color and depth get the same
     count, so `createHdrTarget` asks how many the color really has
     (`glGetTexLevelParameteriv` with `GL_TEXTURE_SAMPLES`) and the tone mapping pass reads
     that many. `glCheckFramebufferStatus` checks the result; when it fails, ours
     prints the reason and the real sample counts of color and depth (`GL_RENDERBUFFER_SAMPLES`
     for the depth).
   - `resizeHdrTarget` is called every frame with the framebuffer size and only makes everything
     again when the size changed.
2. Frame order, in `main.cpp`: read the framebuffer size once at the top of the frame, bind the
   HDR framebuffer, clear it, draw the strips as before (`drawScene`), bind framebuffer 0 (the
   window), draw the tone mapping pass (`drawToneMap`), then ImGui. Framebuffer 0 has to be bound
   before the pass, since a shader can't safely read a texture that is attached to the
   framebuffer it draws into. The window keeps its own 4 samples too (`GLFW_SAMPLES`), which keep
   the panel's arrows smooth.
3. `fullscreen.vert`: one triangle that covers the screen, with no vertex buffer.
   - `vec2 corner = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);`
   - `gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);`
   - The core profile still wants a vertex array bound for `glDrawArrays(GL_TRIANGLES, 0, 3)`,
     so `main.cpp` makes one empty vertex array for it. The pass unbinds it again at the end.
4. Reading the samples in `tonemap.frag`: the HDR texture is a `sampler2DMS`, bound to
   `GL_TEXTURE_2D_MULTISAMPLE`. The texture is the same size as the window, so the pixel's own
   position is its texel: `ivec2 pixel = ivec2(gl_FragCoord.xy)`. A loop reads each sample with
   `texelFetch(uHdrImage, pixel, i)`, gives it its own treatment (below), adds it up, and divides
   by the sample count at the end. No filtering is involved, so neighbouring pixels and strips
   never mix.
5. The treatment of one sample:
   - Take `max(c, 0)` first: negative values can't be shown, and `pow` is not defined for them.
   - Exposure goes before the curve: `c * uExposure`, then one of
     - Off: `clamp(c, 0, 1)`
     - Reinhard: `c / (1 + c)`
     - ACES fit (Narkowicz): `(c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14)`, and
       the result is clamped to 0..1. It darkens a little on purpose: an input of 1 comes out
       at about 0.8.
   - Then linear to sRGB per channel, on values already in 0..1:
     `c <= 0.0031308 ? 12.92 * c : 1.055 * pow(c, 1.0 / 2.4) - 0.055`.
   - `uToneAmount` says how much of exposure and the curve a strip gets. The shader makes the
     color both ways (only clamped, and with exposure and the curve), does the sRGB step on both,
     and mixes them by the amount.
6. One pass per strip (`drawToneMap` in `renderer.cpp`): the viewport stays the whole window, so
   each pixel reads its own place in the HDR picture, and `glScissor` limits each draw to
   one strip, with the same strip edges as `drawScene`. Depth test is off during the pass. The
   amount per strip:
   - PBR: always 1, the full treatment (in a debug view only while the debug tick is on, see
     Debug views below).
   - NPR: 1 when the "NPR uses tone mapping" tick is on (it starts on), 0 when off, so the NPR
     colors are shown as computed, only cut off at 1.
   - Hybrid: the fifth blend slider "Tone mapping" mixes the two,
     `(1 - w) * 1 + w * nprAmount`. Both ends are treatments of the Hybrid strip itself, with its
     own four blend weights: at 0 it is shown the way PBR is shown, at 1 the way NPR is shown. It
     doesn't turn the Hybrid strip into the PBR or NPR picture. Because the shader mixes the
     results by the amount, mixing the amounts gives the same as mixing the two treated Hybrid
     pictures. With the tick on, both ends are 1 and the slider changes nothing.
   - One more scissored draw with amount 0 covers the area under the panel, so nothing old is
     left there.
7. Scene mark in alpha: the model writes alpha 1 (`hybrid_main.glsl`), the grid and axes write
   0 (`line.frag`), and the clear color has alpha 0. The tone mapping pass gives pixels marked 0
   only the sRGB step, so the background, grid and axes look the same whatever exposure and
   curve are set to. Each sample has a clean 0 or 1, and on the model's edges the pass treats
   the model samples and the background samples each by their own rule before averaging.
8. Hand-picked colors are sRGB: the background gray (0.15), the grid and axis colors, and the
   panel's base color and ambient color were all picked by eye on screen. `srgbToLinear` in
   `color.cpp` turns them into linear values once, before they are used, so after the last pass
   they come out as picked. Later ramp colors should go the same way.
9. Textures, in `texture.cpp` and `model.cpp`: `TextureSettings` has an `srgb` flag. Base color
   and emissive textures set it and are uploaded as `GL_SRGB8_ALPHA8`, so the GPU turns their
   colors into linear values when the shader reads them (alpha is not converted).
   Metallic-roughness, normal and occlusion textures hold data, not colors, so they stay
   `GL_RGBA8`, and so does the shared white fallback (1 stays 1 either way). Since the format
   lives in the texture, "srgb" or "plain" is part of the texture cache key, next to the image
   name, wrap and filter.
10. Debug views:
    - Base color always gets the sRGB step, like the lit picture. The Khronos glTF Sample Viewer
      converts its Base Color view the same way.
    - Metallic, roughness, normal and shadow are data, so they never get the sRGB step and show
      their stored values. That keeps them matching the Khronos viewer.
    - The "Tone mapping on debug view" tick decides whether a debug view also gets
      exposure and the curve. It sets itself each time the Debug view menu changes: off for base
      color, metallic, roughness, normal and shadow, on for the diffuse, specular and ambient
      terms, which are light and can go above 1. You can change it afterwards. With it on, the
      data views get exposure and the curve but still no sRGB step. The Default view always gets
      the full treatment, so the tick is grayed out there.
11. Keep `GL_FRAMEBUFFER_SRGB` off. The shader does the conversion, so it happens exactly once,
    and ImGui draws straight onto the window afterwards with its colors unchanged.
12. Why the check numbers work (curve Off, exposure 1, PBR view, Diffuse debug view with the
    debug tick on, so it gets the same treatment as the lit picture): a white sphere with light
    intensity pi gives `1 / pi * 1 * pi = 1`, which is 255. At N.L = 0.5 the linear value 0.5
    becomes 0.735 after the sRGB step, which is 188, not 128. The Default view adds the ambient
    light and the highlight on top, so it reads a bit brighter there (about 192). A gray 128 texture reads 128 in the
    base color debug view: the texture turns it into linear and the last pass turns it back.
    Once Cook-Torrance is in, the PBR diffuse gets the `(1 - F)` weight (about 0.96 here), so the
    same check reads about 250 and 184.

---

## glTF loading (Both)

**What gets done.** A model file (`.gltf`, `.glb`, and also `.obj` or `.fbx`) becomes a list of
mesh and material pairs that the renderer can draw.

**About the loader.** We use Assimp. We expect to mix models from different places, and Assimp
reads OBJ and FBX as well as glTF. It can also work out tangents for us, which helps if we get to
normal mapping. Assimp doesn't decode the images for us, so stb_image does that. Only `model.cpp`
includes Assimp headers; the rest of the program sees the plain `Model` struct.

**Files.** `CMakeLists.txt`, `src/model.h/.cpp`, `src/texture.h/.cpp`, `src/material.h`,
`src/mesh.h/.cpp` (tangent and second UV attributes), `shaders/mesh.vert`,
`src/renderer.cpp` (drawing and fitting),
`shaders/common.glsl` and `shaders/hybrid_main.glsl` (texture reading), `src/main.cpp` and
`src/ui.cpp` (model menu), `third_party/stb/stb_image.h`, `assets/models/`.

**How.**

1. Getting Assimp. In CMake, `FetchContent_Declare(assimp ...)` at tag `v6.0.5`, added to the
   same `FetchContent_MakeAvailable` as the other libraries, and the viewer links
   `assimp::assimp`. Before that, a few `set(... CACHE BOOL "" FORCE)` lines keep the build small:
   `BUILD_SHARED_LIBS OFF` (static library, no DLL to copy),
   `ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF` with `ASSIMP_BUILD_GLTF_IMPORTER`,
   `ASSIMP_BUILD_OBJ_IMPORTER` and `ASSIMP_BUILD_FBX_IMPORTER` ON, `ASSIMP_NO_EXPORT ON`, and
   `ASSIMP_BUILD_TESTS`, `ASSIMP_BUILD_ASSIMP_TOOLS`, `ASSIMP_BUILD_SAMPLES`, `ASSIMP_INSTALL` and
   `ASSIMP_WARNINGS_AS_ERRORS` OFF. `ASSIMP_BUILD_ZLIB ON` makes Assimp build its own copy of
   zlib (it needs it for things like compressed data in FBX files), so Mac and Linux don't need
   zlib installed; Windows does this anyway.
   Configure prints "Enabled importer formats: OBJ FBX GLTF".
   stb_image is one header in `third_party/stb`. Its code is compiled in exactly one `.cpp`
   (`texture.cpp`), which has `#define STB_IMAGE_IMPLEMENTATION` before `#include <stb_image.h>`.
   Assimp has its own renamed copy of stb inside, so the two don't clash.
2. Reading the file: `Assimp::Importer importer;` then
   `const aiScene* scene = importer.ReadFile(path, steps);` with these steps:
   - `aiProcess_Triangulate`: polygons become triangles.
   - `aiProcess_GenSmoothNormals`: makes normals if the file has none.
   - `aiProcess_JoinIdenticalVertices`: shares vertices, so the index buffer is actually used.
   - `aiProcess_CalcTangentSpace`: tangents from the texture coordinates.
   - `aiProcess_PreTransformVertices`: applies the node transforms to the vertex positions, so
     every mesh already sits where the file puts it and one model matrix draws them all.
   - `aiProcess_FlipUVs`: this one is easy to get wrong. Assimp's glTF importer turns V around
     (`v = 1 - v`) so that V counts from the bottom, like most other formats. FlipUVs turns it
     back, so we get glTF's own UVs, which count from the top of the image.

   The scene is owned by the importer and freed with it, so copy what you need before the
   importer goes away.
3. Error checks. `ReadFile` returns `nullptr` when the file is missing or broken, and
   `importer.GetErrorString()` says why (for example "Unable to open file ..."). Also treat a
   scene with `AI_SCENE_FLAGS_INCOMPLETE` in `scene->mFlags` or without `mRootNode` as a failure.
   Assimp doesn't always fill in the error text, so fall back to a short message of your own.
   We print the message, show it in the panel and keep drawing the sphere.
4. Geometry. For each `scene->mMeshes[i]`:
   - `mVertices`, `mNormals`, `mTextureCoords[0]` and `mTextureCoords[1]` (the first two UV
     sets), `mTangents` and `mBitangents`, all `mNumVertices` long. Check `HasNormals()`,
     `HasTextureCoords(0)`, `HasTextureCoords(1)` and `HasTangentsAndBitangents()` first.
   - `mFaces[f].mIndices`, three per face after Triangulate. Faces with fewer corners are points
     or lines, so we skip them, and a mesh with no triangles at all is left out.
   - `mMaterialIndex` says which `scene->mMaterials[...]` the mesh uses.
   - Our vertex has 14 floats: position, normal, UV, a tangent as `vec4`, and the second UV
     (attributes 0 to 4). The tangent's fourth number is +1 or -1 and says on which side the
     bitangent is, the way glTF stores tangents: `dot(cross(normal, tangent), bitangent) < 0 ? -1 : 1`.
   - glTF lets each texture pick a UV set. The viewer keeps sets 0 and 1; a texture that asks
     for a higher set prints a warning and uses set 0. When a mesh has only one set, copy it into
     the second one, so the shader can always read both. The sphere does the same.
   - While copying positions, grow a box (`boundsMin`, `boundsMax`) around all meshes. It's
     used for fitting in step 8.
5. Material. Read the values with `material->Get(key, value)`, which returns `AI_SUCCESS` when
   the key is there:
   - `AI_MATKEY_BASE_COLOR` (an `aiColor4D`), else `AI_MATKEY_COLOR_DIFFUSE` for OBJ and FBX.
   - `AI_MATKEY_METALLIC_FACTOR`, `AI_MATKEY_ROUGHNESS_FACTOR`. OBJ and FBX files often don't have
     them, so start from metallic 0 and roughness 0.5 (a plain non-metal).
   - Also loaded for later: `AI_MATKEY_COLOR_EMISSIVE`, `AI_MATKEY_GLTF_ALPHAMODE`,
     `AI_MATKEY_GLTF_ALPHACUTOFF`, `AI_MATKEY_TWOSIDED`, the normal scale and occlusion strength
     (`AI_MATKEY_GLTF_TEXTURE_SCALE` and `AI_MATKEY_GLTF_TEXTURE_STRENGTH`, from
     `<assimp/GltfMaterial.h>`).

   Textures come from `material->GetTexture(type, 0, &name)`. The types for glTF are
   `aiTextureType_BASE_COLOR` (fall back to `aiTextureType_DIFFUSE` for other formats),
   `aiTextureType_GLTF_METALLIC_ROUGHNESS`, `aiTextureType_NORMALS`,
   `aiTextureType_LIGHTMAP` (Assimp files glTF's occlusion texture under "lightmap") and
   `aiTextureType_EMISSIVE`. The name is either a file next to the model, or `"*0"`, `"*1"` and so
   on for images inside the file (as in `.glb`). For those, `scene->GetEmbeddedTexture(name)` gives
   an `aiTexture`: when `mHeight == 0` it's a compressed PNG or JPEG of `mWidth` bytes in `pcData`
   for `stbi_load_from_memory`; otherwise it's plain pixels in blue, green, red, alpha order.
   Any other name is a file path. In glTF files it is written like a web address, so first turn
   escapes such as `%23` back into the character (`#`); OBJ and FBX names are used as they are.
   Join it to the model's folder with `std::filesystem::path`
   (`folder / name`), but use it as it is when it already starts at a drive or the top folder
   (`has_root_path()`), as some OBJ and FBX files have.

   Each texture also says how to read it. We keep these in a `MaterialTexture` (id, UV set and
   `TextureSettings`):
   - `AI_MATKEY_UVWSRC(type, 0)`: which UV set (glTF's `texCoord`). We keep 0 and 1, anything
     higher falls back to 0.
   - `AI_MATKEY_MAPPINGMODE_U(type, 0)` and `_V`: wrap along U and V. Assimp gives
     `aiTextureMapMode_Wrap`, `_Clamp` or `_Mirror`, which become `GL_REPEAT`,
     `GL_CLAMP_TO_EDGE` and `GL_MIRRORED_REPEAT` (`_Decal`, from other formats, is treated as
     clamp).
   - `AI_MATKEY_GLTF_MAPPINGFILTER_MAG(type, 0)` and `_MIN` (from `<assimp/GltfMaterial.h>`): the
     filters. glTF stores them as the OpenGL numbers (9728 is `GL_NEAREST`, 9729 `GL_LINEAR` and
     so on), so they can be used as they are. They are only there when the file sets them;
     otherwise keep `GL_LINEAR_MIPMAP_LINEAR` and `GL_LINEAR`.

   Keep a map from name plus these settings to texture id, so an image used several times the
   same way loads once. In OpenGL 4.1 the wrap, the filter and the format (sRGB or plain) live in
   the texture itself, so the same image with other settings becomes a second texture. If the base color or
   metallic-roughness texture has a texture transform (`AI_MATKEY_UVTRANSFORM`, from the
   KHR_texture_transform extension), we print a warning, because the viewer doesn't apply it yet.

   One surprise: the glTF importer adds one default material after the materials it imported
   (it only imports the ones the meshes use). A mesh whose glTF primitive has no material uses
   it, so it gets glTF's default values (white, metallic 1, roughness 1).
6. Upload in `texture`:
   - `stbi_load(path, &w, &h, &channels, 4)` (4 asks for RGBA every time), then `glTexImage2D` with
     internal format `GL_SRGB8_ALPHA8` for color textures or `GL_RGBA8` for data textures,
     `glGenerateMipmap(GL_TEXTURE_2D)`, the min and mag filter and the wrap S and T from the
     texture's settings (`glTexParameteri`), and `stbi_image_free`.
   - Base color and emissive textures hold colors, so they are sRGB; metallic-roughness, normal
     and occlusion hold data and stay `GL_RGBA8`. The `srgb` flag in `TextureSettings` carries
     the choice, and it is part of the cache key above. See "HDR and color space" for why.
   - Don't flip the images. Their first row is the top, and with FlipUVs the UVs count from the
     top too, so the two match.
   - A missing texture gets a shared 1 x 1 white texture, so texture times factor is just the
     factor and the shader has one code path. The normal map is the exception: it stays 0,
     because white would tilt the normals.
7. Shader. `mesh.vert` passes both UV sets on as `texCoord` and `texCoord1`. In
   `hybrid_main.glsl` (samplers and uniforms declared in `common.glsl`, base color on texture
   unit 0 and metallic-roughness on unit 1), each texture picks its set with a small uniform,
   `uBaseColorUVSet` and `uMetallicRoughnessUVSet`, set per part from the material:
   - `baseColorUV = (uBaseColorUVSet == 1) ? texCoord1 : texCoord`, same for metallic-roughness
   - `baseColor = texture(uBaseColorTexture, baseColorUV).rgb * uBaseColor`
   - `roughness = texture(uMetallicRoughnessTexture, metallicRoughnessUV).g * uRoughness`
   - `metallic = texture(uMetallicRoughnessTexture, metallicRoughnessUV).b * uMetallic`
8. Fit the model on screen (`fitModelMatrix` in `renderer.cpp`): from the box, scale by
   `2 / largest side`, move the middle of the box's bottom to the origin and put it on the grid
   floor. The scale is the same on every axis, so the normals keep their directions. The sphere
   is drawn as a one-part model with white textures and the panel's material values.

Good to know: Assimp 6.0.5 has two small gaps with glTF files that we decided to live with.
Some compression tools store texture coordinates as packed whole numbers instead of decimals;
Assimp doesn't unpack them, so such a model reads its textures from the wrong place. And a number
written with a decimal point where glTF expects a whole number (a filter of `9728.0` instead of
`9728`) is skipped, so the default is used. Most files don't do either, and a newer Assimp may
fix them.

---

## Test materials (Both)

**What gets done.** Five loose texture sets (cloth, leather, metal, stone, wood) that can be put on
the sphere from a menu.

**Files.** `src/texture.cpp`, `src/material.h`, `src/ui.cpp`, `assets/`, `assets/CREDITS.md`.

**How.**

1. Choose sets with a CC0 or CC BY license, 1K or 2K, and add each one's line to `CREDITS.md` in
   the same commit. Check the license on the download page itself.
2. Load: `stbi_load(path, &w, &h, &channels, 4)`, upload, `stbi_image_free`.
3. Pack into the glTF layout. Loose sets come with separate roughness and metallic images. Build
   one RGBA image where green = roughness and blue = metallic. If a set has no metallic image,
   fill blue with 0 (or 255 for a pure metal).
4. Orientation. Decide once whether loose images need `stbi_set_flip_vertically_on_load(true)` for
   the sphere's UVs, by loading a texture with readable text on it.
5. `uUvScale` uniform to repeat the texture across the sphere.
6. UI: `ImGui::Combo` over the list of materials. Selecting one swaps the texture ids in the
   sphere's `Material`.

---

## Ramp diffuse and stylized highlight (NPR)

**What gets done.** The real stylized diffuse and specular replace the stand-ins from the term
skeleton. The diffuse falls into flat color bands read from a ramp image, and the highlight is a
flat spot whose size follows roughness through the same GGX shape the PBR highlight uses.

**Files.** `shaders/npr_terms.glsl`, `src/main.cpp` (loads the ramps), `src/texture.h` and
`src/texture.cpp` (a texture can skip mipmaps), `src/renderer.cpp` (binds the ramp),
`src/settings.h`, `src/ui.cpp`, `assets/ramps/`, `assets/CREDITS.md`.

**How.**

1. Ramp images: `assets/ramps/two-band.png` (0.3, then 1.0 from t = 0.55) and
   `assets/ramps/three-band.png` (0.25, then 0.6 from t = 0.55, then 1.0 from t = 0.75). Both are
   256 x 1 gray PNGs. The numbers are linear brightness, stored sRGB-encoded in the image (bytes
   149 and 255; 137, 203 and 255), so the shader reads back 0.3 and so on.
2. Ramp texture: loaded through `loadTextureFile` with `GL_SRGB8_ALPHA8` (`srgb = true`), wrap
   `GL_CLAMP_TO_EDGE` on both axes, `GL_NEAREST` for both filters, and no mipmaps
   (`mipmaps = false`). It is bound on texture unit 5 every frame, together with the sampler
   uniform `uRampTexture`, so a shader reload with `R` keeps it. A ramp file that cannot be read
   is replaced by a white texture and the console says so.
3. `diffuseStylized` (Lake et al. 2000; the `N.L * 0.5 + 0.5` remap is Valve's Half Lambert,
   used this way in Team Fortress 2):
   - `t = (dot(N, L) * 0.5 + 0.5) * shadow`, with N.L not clamped, so the side facing away from
     the light uses the part of the ramp below 0.5
   - `rampLookup(t) = texture(uRampTexture, vec2(t, 0.5)).rgb`
   - `result = base / pi * rampLookup(t) * radiance`

   The base color is used as it is, also for metals. The shadow only changes `t`, which is what
   Shadow in the ramp needs later.
4. `specularStylized`, the GGX term (Walter et al. 2007, `a = roughness^2` as in glTF) divided by
   its own peak, which is `1 / (pi * a^2)` at N.H = 1:
   - `r = max(roughness, 0.2)`, `a = r * r` (this floor is only used here)
   - `d = (N.H)^2 * (a^2 - 1) + 1`
   - `ratio = min(a^4 / d^2, 1)`. This is 1 at the center of the highlight and falls toward 0.
     It can not go above 1, but rounding in `a^2 - 1` and the `+ 1` can push it just over, which
     would leave a tiny dot at Threshold 1, so it is kept at 1 at most.
   - `w = max(0.7 * length(vec2(dFdx(ratio), dFdy(ratio))), 0.0001)`,
     `mask = smoothstep(T - 2w(1 - T), T + 2wT, ratio)` with `T = uHighlightThreshold`. At
     T = 0.5 this is `smoothstep(T - w, T + w, ratio)`, Gustavson's `aastep`: `dFdx` and `dFdy` say how much `ratio` changes to the next pixel across and down,
     so the edge is about 1.4 pixels soft in every direction and not jagged. The small minimum is
     there because `smoothstep` is undefined when both edges are equal, which happens at
     roughness 1, where `ratio` is 1 everywhere.
   - `tint = mix(vec3(1), base, metallic)`
   - `fade = smoothstep(0, 2 * wNL, N.L)`, with `wNL` worked out from N.L the same way as `w`.
     It is 0 where N.L <= 0 (light behind the surface) and fades in over about 1.4 pixels, so the
     highlight does not end in a jagged arc where it meets the dark side.
   - `result = mask * fade * uHighlightStrength * (1 - roughness) / pi * tint * radiance * shadow`
   - If "Follows ramp bands" is ticked, `result *= rampLookup(t)`, the same ramp value the
     diffuse uses at this pixel.
   - `dFdx` and `dFdy` are undefined inside an `if` that only some pixels take, so they come first.
     The only `if` is on the "Follows ramp bands" uniform, which is the same for every pixel.
5. Why the highlight grows with roughness: the mask is on where
   `(N.H)^2 >= (1 - a^2 / sqrt(T)) / (1 - a^2)`. A larger `a` lowers the right side. With
   T = 0.5 the edge sits this far from the center (angle between N and H): 0.37 degrees at
   roughness 0.1 (without the floor, so never used), 1.5 at the 0.2 floor (the smallest spot),
   3.3 at 0.3, 9.6 at 0.5, 21 at 0.7 and 63 at 0.9. At roughness 1 the mask covers the whole lit
   side, but the brightness `(1 - roughness)` is 0 there, so nothing shows.
6. Panel, under "NPR shading": Ramp (Two bands, Three bands), and for the highlight Threshold
   (default 0.5), Strength (default 1) and the "Follows ramp bands" tick (default off). Like every
   control in the panel, each has a reset arrow while it differs from its default, and while
   anything in the section differs, the section header has one that resets the whole section.

**Choices and why** (for the slides):

- **Ramp image instead of bands in code.** Rounding N.L to steps or cutting it with smoothstep
  fixes the bands in the shader. With an image, the number, position and color of the bands
  change by swapping a file. This is the method of Lake et al. and of Team Fortress 2.
- **`N.L * 0.5 + 0.5` instead of `max(N.L, 0)`.** Lake et al. clamp N.L, which gives the whole
  dark side one value, so no band can sit past the line between light and dark and a cast shadow
  could not look different from the dark side. The remap keeps that line at t = 0.5. Valve squares
  it (Half Lambert), which bends the bands toward the light; we don't need that, the image can put
  the bands anywhere.
- **Times base / pi times radiance.** Team Fortress 2 also multiplies its ramp result by the
  albedo. With the same base / pi factor as the PBR diffuse, a white ramp on a non-metal facing
  the light is as bright as the PBR side (within about 4 percent), so the Hybrid view blends two
  pictures of about the same brightness, and textures show through. Metals differ: the PBR
  diffuse of a metal is 0, the toon one is not.
- **Band values and edges.** Our own picks. The first edge at t = 0.55 (N.L = 0.1, about 84
  degrees from the light) sits a little inside the PBR terminator, so the shadow map's unreliable
  zone later falls in the dark band. Gray ramps keep the base color readable.
- **Nearest, clamp to edge, no mipmaps.** Nearest keeps each edge one texel sharp; linear would
  soften it a little. Repeat wrap would let the lit end bleed into the dark end near t = 1, and
  mipmaps would blur the bands far away.
- **Metals keep their base color in the toon diffuse.** Physically a metal has no diffuse color,
  but a black toon metal looks wrong. The metal look comes from the tinted highlight instead.
- **GGX over its peak, cut at a threshold.** The usual toon highlight cuts Blinn-Phong, whose size
  follows a shininess number and not our roughness. Dividing GGX by its peak is our own idea. At
  T = 0.5 the toon edge is where this GGX shape has fallen to half its peak, at every roughness,
  so the toon spot follows the same GGX shape as the PBR highlight. The two are not exactly the
  same size: the PBR highlight also has Fresnel, the visibility term and N.L, which change it
  near grazing light, and the toon one has the 0.2 floor.
- **Roughness floor 0.2 in the toon highlight.** Below about 0.15 the spot would be smaller than a
  pixel. Smooth toon highlights come out a little larger than the PBR ones.
- **Brightness `(1 - roughness)`.** Our own rule. Without it a fully rough sphere turns white over
  its whole lit half. Rougher is dimmer, like on the PBR side, though much more gently.
- **Tint `mix(1, base, metallic)`.** Like the PBR side's F0, it turns into the base color at the
  metal end: gold gets a gold highlight, plastic a white one. The PBR F0 starts from 0.04 for
  non-metals, the toon tint from white, so in between they differ (white base, metallic 0.5:
  F0 0.52, tint 1).
- **Soft edge with Gustavson's `aastep`.** A hard `step` gives jagged, crawling edges, and a
  fixed soft width gets blurry up close. The common shortcut `fwidth` (`|dFdx| + |dFdy|`) gives
  edges 2 pixels wide on straight edges and up to 2.8 on diagonals; the length of the two
  derivatives gives about 1.4 pixels in every direction, for one square root per pixel.
- **The soft window slides with the threshold.** Our own construction. With a centered window,
  Threshold 1 still left a faint one-pixel dot at the peak and Threshold 0 left a soft glow on
  smooth surfaces. Sliding the window keeps it centered at 0.5, so 1 means no highlight and 0
  means the whole lit side, flat.
- **"Follows ramp bands", off by default.** Our own switch. Off, the spot is one clean shape that
  ends only at N.L = 0, the same shape as the PBR highlight, so the Hybrid mix stays consistent.
  On, a large spot that reaches past a band edge dims to that band's value, like painted toon
  art, which leaves a step inside the spot at each band edge. It only matters for large spots
  (roughness about 0.6 and up) with the light from the side.
- **Fade at the dark side.** The same soft edge at N.L = 0 instead of a hard cut. The PBR side
  also goes smoothly to 0 there, through its N.L factor.

---

## Cook-Torrance (PBR)

**What gets done.** The real physical diffuse and specular replace the stand-ins from the term
skeleton. We follow the Khronos glTF Sample Viewer (its renderer code), so our PBR view can be
compared with it. D and F are written as in the glTF 2.0 specification (Appendix B). The
visibility term is the height-correlated one that the specification mentions as more accurate
(Heitz 2014). The diffuse weight comes from the specification's blend of a non-metal and a metal,
done the way the viewer does it, with the non-metal's own Fresnel.

**Files.** `shaders/pbr_terms.glsl`. The sphere grid is built in `src/main.cpp`
(`createSphereGridModel`), and `createSphere` in `src/mesh.cpp` takes a center and a radius for it.

**How.** With `a = roughness * roughness`, roughness clamped to at least 0.05 inside these terms
only (the roughness debug view still shows the value as stored):

1. Distribution, GGX: `D = a^2 / (pi * ((N.H)^2 * (a^2 - 1) + 1)^2)`
2. Visibility, the height-correlated Smith term (Heitz 2014, as in the Khronos viewer):
   `Vis = 0.5 / (N.L * sqrt((N.V)^2 * (1 - a^2) + a^2) + N.V * sqrt((N.L)^2 * (1 - a^2) + a^2))`.
   It is the geometry term G already divided by `4 * N.L * N.V`, so there is no separate divisor.
   The shader multiplies N.L on before dividing (`0.5 * N.L / ...`) and returns 0 if the bottom
   is 0, so tiny N.L and N.V cannot overflow.
3. Fresnel, Schlick: `F = F0 + (1 - F0) * (1 - V.H)^5`. The specular uses
   `F0 = mix(vec3(0.04), base, metallic)`.
4. `specularPhysical = D * Vis * F * radiance * N.L * shadow`
5. `diffusePhysical = (1 - F_dielectric) * (1 - metallic) * base / pi * radiance * N.L * shadow`,
   where `F_dielectric` is the same Schlick formula with `F0 = 0.04`.

   `(1 - metallic)` removes the diffuse color from metals. `(1 - F_dielectric)` takes away the
   light the paint reflects at its surface, so it is not counted twice. Using the mixed F here
   instead would also take away what the metal part reflects: at metallic 0.5 that leaves about
   half the diffuse it should have.
6. No light when N.L is 0 or below, and no highlight when N.V is 0 or below. Clamp N.H and V.H
   to 0 to 1, because `pow` of a negative number is undefined in GLSL. When L and V point exactly
   opposite ways there is no half vector; the shader then uses a zero vector (V.H = 0). That only
   happens when the light or the camera is behind the surface, where there is no highlight.
7. The HDR picture stores half floats, which stop at 65504. A very smooth surface seen at a
   grazing angle can go past that, so the specular is capped a little below it.
8. Sphere grid for the check: "Sphere grid" in the Model menu. 7 x 7 spheres, metallic from 0
   (bottom row) to 1 (top row) and roughness from 0 (left) to 1 (right), color from the Base color
   swatch. To compare with the picture on LearnOpenGL's PBR lighting page, set the swatch to a
   dark red, about 191, 0, 0 (the swatch is sRGB, and that is their linear albedo of
   (0.5, 0, 0)). Their demo has four point lights, and its metallic and roughness stop at 6/7, so
   the top row and right column differ a little.

**Choices and why** (for the slides):

- **Distribution.** GGX instead of the older Beckmann distribution. GGX has a longer tail, so
  highlights fade out more softly, and Walter et al. found it fits measured materials better.
- **Geometry term.** We use the height-correlated Smith form (Heitz 2014), which the Khronos
  glTF Sample Viewer uses. The glTF specification writes the simpler separable Smith form and
  names the height-correlated one as more accurate. LearnOpenGL uses the separable Smith with
  Schlick-GGX and `k = (roughness + 1)^2 / 8`, a remap from Disney that Karis used in Unreal
  Engine 4 for direct lights so rough surfaces do not get too bright. That version is 12 to 34
  percent darker at middle angles and up to 80 percent darker near the edge of the sphere. Ours
  matches the Khronos viewer we compare against, for the cost of two square roots.
- **Fresnel.** Schlick's approximation instead of the exact Fresnel equations. For a non-metal
  (index about 1.5) the two differ by up to about 3.6 points of reflectance near grazing angles,
  and for metals the exact form needs a complex index of refraction per color, which glTF
  materials do not store.
- **Diffuse.** Lambert instead of Burley (Disney) or Oren-Nayar. Those are other fits to measured
  data and are not clearly more accurate, and glTF uses Lambert too.
- **Smallest roughness, 0.05.** At roughness 0 the highlight of a single light is one point of
  endless brightness and the formulas divide by zero. 0.05 is the low end of LearnOpenGL's grid.
- **Diffuse weight.** `(1 - F_dielectric) * (1 - metallic)` as in the Khronos viewer, instead of
  the shorter `(1 - F) * (1 - metallic)` with the mixed F (LearnOpenGL, and the short form at the
  end of glTF Appendix B.3.5). They are the same for non-metals and full metals, but in between
  the short form also removes the metal's reflection from the paint: at metallic 0.5 it gives
  about half the diffuse for a light base color.
- **The two sides are almost, not exactly, equal.** With the `(1 - F)` weight a white sphere
  facing the light reads about 250 in the Diffuse view instead of 255, and about 184 instead of
  188 where it is half lit. The toon diffuse has no Fresnel, so there it stays at 255.

---

## Shadow map (Both)

**What gets done.** A depth image drawn from the light, and a shared function that turns it into
`visibility` from 0 (in shadow) to 1 (lit). A ground plane to catch the shadow.

**Files.** `src/framebuffer.cpp`, `shaders/shadow_depth.vert`, `shaders/shadow_depth.frag`,
`shaders/common.glsl`, `shaders/mesh.vert`, `src/renderer.cpp`, `src/mesh.cpp` (plane).

**How.**

1. Depth target:
   - `glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL)`
   - Filters `GL_NEAREST`. Wrap `GL_CLAMP_TO_BORDER` with
     `glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, white)`, so anything outside the
     map counts as lit.
   - `glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0)`,
     `glDrawBuffer(GL_NONE)`, `glReadBuffer(GL_NONE)`
2. Light matrix, with `center` and `radius` of the scene's bounding sphere:
   - `lightView = glm::lookAt(center + L * 2 * radius, center, up)`
   - `lightProj = glm::ortho(-radius, radius, -radius, radius, 0.1f, 4 * radius)`
   - `uLightMatrix = lightProj * lightView`
3. Shadow pass: bind the depth target, `glViewport(0, 0, 2048, 2048)`,
   `glClear(GL_DEPTH_BUFFER_BIT)`, draw everything with `shadow_depth.vert`
   (`gl_Position = uLightMatrix * uModel * vec4(position, 1)`) and an empty fragment shader.
4. `mesh.vert` also outputs `lightSpacePos = uLightMatrix * worldPos`.
5. Lookup in `common.glsl`:
   - `p = lightSpacePos.xyz / lightSpacePos.w * 0.5 + 0.5`
   - if `p.z > 1` return 1
   - `bias = max(0.005 * (1 - N.L), 0.0005)`
   - `texel = 1.0 / textureSize(uShadowMap, 0)`
   - average over the 9 offsets x, y in -1, 0, 1 of
     `(p.z - bias <= texture(uShadowMap, p.xy + vec2(x, y) * texel).r) ? 1.0 : 0.0`
6. `shadowPhysical(vis)` returns `vis` unchanged.

---

## Shadow in the ramp (NPR)

**What gets done.** On the stylized side the shadow has a hard edge and lands in the same color
bands as the shading.

**Files.** `shaders/npr_terms.glsl`, `shaders/hybrid_main.glsl`, `shaders/common.glsl` (bias),
`src/ui.cpp`.

**How.**

1. `shadowStylized(vis) = smoothstep(T - 0.02, T + 0.02, vis)` with `T = uShadowThreshold`.
2. The ramp lookup from ramp shading already multiplies `t` by the shadow, so a shadowed pixel reads the
   dark end of the ramp. Nothing is multiplied on afterward.
3. Removing speckles, in this order:
   - Normal offset: before the light transform, move the position along the normal,
     `worldPos + N * offset`, with an offset of about one or two shadow map texels in world units.
   - Draw the shadow pass with `glCullFace(GL_FRONT)`, then switch back to `GL_BACK`.
   - Put the ramp's first step a little above t = 0.5, so the unreliable zone where the surface
     turns away from the light is already in the dark band.

**Open point.** What the shadow slider means exactly is still to be decided: edge hardness (as
written here) or moving the shadow into the ramp lookup.

---

## Environment lighting (PBR)

**What gets done.** A skybox, and the real physical ambient term: soft light from all around plus
reflections that blur with roughness.

**Files.** `src/texture.cpp` (HDR loading, cubemap), `shaders/skybox.vert`, `shaders/skybox.frag`,
`shaders/pbr_terms.glsl`, `src/mesh.cpp` (cube), `src/renderer.cpp`.

**How.**

1. Load the `.hdr` panorama: `stbi_loadf(path, &w, &h, &channels, 3)` gives linear floats.
2. Build the cubemap on the CPU, 256 x 256 per face. For each face pixel compute its direction
   `d`, then read the panorama at
   `u = atan(d.z, d.x) / (2 * pi) + 0.5`, `v = acos(d.y) / pi`.
3. Upload each face i from 0 to 5:
   `glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, data)`,
   then `glGenerateMipmap(GL_TEXTURE_CUBE_MAP)`, min filter `GL_LINEAR_MIPMAP_LINEAR`, wrap
   `GL_CLAMP_TO_EDGE`. Once at startup: `glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS)`.
4. Skybox, drawn after the model:
   - Remove the camera's position from the view matrix: `mat4(mat3(uView))`.
   - `gl_Position = (uProjection * view * vec4(position, 1)).xyww`, which puts it at maximum depth.
   - `glDepthFunc(GL_LEQUAL)` while drawing it, then back to `GL_LESS`.
   - Fragment: `texture(uEnv, direction).rgb`.
5. `ambientPhysical`, with `maxMip = log2(size)`:
   - `diffuseEnv = textureLod(uEnv, N, maxMip - 1).rgb`, a very blurred sample
   - `specEnv = textureLod(uEnv, R, roughness * maxMip).rgb`
   - `F = F0 + (max(vec3(1 - roughness), F0) - F0) * (1 - N.V)^5`
   - `result = (1 - F) * (1 - metallic) * base * diffuseEnv + F * specEnv`
6. A slider for environment intensity.

**Open point.** Reflections are part of the ambient term here, so the specular slider does not
affect them. Still to be decided.

---

## Hybrid polish (NPR, then Both)

**What gets done.** The final stylized ambient, preset buttons, labels on the three views, and
every debug view working.

**Files.** `shaders/npr_terms.glsl`, `shaders/hybrid_main.glsl`, `src/settings.h`, `src/ui.cpp`.

**How.**

1. NPR: `ambientStylized = uAmbientColor * s.baseColor`, with a color picker
   (`ImGui::ColorEdit3`).
2. Both: presets are a small array of `{ name, four weights }`. `ImGui::Button(name)` copies the
   weights into `Settings`.
3. Both: labels with
   `ImGui::GetForegroundDrawList()->AddText(position, color, "PBR")`, one per strip.
4. Both: go through every debug view number and confirm it shows the term it names.

---

## Reference check (PBR)

**What gets done.** A short written comparison with screenshots. No new code unless it finds a bug.

**How.** Follow "The reference check" in the work plan. Material inputs are compared one at a time
with the debug views and should match exactly. The lit result is compared by eye, and the
differences are written down with their reason.

---

## Demo and report material (Both)

**What gets done.** Final screenshots, the finished README, and a rehearsed 10 minute demo script.

**How.** Write the script as a list of steps with the preset or slider for each. Take screenshots
of the same model at pure PBR, pure NPR and each interesting mix. One of us runs the script from a
fresh clone without touching code.

---

## Sources

- LearnOpenGL, the PBR chapters (theory, lighting, IBL) and the shadow mapping chapter, for the
  Cook-Torrance, shadow map and cubemap formulas.
- Brian Karis, "Real Shading in Unreal Engine 4" (2013), for `a = roughness^2` and as background
  for the `k` remap in LearnOpenGL's geometry term (we use Heitz's visibility term instead).
- The glTF 2.0 specification, for the material layout and accessor structure, and its Appendix B
  for the Cook-Torrance formulas.
- Walter, Marschner, Li and Torrance, "Microfacet Models for Refraction through Rough Surfaces"
  (2007), where GGX comes from. The toon highlight uses it too.
- Schlick, "An Inexpensive BRDF Model for Physically-based Rendering" (1994), for the Fresnel
  approximation.
- Eric Heitz, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs" (2014), for
  the height-correlated visibility term.
- Khronos glTF Sample Renderer (the code behind the glTF Sample Viewer), `brdf.glsl` and
  `pbr.frag`, for how the visibility term and the diffuse weight are put together.
- Lake, Marshall, Harris and Blackstein, "Stylized Rendering Techniques for Scalable Real-Time 3D
  Animation" (NPAR 2000), for the ramp lookup of the toon diffuse.
- Mitchell, McTaggart and Green, "Shading in Valve's Source Engine" (SIGGRAPH 2006 course), for
  Half Lambert, and Mitchell, Francke and Eng, "Illustrative Rendering in Team Fortress 2" (NPAR
  2007), for the same remap into a ramp, multiplied by the albedo.
- Stefan Gustavson, "Procedural Textures in GLSL" (OpenGL Insights, chapter 7, 2012), for the
  `aastep` soft edge of the toon highlight.
- Krzysztof Narkowicz, "ACES Filmic Tone Mapping Curve" (2016), for the ACES fit.
