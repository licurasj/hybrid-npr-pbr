# Work plan

Here's a rough plan for getting the viewer done by the demo. It's a suggestion, so change anything
that doesn't work for you. Dates and hours are guesses.

- `architecture.md` explains how the program is put together.
- `milestone-guide.md` has the details for each part: files, steps, formulas.

## Course dates

Please double-check these on Brightspace.

| When | What |
|---|---|
| Sat Nov 21 | Code demo: a short zip showing we've started |
| Sat Nov 28 | Report outline |
| Thu Dec 3, in class | Final demo |
| Sat Dec 5 | Final turn-in: code and report |

## How we split the work

No fixed roles. Pick any part whose "Needs first" is done, tell the other person you're on it, and
grab the next one when you finish.

Each part has a label so you know what kind of work it is:

- **PBR**: the realistic side
- **NPR**: the stylized, toon side
- **Both**: the viewer itself, which both sides use

Only the first two parts are open at the start, so one of us does those. Meanwhile the other can
set up their machine and look for nice free textures and models.

## Progress

Put your name next to a part when you start it, and update the status when it works.

| Part | Who | Status |
|---|---|---|
| Project skeleton | Lucas | Done |
| Phong sphere | Lucas | Done |
| Term skeleton and comparison view | Lucas | Done |
| HDR and color | Lucas | Done |
| glTF loading | Lucas | Done |
| Test materials | | Not started |
| Ramp shading and stylized highlight | | Not started |
| Cook-Torrance | | Not started |
| Shadow map | | Not started |
| Shadow in the ramp | | Not started |
| Environment lighting | | Not started |
| Hybrid polish | | Not started |
| Reference check | | Not started |
| Demo and report material | | Not started |

## The parts

| Part | What you'll see when it works | Side | Hours | Needs first | Aim for |
|---|---|---|---|---|---|
| Project skeleton | A window with a background color and an empty control panel | Both | 4 | nothing | Nov 20 |
| Phong sphere | A lit sphere. The mouse orbits the camera and moves the light. `R` reloads the shaders | Both | 5 | Project skeleton | Nov 21 |
| Term skeleton and comparison view | Three views side by side, four blend sliders, a debug view menu, a floor grid and an axis indicator. Simple placeholder shading for now | Both | 7 | Phong sphere | Nov 24 |
| HDR and color | Same picture, now tone mapped, with an exposure slider | Both | 5 | Phong sphere | Nov 23 |
| glTF loading | A sample model with its textures | Both | 9 | Phong sphere | Nov 26 |
| Test materials | Cloth, leather, metal, stone and wood on the sphere, picked from a menu | Both | 8 | Phong sphere | Nov 27 |
| Ramp shading and stylized highlight | Banded toon shading and a hard-edged highlight | NPR | 9 | Term skeleton | Nov 27 |
| Cook-Torrance | Metals and non-metals that look right at every roughness | PBR | 7 | Term skeleton, HDR and color | Nov 28 |
| Shadow map | The model casts a soft shadow on itself and on the ground | Both | 5 | Phong sphere | Nov 29 |
| Shadow in the ramp | On the toon side, shadows fall into the same color bands as the shading | NPR | 7 | Ramp shading, Shadow map | Nov 30 |
| Environment lighting | A sky behind the model. Metals reflect it, blurrier as they get rougher | PBR | 9 | HDR and color, Cook-Torrance | Dec 1 |
| Hybrid polish | Toon ambient light, preset buttons for nice mixes, labels on the three views | NPR, then Both | 7 | Ramp shading, Cook-Torrance | Dec 1 |
| Reference check | Screenshots comparing our PBR view with a reference viewer | PBR | 4 | Cook-Torrance, Environment lighting | Dec 2 |
| Demo and report material | Final screenshots, finished README, a practiced 10 minute demo | Both | 8 | everything | Dec 2 |

That's about 94 hours in total, which is a full two weeks. The extras further down are only for
when we have time left.

## What holds up what

| Part | Side | Holds up |
|---|---|---|
| Project skeleton | Both | Everything |
| Phong sphere | Both | Everything after it |
| Term skeleton and comparison view | Both | Both shading sides: Ramp shading and Cook-Torrance, and everything built on them |
| HDR and color | Both | Cook-Torrance, Environment lighting |
| glTF loading | Both | Nothing, though the Reference check uses its models |
| Test materials | Both | Nothing |
| Ramp shading and stylized highlight | NPR | Shadow in the ramp, Hybrid polish |
| Cook-Torrance | PBR | Environment lighting, Hybrid polish, Reference check |
| Shadow map | Both | Shadow in the ramp |
| Shadow in the ramp | NPR | Nothing |
| Environment lighting | PBR | Reference check |
| Hybrid polish | NPR, then Both | Nothing |
| Reference check | PBR | Nothing |
| Demo and report material | Both | Nothing |

In short:

- The first two parts come first, and one person does them. Nothing else can start before the
  sphere is on screen.
- Right after that, five parts open at once: Term skeleton, HDR and color, glTF loading, Test
  materials and Shadow map.
- Term skeleton is the next one to grab, since neither shading side can start without it.
- The longest chain is Project skeleton, Phong sphere, Term skeleton (with HDR and color),
  Cook-Torrance, Environment lighting, Reference check. A delay there moves the end date.

## A few things that will help

- Let's get the main parts working before starting any extras.
- When a part works, it's worth a quick try on the other person's computer too. Mac and Windows
  sometimes disagree, and it's easier to catch early.
- Let's try to stop adding features around Tuesday, December 1, so there's time for screenshots
  and practicing the demo.

## How to tell a part is done

Quick things to look for. They're there to catch mistakes early, not to be ticked off formally.

- **Project skeleton.** It builds from the README commands, and the window title shows OpenGL 4.1
  or higher.
- **Phong sphere.** The highlight moves when the light moves. Orbiting and zooming never flip the
  view. Editing a shader and pressing `R` changes the picture.
- **Term skeleton and comparison view.** Sliders at 0 and at 1 give clearly different pictures.
  The left and right views match the middle one when its sliders are all 0 or all 1.
- **HDR and color.** With the tone curve off and exposure 1, take a screenshot and read colors
  with a color picker. A white sphere under a white light of intensity pi reads 255 where it faces
  the light, and about 188 (not 128) where it is half lit. Later, a gray 128 texture reads 128 in
  the base color debug view.
- **glTF loading.** The base color, metallic, roughness and normal debug views look the same as in
  the Khronos glTF Sample Viewer. Nothing is flipped or swapped.
- **Test materials.** Metal is white in the metallic debug view and cloth is black. Every file has
  a line in `assets/CREDITS.md`.
- **Ramp shading and stylized highlight.** A two-color ramp gives exactly one sharp edge. The
  highlight is small on smooth surfaces, large on rough ones, and clean-edged on both.
- **Cook-Torrance.** A grid of spheres (metallic one way, roughness the other) looks like the
  reference picture on LearnOpenGL's PBR lighting page. Metals have no diffuse color, and edges
  get brighter at grazing angles.
- **Shadow map.** A sphere above the ground casts a round shadow in the right place as the light
  moves. No speckles on lit surfaces, and the shadow stays attached to the object.
- **Shadow in the ramp.** With a three-band ramp, the cast shadow and the dark side of the shading
  are the same color and join without a seam. Turn the light all the way around and look for
  speckles.
- **Environment lighting.** A perfectly smooth metal sphere mirrors the sky with no seams, the
  right way up. A fully rough one shows a smooth blur.
- **Hybrid polish.** Each preset looks the same after a restart. Each debug view shows what its
  name says.
- **Reference check.** We load the same sample models in our viewer and in the
  [Khronos glTF Sample Viewer](https://github.khronos.org/glTF-Sample-Viewer-Release/). The
  material debug views should match exactly. The lit picture will be close but not identical,
  mostly in reflections on rough surfaces, because we use a simpler method. We write down where it
  differs and why.
- **Demo and report material.** One of us runs the demo from a fresh copy without touching code.

## If we run short on time

Nothing is cut for now. If we do fall behind, these are the easiest things to drop, in this order.
The main idea, blending each lighting term separately, survives all of them.

1. The preset buttons and view labels in Hybrid polish
2. Two of the five test materials (keep cloth, leather and metal)
3. Reflections: use a plain ambient color on the PBR side
4. Soft shadow edges: use a single hard shadow sample

If glTF loading is still giving trouble around November 27, we can carry on with the sphere and
the test materials and come back to it later.

## Extras, if there's time

| Extra | What you'll see | Hours | Needs first |
|---|---|---|---|
| Ramp presets | A menu of ramps, including a cool-to-warm one | 2 | Ramp shading |
| Rim light | A bright edge where the surface turns away from the camera | 2 | Ramp shading |
| Normal mapping | Weave and grain detail on both sides | 4 | glTF loading |
| Outline | A dark outline around the model in the toon and hybrid views | 4 | Ramp shading |
| Cloth sheen | A soft bright edge on cloth at grazing angles | 4 | Cook-Torrance |
| Stretched highlight | For brushed metal or satin | 5 | Cook-Torrance, Normal mapping |
| Accurate reflections | Rough reflections that match the reference viewer | 8 | Environment lighting |
| Blend per material | Each material of a model (cloth, skin, hair) gets its own four blend sliders, saved in presets, e.g. PBR cloth with NPR skin and hair | 3 | glTF loading, Hybrid polish |
| Per-region mask | A painted mask texture fine-tunes the blend inside a material, e.g. a soft fade from skin to sleeve. Works on top of the per-material blend | 6 | Blend per material |
