#version 410 core

// The line keeps the color of its vertices, with no lighting
// Alpha 0 marks the pixel as not part of the scene, so the last pass gives it
// only the sRGB step and no exposure or tone curve.

in vec3 lineColor;

out vec4 fragColor;

void main() {
  fragColor = vec4(lineColor, 0.0);
}
