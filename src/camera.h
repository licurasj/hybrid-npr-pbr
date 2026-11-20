#pragma once

#include <glm/glm.hpp>

// Orbit camera that circles around a target point
// Angles are in degrees.
struct Camera {
  float yaw = 0.0f;
  float pitch = 20.0f;
  float distance = 3.6f;
  glm::vec3 target = glm::vec3(0.0f);
};

// Unit direction for a yaw and pitch, used for the camera and the light
glm::vec3 directionFromAngles(float yaw, float pitch);

// Where the camera is in the world
glm::vec3 cameraPosition(const Camera& camera);

// The view matrix that looks from the camera at the target
glm::mat4 cameraView(const Camera& camera);

// The projection matrix for the given width to height ratio
glm::mat4 cameraProjection(float aspect);

// Turn the camera around the target by a mouse movement in pixels
void orbitCamera(Camera& camera, float dx, float dy);

// Move the camera closer or farther with the mouse wheel
void zoomCamera(Camera& camera, float scroll);
