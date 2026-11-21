#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

#include "material.h"
#include "mesh.h"

// One piece of a model: a mesh and the material it is drawn with
struct ModelPart {
  Mesh mesh;
  Material material;
};

// A model read from a file, as a plain list of mesh and material pairs
// The node transforms of the file are already applied to the vertex positions,
// so every part is drawn with the same model matrix.
struct Model {
  std::vector<ModelPart> parts;

  // Smallest and largest corner of a box around all parts, used to center and scale the model
  glm::vec3 boundsMin = glm::vec3(0.0f);
  glm::vec3 boundsMax = glm::vec3(0.0f);

  // Every texture the model made, so they can be freed once even when materials share them
  std::vector<GLuint> textures;

  // Why the file could not be loaded, empty when it loaded
  std::string error;
};

// Read a model file (glTF, GLB, OBJ or FBX)
// It prints what it found: the materials, the textures and the number of meshes.
// If the file is missing or broken, a message is printed, the model has no parts
// and its error says what went wrong.
Model loadModel(const std::string& path);

// Free the buffers and textures of a model
void deleteModel(Model& model);
