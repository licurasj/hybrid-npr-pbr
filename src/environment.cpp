#include "environment.h"

#include <chrono>
#include <cstdio>
#include <vector>

#include "shader.h"


// Sizes and sample counts of the startup maps

// The irradiance map changes very slowly with the direction, so small faces are enough.
// 32 x 32 is the size LearnOpenGL uses.
const int IRRADIANCE_FACE_SIZE = 32;
// 2048 samples per pixel, as in the Khronos glTF Sample Renderer
const int IRRADIANCE_SAMPLES = 2048;

// The prefiltered map starts at the full size of the environment cubemap, so a mirror (level 0)
// stays as sharp as the sky behind it, and halves with each level down to 16 x 16.
// Level i is drawn for roughness i / (levels - 1): 0, 0.2, 0.4, 0.6, 0.8 and 1.
const int PREFILTERED_LEVELS = 6;
// 1024 samples per pixel, as in the Khronos glTF Sample Renderer and LearnOpenGL
const int PREFILTERED_SAMPLES = 1024;

// The lookup table is smooth, so 512 x 512 (the LearnOpenGL size) is plenty
const int LOOKUP_TABLE_SIZE = 512;
const int LOOKUP_TABLE_SAMPLES = 1024;


// The fragment shader of one map, joined after the shared sampling code
static GLuint loadMapProgram(const std::string& shaderFolder, const std::string& file) {
  std::vector<std::string> files = { "sampling.glsl", file };
  return loadShaderProgram(shaderFolder, "fullscreen.vert", files);
}


// Make an empty cubemap of 16-bit floats with the given number of mip levels
// Every level of every face gets its storage now, so the shaders can draw into it.
static GLuint createEmptyCubemap(int size, int levels) {
  GLuint cubemap;
  glGenTextures(1, &cubemap);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
  for (int level = 0; level < levels; level++) {
    int levelSize = size >> level;
    for (int f = 0; f < 6; f++) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, level, GL_RGBA16F, levelSize, levelSize, 0, GL_RGBA,
                   GL_FLOAT, nullptr);
    }
  }
  // The texture has only these levels, not a full chain down to 1 x 1
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, levels - 1);
  GLint minFilter = (levels > 1) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minFilter);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  return cubemap;
}


// Check that OpenGL can draw into what is attached to the bound framebuffer
// RGBA16F and RG16F are formats OpenGL 4.1 must be able to draw into, so this should never fail,
// but if a driver refuses, the drawing would do nothing and the map would stay empty without a word.
static bool framebufferReady(const char* what) {
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    std::printf("Could not draw into the %s (framebuffer status 0x%x)\n", what, status);
    return false;
  }
  return true;
}


// Draw the program into all six faces of one mip level of a cubemap
// The framebuffer is bound already. Each face is attached in turn and gets one full-screen
// triangle, so the GPU works through one face at a time.
// Returns false if a face cannot be drawn into.
static bool drawCubemapLevel(GLuint program, GLuint cubemap, int level, int size, const char* what) {
  glViewport(0, 0, size, size);
  setUniform(program, "uFaceSize", (float)size);
  for (int f = 0; f < 6; f++) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, cubemap,
                           level);
    if (!framebufferReady(what)) {
      return false;
    }
    setUniform(program, "uFace", f);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
  return true;
}


// Build the irradiance map and the prefiltered map
bool buildEnvironmentLighting(EnvironmentMap& map, const std::string& shaderFolder, GLuint emptyVertexArray) {
  auto start = std::chrono::steady_clock::now();

  // Both shaders are loaded first. If one has an error, nothing is touched, so after a shader
  // reload the maps that were there stay in use.
  GLuint irradianceProgram = loadMapProgram(shaderFolder, "irradiance.frag");
  GLuint prefilterProgram = loadMapProgram(shaderFolder, "prefilter.frag");
  if (irradianceProgram == 0 || prefilterProgram == 0) {
    glDeleteProgram(irradianceProgram);
    glDeleteProgram(prefilterProgram);
    std::printf("Could not build the environment lighting maps, the old ones (if any) stay\n");
    return false;
  }

  // The source is read on texture unit 0 while the maps are drawn
  // The size of its level 0 tells the shaders which level each sample should read.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, map.cubemap);
  GLint sourceSize = 0;
  glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_TEXTURE_WIDTH, &sourceSize);

  // A framebuffer of our own to draw into the faces, and the full-screen triangle
  GLuint framebuffer;
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glBindVertexArray(emptyVertexArray);

  // Irradiance map, one level
  GLuint irradiance = createEmptyCubemap(IRRADIANCE_FACE_SIZE, 1);
  glUseProgram(irradianceProgram);
  setUniform(irradianceProgram, "uSource", 0);
  setUniform(irradianceProgram, "uSourceSize", (float)sourceSize);
  setUniform(irradianceProgram, "uSampleCount", IRRADIANCE_SAMPLES);
  glBindTexture(GL_TEXTURE_CUBE_MAP, map.cubemap);
  bool drawn = drawCubemapLevel(irradianceProgram, irradiance, 0, IRRADIANCE_FACE_SIZE, "irradiance map");

  // Prefiltered map, one level per roughness step
  // A source smaller than 32 x 32 (the plain gray fallback) gets fewer levels.
  int levels = 1;
  while (levels < PREFILTERED_LEVELS && (sourceSize >> levels) >= 1) {
    levels++;
  }
  GLuint prefiltered = createEmptyCubemap(sourceSize, levels);
  glUseProgram(prefilterProgram);
  setUniform(prefilterProgram, "uSource", 0);
  setUniform(prefilterProgram, "uSourceSize", (float)sourceSize);
  setUniform(prefilterProgram, "uSampleCount", PREFILTERED_SAMPLES);
  glBindTexture(GL_TEXTURE_CUBE_MAP, map.cubemap);
  for (int level = 0; level < levels && drawn; level++) {
    float roughness = (levels > 1) ? (float)level / (levels - 1) : 0.0f;
    setUniform(prefilterProgram, "uRoughness", roughness);
    drawn = drawCubemapLevel(prefilterProgram, prefiltered, level, sourceSize >> level, "prefiltered map");
  }

  // Put the state back to plain defaults
  // The viewport stays at the last map size: the frame code sets its own viewport and framebuffer
  // before it draws anything, so that is fine.
  glUseProgram(0);
  glDeleteProgram(irradianceProgram);
  glDeleteProgram(prefilterProgram);
  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &framebuffer);

  // The old maps are replaced either way
  glDeleteTextures(1, &map.irradiance);
  glDeleteTextures(1, &map.prefiltered);
  if (!drawn) {
    // Half-drawn maps would light the model wrongly, so none are kept. With no maps the
    // drawing code uses the simple method.
    glDeleteTextures(1, &irradiance);
    glDeleteTextures(1, &prefiltered);
    map.irradiance = 0;
    map.prefiltered = 0;
    map.prefilteredMipCount = 0;
    std::printf("The environment lighting uses the simple method instead\n");
    return false;
  }
  map.irradiance = irradiance;
  map.prefiltered = prefiltered;
  map.prefilteredMipCount = levels;

  // Wait for the GPU to finish, so the time printed is the real building time
  glFinish();
  double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  std::printf("  Environment lighting maps: irradiance %d x %d, prefiltered %d levels from %d x %d, %.0f ms\n",
              IRRADIANCE_FACE_SIZE, IRRADIANCE_FACE_SIZE, map.prefilteredMipCount, sourceSize, sourceSize, ms);
  return true;
}


// Build the BRDF lookup table
GLuint createBrdfLookupTable(const std::string& shaderFolder, GLuint emptyVertexArray) {
  auto start = std::chrono::steady_clock::now();
  GLuint program = loadMapProgram(shaderFolder, "brdf_lut.frag");
  if (program == 0) {
    std::printf("Could not build the BRDF lookup table\n");
    return 0;
  }

  // Two channels of 16-bit floats, A and B, a format OpenGL 4.1 lets us draw into
  GLuint table;
  glGenTextures(1, &table);
  glBindTexture(GL_TEXTURE_2D, table);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, LOOKUP_TABLE_SIZE, LOOKUP_TABLE_SIZE, 0, GL_RG, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  GLuint framebuffer;
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, table, 0);

  bool drawn = framebufferReady("BRDF lookup table");
  if (drawn) {
    glViewport(0, 0, LOOKUP_TABLE_SIZE, LOOKUP_TABLE_SIZE);
    glUseProgram(program);
    setUniform(program, "uTableSize", (float)LOOKUP_TABLE_SIZE);
    setUniform(program, "uSampleCount", LOOKUP_TABLE_SAMPLES);
    glBindVertexArray(emptyVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  // Put the state back to plain defaults
  // The viewport stays at the table size: the frame code sets its own viewport and framebuffer
  // before it draws anything, so that is fine.
  glUseProgram(0);
  glBindVertexArray(0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glDeleteFramebuffers(1, &framebuffer);
  glDeleteProgram(program);

  // An empty table would make the reflections black, so it is thrown away. With no table the
  // drawing code uses the simple method.
  if (!drawn) {
    glDeleteTextures(1, &table);
    std::printf("Could not build the BRDF lookup table\n");
    return 0;
  }

  // Wait for the GPU to finish, so the time printed is the real building time
  glFinish();
  double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  std::printf("  BRDF lookup table: %d x %d, %d samples per pixel, %.0f ms\n", LOOKUP_TABLE_SIZE,
              LOOKUP_TABLE_SIZE, LOOKUP_TABLE_SAMPLES, ms);
  return table;
}
