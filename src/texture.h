#pragma once

#include <glad/gl.h>

#include <string>

// How a texture is read, with the OpenGL values (glTF files use the same numbers)
// Wrap says what happens to texture coordinates outside 0 to 1: GL_REPEAT, GL_CLAMP_TO_EDGE
// or GL_MIRRORED_REPEAT. Filter says how pixels are blended: GL_NEAREST takes the closest
// pixel (sharp, blocky), GL_LINEAR blends the closest four, and the MIPMAP ones also use the
// smaller copies of the image. The defaults are what we use when a file says nothing.
struct TextureSettings {
  GLint wrapS = GL_REPEAT;                    // along U
  GLint wrapT = GL_REPEAT;                    // along V
  GLint minFilter = GL_LINEAR_MIPMAP_LINEAR;  // when the texture is drawn smaller than it is
  GLint magFilter = GL_LINEAR;                // when the texture is drawn larger than it is

  // True for textures that hold colors (base color, emissive). They are stored as sRGB, so the
  // GPU turns them into linear values when the shader reads them. Textures that hold data
  // (metallic-roughness, normal, occlusion) stay false and are read as they are.
  bool srgb = false;

  // Smaller copies of the image for when the texture is drawn far away. A texture without them
  // needs a min filter that does not use them, GL_NEAREST or GL_LINEAR.
  bool mipmaps = true;
};

// All textures are stored as 8 bits per channel RGBA.
// Images are not flipped: their first row goes into the first row of the texture.
// Each function returns 0 if the image could not be read, after printing why.

// Load an image file such as a PNG or JPEG into a texture
GLuint loadTextureFile(const std::string& path, const TextureSettings& settings);

// Load an image file that is already in memory, for example one stored inside a .glb file
// The name is only used in the printed messages.
GLuint loadTextureMemory(const unsigned char* data, int size, const std::string& name,
                         const TextureSettings& settings);

// Make a texture from RGBA pixels, 4 bytes per pixel, first row first
GLuint createTexture(const unsigned char* pixels, int width, int height, const TextureSettings& settings);

// Make a 1 x 1 white texture, used when a material has no texture of some kind
// so the shader can always read a texture and multiply it by the factor.
GLuint createWhiteTexture();


// Environment images

// Width and height of each cube face made from an environment image, in pixels
// A 2048 x 1024 panorama has about as many pixels around its middle as four faces of 512.
const int ENVIRONMENT_FACE_SIZE = 512;

// A cubemap made from an environment image, and how many mip levels it has
// GLSL 4.10 has no function that asks a texture for its number of levels,
// so the program keeps the count and passes it to the shaders.
// The irradiance and prefiltered maps are made from the cubemap at startup, see environment.h.
// They stay 0 until then.
struct EnvironmentMap {
  GLuint cubemap = 0;
  int mipCount = 0;
  GLuint irradiance = 0;       // fill light for each normal direction
  GLuint prefiltered = 0;      // reflections, one mip level per roughness step
  int prefilteredMipCount = 0;
};

// Load a .hdr panorama (an equirectangular image: 360 degrees across, 180 degrees down)
// and turn it into a cubemap with faceSize x faceSize pixels per face and a full mip chain.
// The values stay linear floats, stored as 16-bit floats.
// Returns a map with cubemap 0 if the file could not be read, after printing why.
EnvironmentMap loadEnvironmentMap(const std::string& path, int faceSize);

// Make a 1 x 1 cubemap of one linear color, used when no environment image can be read
EnvironmentMap createPlainEnvironmentMap(float value);

// Free the cubemap and the maps made from it
void deleteEnvironmentMap(EnvironmentMap& map);
