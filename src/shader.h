#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <string>
#include <vector>

// Build a shader program from a vertex shader file and a list of fragment shader files
// The fragment files are joined in order after a version line.
// Prints the error and returns 0 if a file is missing or does not compile or link.
GLuint loadShaderProgram(const std::string& folder, const std::string& vertexFile,
                         const std::vector<std::string>& fragmentFiles);

// Build a shader program from one vertex shader file and one fragment shader file
// Each file has its own version line. Returns 0 on an error like the function above.
GLuint loadShaderPair(const std::string& folder, const std::string& vertexFile,
                      const std::string& fragmentFile);

// Set a whole number uniform on the program that is currently in use
void setUniform(GLuint program, const char* name, int value);

// Set a single number uniform
void setUniform(GLuint program, const char* name, float value);

// Set a vector uniform such as a color or a direction
void setUniform(GLuint program, const char* name, const glm::vec3& value);

// Set a 4 by 4 matrix uniform
void setUniform(GLuint program, const char* name, const glm::mat4& value);
