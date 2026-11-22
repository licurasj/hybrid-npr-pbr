#include "framebuffer.h"

#include <algorithm>
#include <cstdio>


// Print why a framebuffer is not complete
// Returns true if it is complete.
static bool checkFramebuffer(const char* name) {
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status == GL_FRAMEBUFFER_COMPLETE) {
    return true;
  }
  const char* reason = "unknown reason";
  if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT) {
    reason = "an attachment cannot be drawn into";
  } else if (status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT) {
    reason = "nothing is attached";
  } else if (status == GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE) {
    reason = "the attachments have different sample counts";
  } else if (status == GL_FRAMEBUFFER_UNSUPPORTED) {
    reason = "this format combination is not supported by the driver";
  }
  std::printf("The %s framebuffer cannot be used: %s (status 0x%x)\n", name, reason, status);
  return false;
}


// Make the multisample color texture and depth buffer
bool createHdrTarget(HdrTarget& target, int width, int height) {
  target.width = width;
  target.height = height;

  // Ask for HDR_SAMPLES, or fewer if the driver cannot make a multisample texture with that many
  GLint maxSamples = 1;
  glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxSamples);
  int askedSamples = std::min(HDR_SAMPLES, (int)maxSamples);

  // Color: a multisample texture, so the tone mapping pass can read each sample
  // GL_TRUE keeps the sample positions the same in every pixel, which OpenGL needs
  // when a texture and a renderbuffer are attached together.
  glGenTextures(1, &target.colorTexture);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, target.colorTexture);
  glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, askedSamples, GL_RGBA16F, width, height, GL_TRUE);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

  // Depth: only used while drawing, so a renderbuffer is enough
  // It needs the same number of samples as the color.
  glGenRenderbuffers(1, &target.depthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, target.depthBuffer);
  glRenderbufferStorageMultisample(GL_RENDERBUFFER, askedSamples, GL_DEPTH_COMPONENT24, width, height);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  glGenFramebuffers(1, &target.framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                         target.colorTexture, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.depthBuffer);
  bool ok = checkFramebuffer("HDR");

  // The driver may give more samples than asked for, so ask how many the color really has
  // The tone mapping pass reads that many.
  GLint colorSamples = 0;
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, target.colorTexture);
  glGetTexLevelParameteriv(GL_TEXTURE_2D_MULTISAMPLE, 0, GL_TEXTURE_SAMPLES, &colorSamples);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
  target.samples = colorSamples;

  // If it does not work, show what the driver gave each part
  if (!ok) {
    GLint depthSamples = 0;
    glBindRenderbuffer(GL_RENDERBUFFER, target.depthBuffer);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &depthSamples);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    std::printf("Asked for %d samples, got %d for color and %d for depth\n", askedSamples,
                colorSamples, depthSamples);
  }

  // Back to drawing into the window
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return ok;
}


// Make the target again when the window has a new size
bool resizeHdrTarget(HdrTarget& target, int width, int height) {
  if (width == target.width && height == target.height) {
    return true;
  }
  deleteHdrTarget(target);
  return createHdrTarget(target, width, height);
}


// Free everything, deleting 0 is allowed and does nothing
void deleteHdrTarget(HdrTarget& target) {
  glDeleteFramebuffers(1, &target.framebuffer);
  glDeleteTextures(1, &target.colorTexture);
  glDeleteRenderbuffers(1, &target.depthBuffer);
  target = HdrTarget();
}
