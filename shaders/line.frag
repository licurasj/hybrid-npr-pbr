#version 410 core

// The line keeps the color of its vertices, with no lighting

in vec3 lineColor;

out vec4 fragColor;

void main() {
  fragColor = vec4(lineColor, 1.0);
}
