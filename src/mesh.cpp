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
