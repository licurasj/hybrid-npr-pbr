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
`shaders/phong.frag` (temporary, replaced in the term skeleton).

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
5. `phong.frag`.
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
7. Comparison view in `renderer`: for strip i from 0 to 2,
   - `glEnable(GL_SCISSOR_TEST)`
   - `glViewport(i * w / 3, 0, w / 3, h)` and `glScissor` with the same numbers
   - projection aspect is `(w / 3) / h`
   - send weights: all 0, the slider values, all 1
   - draw the model
8. UI: `ImGui::SliderFloat("Diffuse", &settings.weights[1], 0, 1)` for each weight,
   `ImGui::Combo` for view mode and debug view.

---

## HDR and color space (Both)

**What gets done.** The scene is drawn into a float texture, then a last pass applies exposure, a
tone curve and the sRGB conversion.

**Files.** `src/framebuffer.h/.cpp`, `shaders/fullscreen.vert`, `shaders/tonemap.frag`,
`src/renderer.cpp`, `src/settings.h` (exposure, tone curve choice), `src/ui.cpp`.

**How.**

1. HDR target in `framebuffer`:
   - `glGenFramebuffers`, `glBindFramebuffer(GL_FRAMEBUFFER, fbo)`
   - Color: `glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL)`,
     filters `GL_LINEAR`, then
     `glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0)`
   - Depth: `glGenRenderbuffers`, `glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h)`,
     `glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo)`
   - `glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE`
   - Recreate the textures when the window size changes.
2. Frame order: bind the HDR target, draw the scene, bind framebuffer 0, draw the tone mapping
   pass, then ImGui.
3. `fullscreen.vert`, one triangle that covers the screen with no vertex buffer (a vertex array
   must still be bound):
   - `vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);`
   - `uv = p; gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);`
   - Draw with `glDrawArrays(GL_TRIANGLES, 0, 3)`.
4. `tonemap.frag`: `c = texture(uHdr, uv).rgb * uExposure`, then one of
   - Off: `clamp(c, 0, 1)`
   - Reinhard: `c / (1 + c)`
   - ACES fit: `(c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14)`, clamped to 0..1

   then linear to sRGB per channel:
   `c <= 0.0031308 ? 12.92 * c : 1.055 * pow(c, 1.0 / 2.4) - 0.055`.
5. Keep `GL_FRAMEBUFFER_SRGB` disabled.
6. Why the check numbers work: a white sphere with light intensity pi gives
   `1 / pi * 1 * pi = 1`, which is 255. At N.L = 0.5 the linear value 0.5 becomes 0.735 after the
   sRGB conversion, which is 188.

---

## glTF loading (Both)

**What gets done.** A `.gltf` or `.glb` file becomes a list of mesh and material pairs that the
renderer can draw.

**Files.** `src/model.h/.cpp`, `src/texture.h/.cpp`, `src/material.h`, `src/mesh.cpp`,
`shaders/common.glsl` (texture reading).

**How.**

1. In exactly one `.cpp`, define `TINYGLTF_IMPLEMENTATION`, `STB_IMAGE_IMPLEMENTATION` and
   `STB_IMAGE_WRITE_IMPLEMENTATION` before including `tiny_gltf.h`. tinygltf uses stb_image
   itself, so stb must not be implemented a second time elsewhere.
2. Load: `tinygltf::TinyGLTF loader; tinygltf::Model model;` then
   `loader.LoadASCIIFromFile(&model, &err, &warn, path)` for `.gltf` or
   `loader.LoadBinaryFromFile(...)` for `.glb`. Print `warn` and `err`.
3. Geometry. For each `model.meshes[m].primitives[p]`:
   - `primitive.attributes` maps `"POSITION"`, `"NORMAL"`, `"TEXCOORD_0"` to an accessor index.
   - `accessor = model.accessors[index]`, `view = model.bufferViews[accessor.bufferView]`,
     `buffer = model.buffers[view.buffer]`.
   - First byte: `buffer.data.data() + view.byteOffset + accessor.byteOffset`.
     Step between elements: `accessor.ByteStride(view)`. Number of elements: `accessor.count`.
   - Indices: `primitive.indices` is another accessor. Its `componentType` can be unsigned byte
     (5121), unsigned short (5123) or unsigned int (5125). Convert all to 32-bit.
4. Position in the scene: walk `model.scenes[model.defaultScene].nodes` and their children.
   A node has either `matrix` (16 numbers) or `translation`, `rotation`, `scale`. Multiply parent
   by child and store the result as the mesh's model matrix.
5. Material. `model.materials[primitive.material].pbrMetallicRoughness` has `baseColorFactor`,
   `baseColorTexture.index`, `metallicFactor`, `roughnessFactor`,
   `metallicRoughnessTexture.index`. A texture index leads to
   `model.textures[i].source`, then `model.images[source]` with `width`, `height`, `component`
   and the pixel bytes in `image`.
6. Upload in `texture`:
   - Base color: internal format `GL_SRGB8_ALPHA8`. Metallic-roughness: `GL_RGBA8`.
   - `glTexImage2D`, `glGenerateMipmap(GL_TEXTURE_2D)`, min filter `GL_LINEAR_MIPMAP_LINEAR`,
     wrap `GL_REPEAT`.
   - Do not flip glTF images. Their first row is the top, and glTF UVs count from the top too,
     so the two cancel out.
   - A material with no texture gets a 1 x 1 white texture, so the shader has one code path.
7. Shader, in `common.glsl`:
   - `base = texture(uBaseColorTex, uv).rgb * uBaseColorFactor.rgb`
   - `roughness = texture(uMetalRoughTex, uv).g * uRoughnessFactor`
   - `metallic = texture(uMetalRoughTex, uv).b * uMetallicFactor`
8. Fit the model on screen: read the position accessor's `minValues` and `maxValues`, then center
   and scale.

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

**What gets done.** The real stylized diffuse and specular replace the stand-ins from the term skeleton.

**Files.** `shaders/npr_terms.glsl`, `src/texture.cpp` (ramp), `src/settings.h`, `src/ui.cpp`,
`assets/ramps/`.

**How.**

1. Ramp texture: a small image, for example 256 x 1. Internal format `GL_SRGB8_ALPHA8`, wrap
   `GL_CLAMP_TO_EDGE`, no mipmaps. `GL_NEAREST` gives hard bands, `GL_LINEAR` slightly soft ones.
2. `diffuseStylized`:
   - `t = (dot(N, L) * 0.5 + 0.5) * shadow`
   - `rampColor = texture(uRamp, vec2(t, 0.5)).rgb`
   - `result = s.baseColor / pi * rampColor * l.radiance`

   The remap puts N.L = 0 (the edge between lit and unlit) at t = 0.5.
3. `specularStylized`, the GGX term divided by its own peak:
   - `a = roughness * roughness`, kept above about 0.02
   - `d = (N.H)^2 * (a^2 - 1) + 1`
   - `ratio = a^4 / d^2`. This is 1 at the center of the highlight and falls toward 0.
   - `edge = fwidth(ratio)`, `mask = smoothstep(T - edge, T + edge, ratio)` with threshold
     `T = uHighlightThreshold`. `fwidth` keeps the edge one pixel soft, so it is not jagged.
   - `tint = mix(vec3(1), s.baseColor, s.metallic)`
   - `result = mask * uHighlightStrength * tint * l.radiance * step(0.0, dot(N, L)) * shadow`
4. Why the highlight grows with roughness: the mask is on where
   `(N.H)^2 >= (1 - a^2 / sqrt(T)) / (1 - a^2)`. A larger `a` lowers the right side.
5. Sliders: threshold, strength, ramp choice.

**Open points.** At roughness above about 0.7 the highlight covers much of the sphere. If it looks
wrong, scale the strength by `(1 - roughness)`. Whether a stylized metal shows its base color in
the diffuse is still to be decided.

---

## Cook-Torrance (PBR)

**What gets done.** The real physical diffuse and specular replace the stand-ins from the term skeleton.

**Files.** `shaders/pbr_terms.glsl`. A sphere grid for the check in `src/renderer.cpp` or
`main.cpp`.

**How.** With `a = roughness * roughness` and roughness kept above about 0.04:

1. Distribution, GGX: `D = a^2 / (pi * ((N.H)^2 * (a^2 - 1) + 1)^2)`
2. Geometry, Smith with Schlick-GGX: `k = (roughness + 1)^2 / 8`,
   `G1(x) = x / (x * (1 - k) + k)`, `G = G1(N.L) * G1(N.V)`
3. Fresnel, Schlick: `F0 = mix(vec3(0.04), base, metallic)`,
   `F = F0 + (1 - F0) * (1 - V.H)^5`
4. `specularPhysical = D * G * F / (4 * N.L * N.V + 0.0001) * radiance * N.L * shadow`
5. `diffusePhysical = (1 - F) * (1 - metallic) * base / pi * radiance * N.L * shadow`

   The factor `(1 - F) * (1 - metallic)` is what removes the diffuse color from metals and keeps
   energy from being counted twice.
6. Clamp N.L, N.V, N.H and V.H to 0 or above with `max`.
7. Sphere grid for the check: 7 x 7 spheres, metallic 0 to 1 on one axis and roughness 0.05 to 1
   on the other, set with the uniform factors and a white 1 x 1 texture.

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
- Brian Karis, "Real Shading in Unreal Engine 4" (2013), for `a = roughness^2` and the `k` in the
  geometry term.
- The glTF 2.0 specification, for the material layout and accessor structure.
- Krzysztof Narkowicz, "ACES Filmic Tone Mapping Curve" (2016), for the ACES fit.
