#version 410 core

// Unlit lines for the floor grid and the axis indicator
// Each vertex brings its own color.

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

uniform mat4 uMVP;

out vec3 lineColor;

void main() {
  lineColor = color;
  gl_Position = uMVP * vec4(position, 1.0);
}
