#include "mesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <vector>


// Make a unit sphere with the given number of rows and columns
// Rows go from the top pole to the bottom pole and columns go around the middle.
Mesh createSphere(int stacks, int slices) {
  // Each vertex is 8 floats, position then normal then texture coordinate
  std::vector<float> vertices;
  for (int i = 0; i <= stacks; i++) {
    float theta = glm::pi<float>() * i / stacks;
    for (int j = 0; j <= slices; j++) {
      float phi = 2.0f * glm::pi<float>() * j / slices;
      glm::vec3 p(glm::sin(theta) * glm::cos(phi), glm::cos(theta), glm::sin(theta) * glm::sin(phi));

      // On a unit sphere the normal is the same as the position
      vertices.push_back(p.x);
      vertices.push_back(p.y);
      vertices.push_back(p.z);
      vertices.push_back(p.x);
      vertices.push_back(p.y);
      vertices.push_back(p.z);
      vertices.push_back((float)j / slices);
      vertices.push_back((float)i / stacks);
    }
  }

  // Two triangles for each grid cell
  // The corners are listed counterclockwise as seen from outside.
  std::vector<unsigned int> indices;
  for (int i = 0; i < stacks; i++) {
    for (int j = 0; j < slices; j++) {
      unsigned int a = i * (slices + 1) + j;
      unsigned int b = a + slices + 1;
      indices.push_back(a);
      indices.push_back(a + 1);
      indices.push_back(b);
      indices.push_back(a + 1);
      indices.push_back(b + 1);
      indices.push_back(b);
    }
  }

  // Upload to the GPU
  Mesh mesh;
  mesh.indexCount = (int)indices.size();
  glGenVertexArrays(1, &mesh.vao);
  glBindVertexArray(mesh.vao);

  glGenBuffers(1, &mesh.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

  glGenBuffers(1, &mesh.ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

  // Attribute 0 is the position, 1 the normal and 2 the texture coordinate
  GLsizei stride = 8 * sizeof(float);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);

  glBindVertexArray(0);
  return mesh;
}


// Draw the mesh with the shader program that is in use
void drawMesh(const Mesh& mesh) {
  glBindVertexArray(mesh.vao);
  glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, (void*)0);
}


// Add one line from a to b in one color
// Each vertex is 6 floats, position then color.
static void addLine(std::vector<float>& vertices, glm::vec3 a, glm::vec3 b, glm::vec3 color) {
  glm::vec3 ends[2] = { a, b };
  for (int i = 0; i < 2; i++) {
    vertices.push_back(ends[i].x);
    vertices.push_back(ends[i].y);
    vertices.push_back(ends[i].z);
    vertices.push_back(color.r);
    vertices.push_back(color.g);
    vertices.push_back(color.b);
  }
}


// Upload a list of line vertices to the GPU
static LineMesh uploadLines(const std::vector<float>& vertices) {
  LineMesh lines;
  lines.vertexCount = (int)vertices.size() / 6;
  glGenVertexArrays(1, &lines.vao);
  glBindVertexArray(lines.vao);

  glGenBuffers(1, &lines.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, lines.vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

  // Attribute 0 is the position and 1 the color
  GLsizei stride = 6 * sizeof(float);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
  return lines;
}


// Make a square grid of lines on the floor with one unit between lines
// The line along X through the origin is reddish and the one along Z is bluish,
// muted so they do not stand out more than the sphere.
LineMesh createGrid(int size, float height) {
  glm::vec3 gray(0.30f, 0.30f, 0.30f);
  glm::vec3 xColor(0.55f, 0.27f, 0.27f);
  glm::vec3 zColor(0.27f, 0.36f, 0.58f);
  float half = size / 2.0f;

  std::vector<float> vertices;
  for (int i = 0; i <= size; i++) {
    float offset = i - half;

    // A line along X at this Z
    glm::vec3 color = (offset == 0.0f) ? xColor : gray;
    addLine(vertices, glm::vec3(-half, height, offset), glm::vec3(half, height, offset), color);

    // A line along Z at this X
    color = (offset == 0.0f) ? zColor : gray;
    addLine(vertices, glm::vec3(offset, height, -half), glm::vec3(offset, height, half), color);
  }
  return uploadLines(vertices);
}


// Free the buffers of a mesh
void deleteMesh(Mesh& mesh) {
  glDeleteBuffers(1, &mesh.vbo);
  glDeleteBuffers(1, &mesh.ebo);
  glDeleteVertexArrays(1, &mesh.vao);
}


// Make three lines from the origin along X, Y and Z
LineMesh createAxes(float length) {
  std::vector<float> vertices;
  glm::vec3 origin(0.0f);
  addLine(vertices, origin, glm::vec3(length, 0.0f, 0.0f), glm::vec3(0.90f, 0.30f, 0.30f));
  addLine(vertices, origin, glm::vec3(0.0f, length, 0.0f), glm::vec3(0.40f, 0.80f, 0.30f));
  addLine(vertices, origin, glm::vec3(0.0f, 0.0f, length), glm::vec3(0.30f, 0.50f, 1.00f));
  return uploadLines(vertices);
}


// Draw the lines with the shader program that is in use
// The lines are always 1 pixel wide. A Mac only gives an OpenGL 4.1 core context
// with forward compatibility, and there glLineWidth above 1 is not allowed.
// On high density screens such as a Mac Retina screen the lines look thinner.
void drawLines(const LineMesh& lines) {
  glBindVertexArray(lines.vao);
  glDrawArrays(GL_LINES, 0, lines.vertexCount);
}


// Free the buffers of a set of lines
void deleteLines(LineMesh& lines) {
  glDeleteBuffers(1, &lines.vbo);
  glDeleteVertexArrays(1, &lines.vao);
}
