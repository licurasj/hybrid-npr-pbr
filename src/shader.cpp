#include "shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <cctype>
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


// Print a compile log with file names instead of string numbers
// The driver numbers the source strings from 0 and starts each error line with the number
// of the string it is in, so that number is replaced by the name of the string.
static void printLog(const std::string& log, const std::vector<std::string>& names) {
  std::istringstream lines(log);
  std::string line;
  while (std::getline(lines, line)) {
    // Some drivers write ERROR or WARNING in front of the number
    size_t start = 0;
    if (line.rfind("ERROR: ", 0) == 0) {
      start = 7;
    } else if (line.rfind("WARNING: ", 0) == 0) {
      start = 9;
    }

    // Read the number, which is followed by an opening bracket or a colon
    size_t end = start;
    while (end < line.size() && std::isdigit((unsigned char)line[end])) {
      end++;
    }
    // A number longer than four digits is not a string number, so the line is left as it is
    bool shortNumber = end > start && end - start <= 4;
    if (shortNumber && end < line.size() && (line[end] == '(' || line[end] == ':')) {
      int number = std::stoi(line.substr(start, end - start));
      if (number < (int)names.size()) {
        line = line.substr(0, start) + names[number] + line.substr(end);
      }
    }
    std::printf("%s\n", line.c_str());
  }
}


// Compile one shader stage from several source strings
// names holds a name for each string, used in the error log. Returns 0 on failure.
static GLuint compileShader(GLenum type, const std::vector<std::string>& sources,
                            const std::vector<std::string>& names) {
  // glShaderSource takes the strings as a list of pointers
  std::vector<const char*> texts;
  for (const std::string& source : sources) {
    texts.push_back(source.c_str());
  }

  GLuint shader = glCreateShader(type);
  glShaderSource(shader, (GLsizei)texts.size(), texts.data(), nullptr);
  glCompileShader(shader);

  GLint ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[4096];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    std::printf("Error compiling %s\n", type == GL_VERTEX_SHADER ? "the vertex shader" : "the fragment shader");
    printLog(log, names);
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}


// Link a vertex and a fragment shader into a program
// Returns 0 if either shader failed to compile or the link fails.
static GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader, const std::string& folder) {
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
    std::printf("Error linking the shaders in %s\n%s\n", folder.c_str(), log);
    glDeleteProgram(program);
    return 0;
  }
  return program;
}


// Build a shader program from a vertex shader file and a list of fragment shader files
GLuint loadShaderProgram(const std::string& folder, const std::string& vertexFile,
                         const std::vector<std::string>& fragmentFiles) {
  // The vertex shader is one file with its own version line
  std::string vertexSource;
  if (!readFile(folder + vertexFile, vertexSource)) {
    return 0;
  }

  // The fragment files have no version line, so it goes in front as string 0
  std::vector<std::string> fragmentSources = { "#version 410 core\n" };
  std::vector<std::string> fragmentNames = { "start of shader or unknown file" };
  for (const std::string& file : fragmentFiles) {
    std::string text;
    if (!readFile(folder + file, text)) {
      return 0;
    }

    // Some editors save a byte order mark at the start of the file, which the compiler cannot read
    if (text.rfind("\xEF\xBB\xBF", 0) == 0) {
      text = text.substr(3);
    }

    // Some drivers count lines across all the strings as if they were one,
    // so a line directive restarts the count at 1 and sets the string number for each file.
    // The newline at the end keeps the next file's line directive on its own line
    // when a file was saved without a final newline.
    std::string number = std::to_string(fragmentSources.size());
    fragmentSources.push_back("#line 1 " + number + "\n" + text + "\n");
    fragmentNames.push_back(file);
  }

  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, { vertexSource }, { vertexFile });
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSources, fragmentNames);
  return linkProgram(vertexShader, fragmentShader, folder);
}


// Build a shader program from one vertex file and one fragment file
// Both files have their own version line, so nothing is added in front.
GLuint loadShaderPair(const std::string& folder, const std::string& vertexFile,
                      const std::string& fragmentFile) {
  std::string vertexSource;
  std::string fragmentSource;
  if (!readFile(folder + vertexFile, vertexSource) || !readFile(folder + fragmentFile, fragmentSource)) {
    return 0;
  }

  GLuint vertexShader = compileShader(GL_VERTEX_SHADER, { vertexSource }, { vertexFile });
  GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, { fragmentSource }, { fragmentFile });
  return linkProgram(vertexShader, fragmentShader, folder);
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
