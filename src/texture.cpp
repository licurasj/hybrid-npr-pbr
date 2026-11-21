#include "texture.h"

#include <cstdio>

// stb_image is a single header library. Its code is compiled here, in this one file only.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


// Make a texture from RGBA pixels, 4 bytes per pixel, first row first
GLuint createTexture(const unsigned char* pixels, int width, int height, const TextureSettings& settings) {
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Smaller copies of the image for when the texture is far away
  // They are made every time, even if the filter below does not use them.
  glGenerateMipmap(GL_TEXTURE_2D);
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
