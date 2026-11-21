#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include "texture.h"

// How the alpha (transparency) value of a material is used, as in glTF
// Opaque ignores it, Mask cuts away pixels below the cutoff and Blend mixes with what is behind.
enum AlphaMode { ALPHA_OPAQUE, ALPHA_MASK, ALPHA_BLEND };

// One texture of a material and how to read it
// The image is uploaded with these settings already, so they are kept here only to look at.
// uvSet says which set of texture coordinates of the mesh it uses: 0 or 1.
struct MaterialTexture {
  GLuint id = 0;
  int uvSet = 0;
  TextureSettings settings;
};

// Everything one material of a model file says about its surface
// These are the glTF material inputs. For now the renderer uses the base color and the
// metallic and roughness values; the others are loaded so they are ready for later.
// Each texture is multiplied by its factor in the shader.
struct Material {
  // Base color and its texture (RGBA)
  glm::vec4 baseColorFactor = glm::vec4(1.0f);
  MaterialTexture baseColorTexture;

  // glTF keeps roughness in the green channel and metallic in the blue channel of one texture
  float metallicFactor = 1.0f;
  float roughnessFactor = 1.0f;
  MaterialTexture metallicRoughnessTexture;

  // Normal map, id 0 when the material has none (a white texture would bend the normals)
  MaterialTexture normalTexture;
  float normalScale = 1.0f;

  // Ambient occlusion in the red channel: how much light reaches into creases
  MaterialTexture occlusionTexture;
  float occlusionStrength = 1.0f;

  // Light the surface gives off by itself
  glm::vec3 emissiveFactor = glm::vec3(0.0f);
  MaterialTexture emissiveTexture;

  AlphaMode alphaMode = ALPHA_OPAQUE;
  float alphaCutoff = 0.5f;
  bool doubleSided = false;
};
