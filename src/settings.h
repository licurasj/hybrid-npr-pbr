#pragma once

#include <glm/glm.hpp>

// Every value the control panel can change
// The panel writes these and the drawing code reads them.
struct Settings {
  // Light direction in degrees, set by the right mouse button or the panel
  float lightYaw = 45.0f;
  float lightPitch = 30.0f;

  // Light brightness, the light color is white
  float lightIntensity = 3.0f;

  // Flat ambient color used by the stylized ambient
  glm::vec3 ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);

  // Material
  glm::vec3 baseColor = glm::vec3(0.8f, 0.3f, 0.2f);
  float metallic = 0.0f;
  float roughness = 0.5f;
  float shininess = 32.0f;

  // Blend weights for shadow, diffuse, specular and ambient
  // 0 is physical and 1 is stylized.
  float weights[4] = { 0.5f, 0.5f, 0.5f, 0.5f };

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
