#pragma once

#include <glad/gl.h>

#include <string>

#include "texture.h"

// The startup maps of the environment lighting (the split sum of Karis 2013)
// Each one is drawn by a fragment shader into a texture, one face and one mip level at a time.
// OpenGL 4.1 has no compute shaders, so drawing a full-screen triangle is how the GPU is used here.
// The shaders are read from the shader folder each time, so an environment switch in the panel
// builds the maps with the current version of the files.

// Build the irradiance map and the prefiltered map from map.cubemap
// emptyVertexArray is the vertex array the full-screen triangle is drawn with. Leaves the
// window's framebuffer bound and no program in use. Returns true when new maps are in place.
// If a shader does not load, nothing changes: the old maps stay (none after an environment
// switch, which deleted them). If OpenGL cannot draw into a map, the old and new maps are all
// deleted. With no maps the drawing code falls back to the simple method.
bool buildEnvironmentLighting(EnvironmentMap& map, const std::string& shaderFolder, GLuint emptyVertexArray);

// Build the BRDF lookup table, which is the same for every environment, so it is made at startup
// and again when the shaders are reloaded. Returns 0 if its shader does not load or OpenGL cannot
// draw into it.
GLuint createBrdfLookupTable(const std::string& shaderFolder, GLuint emptyVertexArray);
