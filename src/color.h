#pragma once

#include <glm/glm.hpp>

// Turn a color picked by eye (an sRGB color, like the colors in a paint program)
// into the linear value the shaders compute with.
// The last pass turns linear values back into sRGB for the screen, so a color that goes
// through both comes out as it was picked.
glm::vec3 srgbToLinear(const glm::vec3& color);

// The same for one channel
float srgbToLinear(float c);
