#pragma once

#include <glm/glm.hpp>

// The first two entries of the model menu, which are built by the program and not loaded from files
const int MODEL_SPHERE = 0;
const int MODEL_SPHERE_GRID = 1;

// Every value the control panel can change
// The panel writes these and the drawing code reads them.
struct Settings {
  // Light direction in degrees, set by the right mouse button or the panel
  float lightYaw = 45.0f;
  float lightPitch = 30.0f;

  // Light brightness, the light color is white
  float lightIntensity = 3.0f;

  // Brightness factor before the tone curve, 1 leaves the picture as it is
  float exposure = 1.0f;

  // Tone curve that squeezes bright values into what the screen can show,
  // in the order of the tone curve menu: 0 Off, 1 Reinhard, 2 ACES fit
  int toneCurve = 0;

  // Whether the NPR view also gets exposure and the tone curve
  // When it is off, the NPR colors are shown as computed, only cut off at 1.
  bool toonUsesToneMapping = true;

  // Whether a debug view gets exposure and the tone curve
  // It is set again each time the debug view changes, see the debug view menu in the panel.
  // The normal lit picture always gets them.
  bool debugViewToneMapping = true;

  // Flat ambient color used by the stylized ambient
  glm::vec3 ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);

  // Which entry of the model menu is shown, see MODEL_SPHERE and MODEL_SPHERE_GRID below
  // The model files come after those two.
  int modelChoice = 0;

  // Material of the sphere, a loaded model uses the values from its file
  glm::vec3 baseColor = glm::vec3(0.8f, 0.3f, 0.2f);
  float metallic = 0.0f;
  float roughness = 0.5f;

  // Stylized shading
  // Which ramp image the toon diffuse uses, in the order of the ramp menu:
  // 0 two bands, 1 three bands
  int rampChoice = 0;
  // Where the edge of the toon highlight sits: 0.5 puts it where the GGX shape of the highlight
  // has fallen to half its peak, a higher value makes the spot smaller
  float highlightThreshold = 0.5f;
  // Brightness of the toon highlight, 1 is as bright as a white surface facing the light
  float highlightStrength = 1.0f;
  // Whether the toon highlight is dimmed by the ramp band it falls in
  bool highlightFollowsRamp = false;

  // Blend weights for shadow, diffuse, specular and ambient
  // 0 is physical and 1 is stylized.
  float weights[4] = { 0.5f, 0.5f, 0.5f, 0.5f };

  // Blend weight for the tone mapping of the Hybrid view
  // 0 shows it like the PBR view and 1 like the NPR view.
  // When the NPR view uses tone mapping too, both ends are the same.
  float toneMappingWeight = 0.5f;

  // Which views are shown side by side, in the order of the view mode menu
  // 0 shows PBR, Hybrid and NPR side by side.
  int viewMode = 0;

  // Which value the shader shows on its own, in the order of the debug view menu
  // 0 is the normal lit picture.
  int debugView = 0;

  // Floor grid and the axis indicator in the corner of each view
  bool showGrid = true;
  bool showAxes = true;
};
