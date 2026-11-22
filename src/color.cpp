#include "color.h"

#include <cmath>


// sRGB to linear for one channel, the formula of the sRGB standard
// Dark values use a straight line, the rest a power curve.
float srgbToLinear(float c) {
  if (c <= 0.04045f) {
    return c / 12.92f;
  }
  return std::pow((c + 0.055f) / 1.055f, 2.4f);
}


// sRGB to linear for each channel of a color
glm::vec3 srgbToLinear(const glm::vec3& color) {
  return glm::vec3(srgbToLinear(color.r), srgbToLinear(color.g), srgbToLinear(color.b));
}
