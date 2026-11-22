#version 410 core

// One triangle that covers the whole screen, with no vertex buffer
// The three corners come from the vertex number: (-1, -1), (3, -1) and (-1, 3) on the screen.
// The parts outside the screen are cut away, and what is left is exactly the screen.
// The fragment shader finds its pixel from gl_FragCoord, so nothing else is passed on.

void main() {
  vec2 corner = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
  gl_Position = vec4(corner * 2.0 - 1.0, 0.0, 1.0);
}
