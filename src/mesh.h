#pragma once

#include <glad/gl.h>

#include <vector>

// Number of floats in one vertex: position 3, normal 3, texture coordinate 2, tangent 4,
// second texture coordinate 2
const int FLOATS_PER_VERTEX = 14;

// One drawable mesh on the GPU
// Each vertex has a position, a normal, a texture coordinate, a tangent and a second texture
// coordinate. glTF lets each texture pick one of two sets of texture coordinates. A mesh with
// only one set gets a copy of it as the second set, so the shader can always read both.
// The tangent points along the texture's U direction. Its fourth number is 1 or -1 and says
// which way the bitangent points, the same way glTF stores tangents. Normal mapping needs it later.
struct Mesh {
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ebo = 0;
  int indexCount = 0;
};

// Upload vertices (FLOATS_PER_VERTEX floats each) and triangle indices to the GPU
Mesh createMesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);

// Make a unit sphere with the given number of rows and columns
Mesh createSphere(int stacks, int slices);

// Draw the mesh with the shader program that is in use
void drawMesh(const Mesh& mesh);

// Free the buffers of a mesh
void deleteMesh(Mesh& mesh);

// A set of colored lines on the GPU, used for the floor grid and the axis indicator
// Each vertex has a position and a color, and every two vertices make one line.
struct LineMesh {
  GLuint vao = 0;
  GLuint vbo = 0;
  int vertexCount = 0;
};

// Make a square grid of lines on the floor, from minus half to plus half the size in X and Z
LineMesh createGrid(int size, float height);

// Make three lines from the origin along X, Y and Z, in red, green and blue
LineMesh createAxes(float length);

// Draw the lines with the shader program that is in use
void drawLines(const LineMesh& lines);

// Free the buffers of a set of lines
void deleteLines(LineMesh& lines);
