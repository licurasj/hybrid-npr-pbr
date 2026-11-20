#include "shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstdio>
#include <fstream>
#include <sstream>


// Read a whole text file into a string
// Returns false if the file cannot be opened.
static bool readFile(const std::string& path, std::string& text) {
  std::ifstream file(path);
  if (!file) {
    std::printf("Could not open %s\n", path.c_str());
    return false;
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  text = buffer.str();
  return true;
}


// Compile one shader stage and print the error log if it fails
// Returns 0 on failure.
static GLuint compileShader(GLenum type, const std::string& source, const std::string& path) {
  GLuint shader = glCreateShader(type);
  const char* text = source.c_str();
  glShaderSource(shader, 1, &text, nullptr);
  glCompileShader(shader);

  GLint ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[2048];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    std::printf("Error compiling %s\n%s\n", path.c_str(), log);
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}


// Build a shader program from a vertex shader file and a fragment shader file
GLuint loadShaderProgram(const std::string& vertexPath, const std::string& fragmentPath) {
  std::string vertexSource;
  std::string fragmentSource;
  if (!readFile(vertexPath, vertexSource) || !readFile(fragmentPath, fragmentSource)) {
    return 0;
  }

  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource, vertexPath);
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);
  if (vertexShader == 0 || fragmentShader == 0) {
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return 0;
  }

  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  // The shaders are part of the program now and can be deleted
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  GLint ok = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[2048];
    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    std::printf("Error linking %s and %s\n%s\n", vertexPath.c_str(), fragmentPath.c_str(), log);
    glDeleteProgram(program);
    return 0;
  }
  return program;
}


// Set a whole number uniform
// Each setter looks up the uniform by name and sets it on the current program.
void setUniform(GLuint program, const char* name, int value) {
  glUniform1i(glGetUniformLocation(program, name), value);
}

// Set a single number uniform
void setUniform(GLuint program, const char* name, float value) {
  glUniform1f(glGetUniformLocation(program, name), value);
}

// Set a vector uniform such as a color or a direction
void setUniform(GLuint program, const char* name, const glm::vec3& value) {
  glUniform3fv(glGetUniformLocation(program, name), 1, glm::value_ptr(value));
}

// Set a 4 by 4 matrix uniform
void setUniform(GLuint program, const char* name, const glm::mat4& value) {
  glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, glm::value_ptr(value));
}
