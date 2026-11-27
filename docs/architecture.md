# Architecture overview

How the viewer is put together, written so that either of us can explain it out loud. This is the
planned design. Update it when the code changes shape.

Terms the course did not cover are explained in the [glossary](#glossary) at the end.

## 1. The idea in one paragraph

The program loads a model, then draws it every frame in three passes: a shadow pass, a lighting
pass and a tone mapping pass. All of the interesting work is in the lighting pass, in one fragment
shader. That shader computes every lighting term twice, once the physical way and once the stylized
way, and then mixes the two results with a weight per term. Pure PBR is all weights at 0, pure NPR
is all weights at 1, and the hybrid is whatever the sliders say. The side-by-side view is the same
scene drawn three times with three sets of weights.

## 2. Modules

Each module is one `.h` and `.cpp` pair in `src/`. There is no scene graph, no entity system and no
material class hierarchy.

| Module | Responsible for | Hands on |
|---|---|---|
| `main.cpp` | Creates the window, loads everything once, runs the frame loop | Calls the modules below in order |
| `settings.h` | One plain struct with every value the UI can change: blend weights (five terms plus tone mapping), light direction, exposure, tone curve and the two tone mapping ticks, ramp choice and toon highlight threshold, strength and the follows-ramp tick, environment choice, background, rotation, intensity and method, view mode, debug view | Read by the renderer, written by the UI |
| `camera` | Orbit camera: turns mouse input into a view matrix and a projection matrix | Two matrices to the renderer |
| `shader` | Reads GLSL files, joins them, compiles, links, reports errors, sets uniforms, reloads on request | A program id |
| `texture` | Loads images with stb_image and creates GPU textures with the right color space. Loads the `.hdr` environment image and turns it into a cubemap | Texture ids |
| `environment` | Builds the environment lighting maps from the cubemap (irradiance and prefiltered maps) and the BRDF lookup table, with small fragment-shader passes | Texture ids |
| `mesh` | Holds one vertex array (positions, normals, UVs) and draws it. Can generate a sphere and the cube for the sky | A drawable mesh |
| `model` | Reads a model file with Assimp (glTF first, but OBJ and FBX work too) and turns it into meshes and materials | A list of mesh and material pairs |
| `material.h` | A plain struct: the texture ids and constant factors for one material | Bound by the renderer before drawing |
| `color` | One small helper, `srgbToLinear`, for colors picked by eye (background, grid, axes, panel colors) | Linear colors |
| `framebuffer` | Creates the off-screen targets: the HDR target (a 4x multisample float color texture and a multisampled depth buffer) and later the shadow depth map | Framebuffer and texture ids |
| `renderer` | Runs the three passes for one frame | The final image on screen |
| `ui` | Draws the ImGui control panel | Changes to `Settings` |

```mermaid
flowchart TD
  main[main.cpp<br/>window, loading, frame loop]
  ui[ui<br/>ImGui panel]
  settings[settings.h<br/>all adjustable values]
  camera[camera<br/>orbit camera]
  model[model<br/>glTF loading]
  texture[texture<br/>images, cubemap]
  environment[environment<br/>lighting maps]
  mesh[mesh<br/>vertex arrays]
  material[material.h<br/>texture ids and factors]
  shader[shader<br/>compile, link, uniforms]
  framebuffer[framebuffer<br/>shadow map, HDR target]
  color[color<br/>sRGB helper]
  renderer[renderer<br/>shadow, lighting, tone map]

  main --> ui
  main --> camera
  main --> model
  main --> renderer
  ui -->|writes| settings
  settings -->|read by| renderer
  camera -->|view and projection| renderer
  model --> mesh
  model --> material
  model --> texture
  material --> texture
  main --> environment
  environment --> renderer
  mesh --> renderer
  material --> renderer
  shader --> renderer
  framebuffer --> renderer
  color --> renderer
```

The shaders live in `shaders/` as separate files:

| File | Contents |
|---|---|
| `mesh.vert` | Vertex shader for the lighting pass |
| `common.glsl` | Uniforms, the `Surface` and `Light` structs, texture reading |
| `environment.glsl` | Reading the environment cubemaps with their rotation and intensity (used by the hybrid shader and the sky) |
| `pbr_terms.glsl` | The physical version of each lighting term |
| `npr_terms.glsl` | The stylized version of each lighting term |
| `hybrid_main.glsl` | `main()`: builds the inputs, calls both versions, blends, debug views |
| `shadow_depth.vert`, `shadow_depth.frag` | Shadow pass |
| `fullscreen.vert`, `tonemap.frag` | Tone mapping pass |
| `skybox.vert`, `skybox.frag` | The sky behind the model |
| `sampling.glsl`, `irradiance.frag`, `prefilter.frag`, `brdf_lut.frag` | Startup passes that build the environment lighting maps and the lookup table |
| `outline.vert`, `outline.frag` | Optional outline pass |

GLSL has no `#include`. The `shader` module joins `common.glsl`, `environment.glsl`, `pbr_terms.glsl`,
`npr_terms.glsl` and `hybrid_main.glsl` in that order into one fragment shader. The split exists so that the two of
us can each work in our own file.

## 3. One frame, step by step

### On the CPU

1. Read mouse and keyboard input. Update the camera and the light direction.
2. Build the control panel. Slider changes go into `Settings`.
3. Compute the light's view-projection matrix: an orthographic box that looks along the light
   direction and encloses the model.
4. When the Environment menu changed (and on the first frame): load the `.hdr` image, turn it into
   a cubemap and build its lighting maps (see "Made at startup" below).

### Made at startup

These are drawn once with small fragment shaders, each a single triangle covering one face or
the whole table, and kept until the program ends or the environment changes:

| Map | Size | What it holds |
|---|---|---|
| Environment cubemap | 512 x 512 per face, all mip levels, RGBA16F | The sky itself, built on the CPU from the panorama |
| Irradiance map | 32 x 32 per face, RGBA16F | The soft light a matte surface facing each direction receives (fill light) |
| Prefiltered map | 512 x 512 per face, 6 mip levels, RGBA16F | The environment blurred for roughness 0, 0.2, ... 1, one level each (reflections) |
| BRDF lookup table | 512 x 512, RG16F | Two numbers per N.V and roughness that turn the prefiltered color into the reflected amount; made once, it does not depend on the image |

### On the GPU

| Pass | Draws | Reads | Writes into |
|---|---|---|---|
| 1. Shadow | The model, seen from the light, depth only | Vertex positions, light matrix | Shadow map: a 2048 x 2048 depth texture |
| 2. Lighting | The model with the hybrid shader, once per view, then the grid and axes, then the sky (a cube at the far depth, drawn only where nothing else is; skipped for the Plain background and in debug views) | Material textures, shadow map, environment cubemap and its lighting maps, BRDF lookup table, ramp texture, camera and light uniforms, blend weights | HDR target: 16-bit float color and depth the size of the window, 4 samples per pixel. Alpha marks scene pixels (model and sky 1, everything else 0) |
| 2b. Outline (optional) | The model again, slightly enlarged, back faces only, in a flat color | Vertex positions and normals | The same HDR target |
| 3. Tone mapping | One triangle that covers the screen, drawn once per view and once more for the area under the panel. Each sample of a pixel gets its own treatment, then they are averaged | Every sample of the HDR target, exposure, tone curve, tone amount of each view | The window's own framebuffer |
| 4. UI | The ImGui panel | ImGui draw data | The window's own framebuffer |

```mermaid
flowchart LR
  subgraph CPU
    input[Input, camera, light]
    panel[UI panel to Settings]
    lightmat[Light matrix]
  end

  subgraph GPU
    shadow[Pass 1: shadow<br/>depth from the light]
    smap[(Shadow map<br/>depth texture)]
    lighting[Pass 2: lighting<br/>hybrid shader, once per view]
    outline[Pass 2b: outline<br/>optional]
    hdr[(HDR target<br/>float color, 4 samples)]
    tonemap[Pass 3: tone mapping<br/>each sample: exposure, tone curve, sRGB<br/>then average, once per view]
    screen[(Window)]
    imgui[Pass 4: UI panel]
  end

  textures[(Material textures, ramp,<br/>environment maps, lookup table)]

  input --> lightmat --> shadow
  shadow --> smap --> lighting
  textures --> lighting
  panel -->|weights| lighting
  panel -->|exposure, curve, tone amount| tonemap
  lighting --> hdr
  outline --> hdr
  hdr --> tonemap --> screen
  imgui --> screen
```

### The side-by-side view

In pass 2 the window is split into three vertical strips with `glViewport` and `glScissor`. The
model is drawn once into each strip with the same camera and light. Only the five weights differ:
all 0 for PBR, the slider values for the hybrid, all 1 for NPR. The shadow pass runs once and is
shared. Drawing the scene three times is wasteful, but our scenes are one model, and it means the
comparison needs no extra shader code.

The tone mapping pass uses the same strips. It runs once per strip, cut to the strip with
`glScissor`, and each strip gets its own tone amount: how much of exposure and the tone curve it
gets. PBR always gets all of it (in a debug view only while the debug tick is on). NPR gets all of
it while the "NPR uses tone mapping" tick is on (the start) and none when it is off, so its colors
are shown as computed, only cut off at 1. The Hybrid view is set by a sixth blend slider, "Tone
mapping": 0 shows the Hybrid strip the way PBR is shown, 1 the way NPR is shown. Both ends are
the Hybrid strip's own shading, so the slider mixes two display treatments of it, not the PBR and
NPR pictures. With the tick on, both ends are the same and the slider changes nothing. One more
draw with amount 0 covers the area under the panel.

### Color space rule

All lighting math happens in linear color. Three places enforce this, and nowhere else touches it:

1. **Loading.** Base color and emissive textures and the ramp are created with an sRGB
   format, so the GPU converts them to linear when the shader reads them. Metallic-roughness,
   normal and occlusion textures hold data, not colors, and are created with a plain format. The
   environment image is loaded as floats that are already linear. Colors we pick by eye, the
   background, grid and axis colors and the panel's base color and ambient color, are sRGB too:
   `srgbToLinear` turns each into linear once before it is used.
2. **Lighting.** The HDR target stores linear values, including values above 1. Its alpha channel
   marks what is scene: the model and the sky write 1, the plain background, grid and axes 0.
3. **Tone mapping.** The last shader applies exposure, a tone curve that brings bright values into
   the 0 to 1 range, and then the linear to sRGB conversion. Pixels marked as not scene get only
   the conversion, so the background, grid and axes look the same in every mode. `GL_FRAMEBUFFER_SRGB`
   stays off, so the conversion happens exactly once and ImGui is drawn afterward untouched.

Debug views follow the same idea. Base color and emissive are colors, so they get the sRGB step.
Metallic, roughness, normal, occlusion and shadow are data and are shown as stored, never
converted, so they match the Khronos glTF Sample Viewer. Whether a debug view also gets exposure and
the curve is a panel tick that sets itself when the debug view changes: off for base color,
emissive and the data views, on for the
diffuse, specular, ambient and reflection terms, which are light.

## 4. Data flow for one material

```
Files on disk           model.gltf or .glb, with its images
                        or a folder of loose textures (base color, roughness, metallic)
        |
        v  model + texture modules, on the CPU, once at startup
Loaded data             Mesh: positions, normals, UVs, indices
                        Material: base color texture (sRGB), metallic-roughness texture (linear),
                        constant factors
        |
        v  uploaded to the GPU once
GPU objects             Vertex array with attributes 0 position, 1 normal, 2 UV
                        2D textures
        |
        v  renderer, every frame, before drawing the mesh
Uniforms and bindings   Matrices: model, view, projection, light
                        Camera position, light direction, color and intensity
                        Texture units: 0 base color, 1 metallic-roughness, 2 normal (optional),
                        3 shadow map, 4 environment cubemap, 5 ramp, 6 irradiance map,
                        7 prefiltered map, 8 BRDF lookup table, 9 occlusion, 10 emissive
                        Environment rotation, intensity, method and mip counts
                        Five blend weights, stylized parameters, debug view number
        |
        v  mesh.vert, per vertex
Passed to fragments     World position, world normal, UV, position in light space
        |
        v  hybrid shader, per pixel
Linear HDR color        Written to the HDR target, several samples per pixel
        |
        v  tonemap.frag, each sample treated, then averaged
Final pixel             sRGB color in the window
```

glTF stores roughness in the green channel and metallic in the blue channel of one texture. Loose
texture sets that come as separate roughness and metallic images are packed into that same layout
when they are loaded, so the shader only ever sees one layout.

## 5. Inside the hybrid shader

### Inputs every term receives

`hybrid_main.glsl` builds two structs once per pixel and passes them to every term function.

```glsl
struct Surface {
  vec3 position;    // world space
  vec3 N;           // unit surface normal
  vec3 V;           // unit direction from the surface to the camera
  vec3 baseColor;   // linear
  float metallic;   // 0 non-metal, 1 metal
  float roughness;  // 0 mirror, 1 fully rough
  float occlusion;  // 1 open, toward 0 in creases the environment light reaches less
};

struct Light {
  vec3 L;           // unit direction from the surface to the light
  vec3 radiance;    // light color times intensity, linear
};
```

### Two versions of each term, one interface

| Term | Physical version, in `pbr_terms.glsl` | Stylized version, in `npr_terms.glsl` |
|---|---|---|
| Shadow: `float shadow*(float visibility)` | Returns the filtered shadow map result as is: a soft edge | Sharpens it into a hard edge with an adjustable threshold |
| Diffuse: `vec3 diffuse*(Surface s, Light l, float shadow)` | Lambert: base color / pi, times N.L, times shadow, times (1 - F)(1 - metallic) with the non-metal F (F0 = 0.04), so metals have none | Base color / pi times the ramp looked up at `(N.L * 0.5 + 0.5) * shadow`, times radiance. The shadow moves the lookup position. It is not multiplied on afterward. Metals keep their base color here |
| Specular: `vec3 specular*(Surface s, Light l, float shadow)` | Cook-Torrance as in the Khronos glTF Sample Viewer: GGX, height-correlated Smith visibility, Schlick Fresnel, times N.L, times shadow | The GGX term divided by its own peak, cut at a threshold with a soft edge about 1.4 pixels wide, and faded out where it meets the dark side. The result is a flat highlight whose size grows with roughness and whose brightness fades as roughness grows. Metals tint it with their base color |
| Ambient: `vec3 ambient*(Surface s)` | Fill light from the environment: the irradiance map along N times the base color, times (1 - F)(1 - metallic) with the non-metal environment F, so metals have none. Times the glTF occlusion; not dimmed by the shadow | A flat ambient color times the base color (a placeholder for now) |
| Reflections: `vec3 reflections*(Surface s)` | The environment seen along the reflection direction: the prefiltered map at the level for the roughness, times the environment Fresnel from the BRDF lookup table (with the multiple-bounce compensation). Times the glTF occlusion; not dimmed by the shadow | None yet (black), so sliding toward NPR fades the reflections out |

The shadow map lookup itself (the depth comparison and the 3 x 3 filtering) is shared code in
`common.glsl`. It produces `visibility`, a number from 0 (in shadow) to 1 (lit).

### Blending

```glsl
float shadow = mix(shadowPhysical(vis),          shadowStylized(vis),          uWeightShadow);
vec3 diffuse  = mix(diffusePhysical(s, l, shadow),  diffuseStylized(s, l, shadow),  uWeightDiffuse);
vec3 specular = mix(specularPhysical(s, l, shadow), specularStylized(s, l, shadow), uWeightSpecular);
vec3 ambient  = mix(ambientPhysical(s),             ambientStylized(s),             uWeightAmbient);
vec3 reflections = mix(reflectionsPhysical(s),     reflectionsStylized(s),     uWeightReflections);
fragColor = vec4(diffuse + specular + ambient + reflections + emissive, 1.0);
```

`emissive` (the glTF glow, factor times texture) is not a term with two versions: it is the same on
both sides, so it is added after the blend and the glow light is the same in every view. What you
see on screen can still differ: with "NPR uses tone mapping" off, the toon side shows it without
exposure and the tone curve.

A weight of 0 is physical and 1 is stylized. Both versions are always computed. That costs a little
GPU time and keeps the shader free of branches, so any mix can be dialed in while the program runs.

The Method switch in the panel (Simple / Prefiltered) picks how the physical ambient and
reflections read the environment: Simple uses the plain mip levels of the cubemap, Prefiltered the
maps made at startup. It is a uniform, so switching costs nothing.

The view modes are not in the shader. The renderer picks which weights to send: zeros, ones, or the
slider values.

A `uDebugView` uniform makes the shader output one thing on its own: base color, metallic,
roughness, the normal, shadow visibility, or a single lighting term. We use this to check our work
against a reference viewer one input at a time.

### Keeping the two sides comparable

Both diffuse versions use the same light and the same base color / pi factor. The ramp only replaces
the N.L factor. A white ramp gives almost the same brightness the physical side has where the
surface faces the light directly, so the two sides stay in the same range and differences on screen
come from the shading model, not from mismatched brightness. Almost, because the physical diffuse
also loses the few percent of light that the surface reflects as highlight (the non-metal Fresnel
term; a white sphere reads about 250 instead of 255), and the stylized side has no Fresnel.

The display step is shared the same way: by default both sides get the same exposure and tone
curve. Unticking "NPR uses tone mapping" takes them away from the NPR side, which is handy for
seeing its colors as computed, but then part of the difference on screen comes from the display
step and not from the shading.

## 6. Why it is arranged this way

- **One forward lighting pass.** We have one light and one model. Deferred shading would add several
  more render targets to explain and debug for no benefit here.
- **Terms as separate functions with a fixed interface.** The project asks which combinations look
  good. A new idea should mean changing one function, and the blend code should never need to
  change.
- **Shader split into files by side.** Two people can work on the physical and stylized sides at
  the same time without editing the same file.
- **One `Settings` struct.** Every adjustable value is in one place, so the UI and the renderer
  cannot disagree and a new slider is two lines.
- **OpenGL 4.1 core.** It is the newest version that runs on macOS, so the same code builds on every
  platform.

Kept simple on purpose:

- One directional light. No point lights or light lists.
- Environment lighting maps are made once per image when it is loaded, not every frame, and the
  prefiltered map assumes the camera looks along the normal (no stretched reflections at grazing
  angles). The simpler method with the cubemap's ordinary mip chain stays in as a switch for
  comparison.
- One fixed shadow map that encloses the model. No cascades.
- The scene is drawn three times for the comparison view, not composited in a cleverer way.
- No hot paths are optimized. The target is one model at interactive frame rates.
- Loading supports what our test assets need from glTF: triangles, positions, normals, one UV set
  and the metallic-roughness material. No animation, skinning or cameras.

## Glossary

- **PBR, physically based rendering.** Shading that follows measured rules of how light reflects, so
  a material looks right under any lighting.
- **NPR, non-photorealistic rendering.** Shading that aims for a drawn or painted look instead.
- **BRDF.** A function that says how much light arriving from one direction leaves a surface toward
  another direction. Phong is a simple BRDF.
- **Microfacet model, Cook-Torrance.** Treats a surface as many tiny mirrors. Three parts: **D**
  (GGX) says how many tiny mirrors face the right way, **G** (Smith) says how many are hidden by
  their neighbors, **F** (Fresnel, Schlick's approximation) says how reflective they are at this
  viewing angle.
- **Metallic and roughness.** The two numbers that describe a PBR material besides its base color.
  Metals have no diffuse color and tint their reflections. Rough surfaces have wide, dim highlights.
- **Linear and sRGB.** Image files store colors on a curve (sRGB) that suits displays. Lighting math
  is only correct on values proportional to light energy (linear). Convert on the way in and on the
  way out.
- **HDR, high dynamic range.** Storing colors as floats that can exceed 1, so bright highlights are
  not clipped during lighting.
- **Tone mapping.** Squeezing HDR values into the 0 to 1 range a display can show.
- **Exposure.** A brightness factor applied before the tone curve, like a camera setting.
- **MSAA, multisampling.** Storing several samples per pixel (we use 4) so edges look smooth.
- **Multisample texture.** A texture that keeps every sample of each pixel. A shader reads one
  sample at a time with `texelFetch`. We use it instead of a *resolve*, the usual step that
  averages the samples into a normal texture first, so the model and the background on an edge
  can each get their own treatment before they are averaged.
- **Framebuffer.** An off-screen image the GPU can draw into. Its textures can be read by a later
  pass.
- **Shadow map.** A depth image drawn from the light. A pixel is in shadow if it is farther from the
  light than the depth stored there.
- **PCF, percentage closer filtering.** Averaging several shadow comparisons around a pixel to
  soften the shadow edge.
- **Shadow acne.** Speckled false shadows on lit surfaces, caused by limited depth precision. Fixed
  with a small depth offset called a bias.
- **Cubemap.** Six square textures arranged as a cube and looked up with a direction. Used for the
  environment.
- **Mip level.** A smaller, blurrier copy of a texture. Higher levels are blurrier.
- **Ramp.** A 1D texture of colors. The stylized diffuse uses a brightness value as the position to
  read from it, which turns smooth shading into bands.
- **Inverted hull.** An outline method: draw the model again slightly enlarged, showing only its
  back faces, in the outline color.
