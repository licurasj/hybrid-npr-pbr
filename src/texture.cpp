#include "texture.h"

#include <cmath>
#include <cstdio>
#include <vector>

#include <glm/glm.hpp>

// stb_image is a single header library. Its code is compiled here, in this one file only.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


// Make a texture from RGBA pixels, 4 bytes per pixel, first row first
GLuint createTexture(const unsigned char* pixels, int width, int height, const TextureSettings& settings) {
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  // Color textures are stored as sRGB and data textures as plain values
  GLint format = settings.srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Smaller copies of the image for when the texture is far away
  // They are made whenever the settings ask for them, even if the filter below does not use them.
  if (settings.mipmaps) {
    glGenerateMipmap(GL_TEXTURE_2D);
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, settings.minFilter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, settings.magFilter);

  // What happens to texture coordinates outside 0 to 1
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, settings.wrapS);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, settings.wrapT);

  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}


// Load an image file into a texture
GLuint loadTextureFile(const std::string& path, const TextureSettings& settings) {
  // The last argument asks stb for 4 channels, so every image comes back as RGBA
  int width, height, channels;
  unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
  if (pixels == nullptr) {
    std::printf("Could not read the image %s: %s\n", path.c_str(), stbi_failure_reason());
    return 0;
  }
  std::printf("  Texture %s: %d x %d, %d channels in the file\n", path.c_str(), width, height, channels);

  GLuint texture = createTexture(pixels, width, height, settings);
  stbi_image_free(pixels);
  return texture;
}


// Load an image file that is already in memory
GLuint loadTextureMemory(const unsigned char* data, int size, const std::string& name,
                         const TextureSettings& settings) {
  int width, height, channels;
  unsigned char* pixels = stbi_load_from_memory(data, size, &width, &height, &channels, 4);
  if (pixels == nullptr) {
    std::printf("Could not read the image %s: %s\n", name.c_str(), stbi_failure_reason());
    return 0;
  }
  std::printf("  Texture %s (inside the file, %d bytes): %d x %d, %d channels\n", name.c_str(), size, width,
              height, channels);

  GLuint texture = createTexture(pixels, width, height, settings);
  stbi_image_free(pixels);
  return texture;
}


// Make a 1 x 1 white texture
GLuint createWhiteTexture() {
  unsigned char white[4] = { 255, 255, 255, 255 };
  return createTexture(white, 1, 1, TextureSettings());
}


// Environment images

// Largest value a 16-bit float can hold
// The sun in a sky image can be brighter than that, and it would turn into infinity.
const float HALF_FLOAT_MAX = 65504.0f;


// Direction through the middle of one pixel of a cube face
// a and b go from -1 to 1 across the face: a changes with the column (left to right) and b with
// the row (top to bottom).
// The face order and the signs are the OpenGL cubemap layout (the table in the OpenGL 4.1
// specification, section 3.8.10): the first row of each face is b = -1.
static glm::vec3 cubeFaceDirection(int face, float a, float b) {
  glm::vec3 d;
  if (face == 0) {
    d = glm::vec3(1.0f, -b, -a);   // +X
  } else if (face == 1) {
    d = glm::vec3(-1.0f, -b, a);   // -X
  } else if (face == 2) {
    d = glm::vec3(a, 1.0f, b);     // +Y
  } else if (face == 3) {
    d = glm::vec3(a, -1.0f, -b);   // -Y
  } else if (face == 4) {
    d = glm::vec3(a, -b, 1.0f);    // +Z
  } else {
    d = glm::vec3(-a, -b, -1.0f);  // -Z
  }
  return glm::normalize(d);
}


// Read the panorama in a direction, blending the four closest pixels
// u goes once around the horizon and v from straight up (0) to straight down (1).
// stb_image gives the top row of the image first, so v = 0 is the first row.
// Across the image the pixels wrap around, from top to bottom they stop at the edge.
static glm::vec3 samplePanorama(const float* pixels, int width, int height, glm::vec3 d) {
  const float PI = 3.14159265f;
  float u = 0.5f + std::atan2(d.z, d.x) / (2.0f * PI);
  float v = std::acos(glm::clamp(d.y, -1.0f, 1.0f)) / PI;

  // Pixel centers sit at half steps, so 0.5 is taken off before splitting into the whole
  // pixel and the part between two pixels
  float x = u * width - 0.5f;
  float y = v * height - 0.5f;
  int x0 = (int)std::floor(x);
  int y0 = (int)std::floor(y);
  float fx = x - x0;
  float fy = y - y0;

  glm::vec3 result(0.0f);
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < 2; i++) {
      int px = ((x0 + i) % width + width) % width;
      int py = glm::clamp(y0 + j, 0, height - 1);
      float weight = (i == 1 ? fx : 1.0f - fx) * (j == 1 ? fy : 1.0f - fy);
      const float* p = pixels + 3 * (py * width + px);
      result += weight * glm::vec3(p[0], p[1], p[2]);
    }
  }
  return result;
}


// Load a .hdr panorama and turn it into a cubemap
EnvironmentMap loadEnvironmentMap(const std::string& path, int faceSize) {
  EnvironmentMap map;

  // stbi_loadf gives linear floats for .hdr files, with no gamma step,
  // and the last argument asks for 3 channels, red, green and blue
  int width, height, channels;
  float* pixels = stbi_loadf(path.c_str(), &width, &height, &channels, 3);
  if (pixels == nullptr) {
    std::printf("Could not read the environment image %s: %s\n", path.c_str(), stbi_failure_reason());
    return map;
  }

  glGenTextures(1, &map.cubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, map.cubemap);

  // Fill each face pixel by pixel with the panorama color in that pixel's direction
  std::vector<float> face(faceSize * faceSize * 3);
  float brightest = 0.0f;
  for (int f = 0; f < 6; f++) {
    for (int row = 0; row < faceSize; row++) {
      for (int column = 0; column < faceSize; column++) {
        float a = 2.0f * (column + 0.5f) / faceSize - 1.0f;
        float b = 2.0f * (row + 0.5f) / faceSize - 1.0f;
        glm::vec3 color = samplePanorama(pixels, width, height, cubeFaceDirection(f, a, b));
        brightest = glm::max(brightest, glm::max(color.r, glm::max(color.g, color.b)));

        // Values a 16-bit float cannot hold are lowered to its largest value
        color = glm::min(color, glm::vec3(HALF_FLOAT_MAX));
        float* p = &face[3 * (row * faceSize + column)];
        p[0] = color.r;
        p[1] = color.g;
        p[2] = color.b;
      }
    }
    // 16-bit floats like the HDR target, with an alpha channel that is not used
    // RGBA16F is also a format OpenGL 4.1 lets us draw into, which RGB16F is not sure to be.
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_RGBA16F, faceSize, faceSize, 0, GL_RGB, GL_FLOAT,
                 face.data());
  }
  stbi_image_free(pixels);

  // Smaller copies of each face, each one averaging 2 x 2 pixels of the one before, down to 1 x 1
  // A face of 512 gives 10 levels: 512, 256, ... 2, 1.
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
  map.mipCount = 1 + (int)std::floor(std::log2((float)faceSize));

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

  std::printf("  Environment %s: %d x %d, cube faces %d x %d, %d levels, brightest value %.0f\n",
              path.c_str(), width, height, faceSize, faceSize, map.mipCount, brightest);
  return map;
}


// Make a 1 x 1 cubemap of one linear color
EnvironmentMap createPlainEnvironmentMap(float value) {
  EnvironmentMap map;
  float color[3] = { value, value, value };
  glGenTextures(1, &map.cubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, map.cubemap);
  for (int f = 0; f < 6; f++) {
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, 0, GL_RGBA16F, 1, 1, 0, GL_RGB, GL_FLOAT, color);
  }
  map.mipCount = 1;
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  return map;
}


// Free the cubemap and the maps made from it
// Deleting texture 0 does nothing, so maps that were never made are fine.
void deleteEnvironmentMap(EnvironmentMap& map) {
  glDeleteTextures(1, &map.cubemap);
  glDeleteTextures(1, &map.irradiance);
  glDeleteTextures(1, &map.prefiltered);
  map = EnvironmentMap();
}
