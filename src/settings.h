#pragma once

#include <glm/glm.hpp>

// Every value the control panel can change
// The panel writes these and the drawing code reads them.
struct Settings {
  // Light direction in degrees, set by the right mouse button or the panel
  float lightYaw = 45.0f;
  float lightPitch = 30.0f;

  // Phong material
  glm::vec3 baseColor = glm::vec3(0.8f, 0.3f, 0.2f);
  float ambient = 0.1f;
  float diffuse = 0.8f;
  float specular = 0.5f;
  float shininess = 32.0f;
};
