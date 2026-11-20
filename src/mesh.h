#pragma once

#include <glad/gl.h>

// One drawable mesh on the GPU
// Each vertex has a position, a normal and a texture coordinate.
struct Mesh {
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ebo = 0;
  int indexCount = 0;
};

// Make a unit sphere with the given number of rows and columns
Mesh createSphere(int stacks, int slices);

// Draw the mesh with the shader program that is in use
void drawMesh(const Mesh& mesh);
