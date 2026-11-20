#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>


// How fast the mouse turns the camera, in degrees per pixel
const float ORBIT_SPEED = 0.3f;

// Pitch stays below straight up or down so the view never flips
const float MAX_PITCH = 89.0f;

// Closest and farthest zoom
const float MIN_DISTANCE = 1.5f;
const float MAX_DISTANCE = 20.0f;


// Unit direction for a yaw and pitch
// Yaw turns around the up axis and pitch tilts up or down.
glm::vec3 directionFromAngles(float yaw, float pitch) {
  float y = glm::radians(yaw);
  float p = glm::radians(pitch);
  return glm::vec3(glm::cos(p) * glm::sin(y), glm::sin(p), glm::cos(p) * glm::cos(y));
}


// Where the camera is in the world
glm::vec3 cameraPosition(const Camera& camera) {
  return camera.target + camera.distance * directionFromAngles(camera.yaw, camera.pitch);
}


// The view matrix looks from the camera position at the target
glm::mat4 cameraView(const Camera& camera) {
  return glm::lookAt(cameraPosition(camera), camera.target, glm::vec3(0.0f, 1.0f, 0.0f));
}


// The projection matrix with a 45 degree field of view
glm::mat4 cameraProjection(float aspect) {
  float fov = glm::radians(45.0f);

  // A narrow strip gets a taller field of view so its width still sees 45 degrees
  // and the model fits across it.
  if (aspect < 1.0f) {
    fov = 2.0f * glm::atan(glm::tan(fov / 2.0f) / aspect);
  }
  return glm::perspective(fov, aspect, 0.1f, 100.0f);
}


// Turn the camera around the target by a mouse movement in pixels
// Dragging right turns the camera left so the model seems to follow the mouse.
void orbitCamera(Camera& camera, float dx, float dy) {
  camera.yaw -= dx * ORBIT_SPEED;
  camera.pitch += dy * ORBIT_SPEED;
  camera.pitch = glm::clamp(camera.pitch, -MAX_PITCH, MAX_PITCH);
}


// Move the camera closer or farther with the mouse wheel
// Each step changes the distance by ten percent, which feels even at any zoom.
void zoomCamera(Camera& camera, float scroll) {
  camera.distance *= glm::pow(0.9f, scroll);
  camera.distance = glm::clamp(camera.distance, MIN_DISTANCE, MAX_DISTANCE);
}
