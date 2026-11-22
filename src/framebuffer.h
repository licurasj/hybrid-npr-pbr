#pragma once

#include <glad/gl.h>

// Samples per pixel asked for the HDR target, which smooth the jagged edges of the model
// OpenGL 4.1 only promises 1 for a multisample texture, so if the driver allows fewer we ask for
// that many. It may also give more than asked, which works as long as color and depth get the
// same count.
const int HDR_SAMPLES = 4;

// The HDR target the scene is drawn into
// HDR (high dynamic range) means the colors are stored as 16 bit floats, so they can go above 1.
// Each pixel keeps HDR_SAMPLES samples. The tone mapping pass reads every sample on its own,
// gives it its own treatment and only then averages them, so the edge of the model and the grid
// or background next to it each get their own rule.
struct HdrTarget {
  GLuint framebuffer = 0;
  GLuint colorTexture = 0;  // multisample texture, RGBA16F
  GLuint depthBuffer = 0;   // multisample renderbuffer, DEPTH_COMPONENT24

  int width = 0;
  int height = 0;
  int samples = 0;  // how many samples the driver really gave the color texture
};

// Make the color texture and depth buffer for a window of this size in framebuffer pixels
// Prints what is wrong and returns false if OpenGL cannot use them.
bool createHdrTarget(HdrTarget& target, int width, int height);

// Make the target again if the window size changed, otherwise do nothing
// Returns false like createHdrTarget.
bool resizeHdrTarget(HdrTarget& target, int width, int height);

// Free the texture and the buffers
void deleteHdrTarget(HdrTarget& target);
