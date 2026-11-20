#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <cstdio>
#include <fstream>
#include <string>

#include "camera.h"
#include "mesh.h"
#include "settings.h"
#include "shader.h"
#include "ui.h"


// Window size when it is not maximized
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 800;

// Values changed by the control panel
Settings settings;

// The orbit camera
Camera camera;

// Shader files, in the folder that CMake passes in as SHADER_DIR
// On another computer that folder does not exist, so main switches to the shaders folder next to the program.
std::string vertexShaderPath = std::string(SHADER_DIR) + "mesh.vert";
std::string fragmentShaderPath = std::string(SHADER_DIR) + "phong.frag";


// Function declarations

// Mouse input
// Camera and light from mouse drags and the wheel
void handleMouse(GLFWwindow* window);
// Called by GLFW when the mouse wheel turns
void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

// Shaders
// Build the shaders again and keep the old ones if that fails
void reloadShaders(GLuint& program);

// Drawing
// Draw the sphere with the camera and the light
void drawScene(GLuint program, const Mesh& mesh, int width, int height);


// Mouse input

// Last cursor position, to work out how far the mouse moved
double lastMouseX = 0.0;
double lastMouseY = 0.0;

// Mouse wheel turns since the last frame
float scrollAmount = 0.0f;

// How fast the right mouse button turns the light, in degrees per pixel
const float LIGHT_SPEED = 0.3f;

// Left drag orbits the camera, the wheel zooms and right drag turns the light
void handleMouse(GLFWwindow* window) {
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  float dx = (float)(x - lastMouseX);
  float dy = (float)(y - lastMouseY);
  lastMouseX = x;
  lastMouseY = y;
  float scroll = scrollAmount;
  scrollAmount = 0.0f;

  // The panel gets the mouse when the cursor is over it
  if (uiWantsMouse()) {
    return;
  }

  zoomCamera(camera, scroll);

  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    orbitCamera(camera, dx, dy);
  }

  // Yaw wraps around and pitch stops short of straight up or down
  if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
    settings.lightYaw += dx * LIGHT_SPEED;
    if (settings.lightYaw > 180.0f) {
      settings.lightYaw -= 360.0f;
    }
    if (settings.lightYaw < -180.0f) {
      settings.lightYaw += 360.0f;
    }
    settings.lightPitch = glm::clamp(settings.lightPitch - dy * LIGHT_SPEED, -89.0f, 89.0f);
  }
}

// Remember how far the mouse wheel turned
// The zoom happens in handleMouse, once the panel has said whether it has the mouse.
void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
  scrollAmount += (float)yOffset;
}


// Shaders

// Build the shaders again from the files
// If the new version has an error, the old program stays in use.
void reloadShaders(GLuint& program) {
  GLuint newProgram = loadShaderProgram(vertexShaderPath, fragmentShaderPath);
  if (newProgram == 0) {
    std::printf("Keeping the old shaders\n");
    return;
  }
  glDeleteProgram(program);
  program = newProgram;
  std::printf("Shaders reloaded\n");
}


// Drawing

// Draw the mesh with the camera and the light
void drawScene(GLuint program, const Mesh& mesh, int width, int height) {
  // Guard against a height of 0 when the window is minimized
  float aspect = (height > 0) ? (float)width / height : 1.0f;

  glUseProgram(program);
  setUniform(program, "uModel", glm::mat4(1.0f));
  setUniform(program, "uView", cameraView(camera));
  setUniform(program, "uProjection", cameraProjection(aspect));
  setUniform(program, "uCameraPos", cameraPosition(camera));

  // The light direction points from the surface toward the light
  setUniform(program, "uLightDir", directionFromAngles(settings.lightYaw, settings.lightPitch));
  setUniform(program, "uBaseColor", settings.baseColor);
  setUniform(program, "uAmbient", settings.ambient);
  setUniform(program, "uDiffuse", settings.diffuse);
  setUniform(program, "uSpecular", settings.specular);
  setUniform(program, "uShininess", settings.shininess);
  drawMesh(mesh);
}


// Open the window, load everything once and run the frame loop
int main() {
  // Start GLFW
  if (!glfwInit()) {
    std::printf("Could not start GLFW\n");
    return 1;
  }

  // Ask for an OpenGL 4.1 core context
  // 4.1 is the newest version a Mac supports, and the Mac also needs forward compatibility.
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  // Open as a normal maximized window so the scene gets most of the screen
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

  // Four samples per pixel smooth the jagged edge of the sphere
  glfwWindowHint(GLFW_SAMPLES, 4);

  // Scale the window and the panel for high resolution screens
  float monitorScale = getMonitorScale();
  int startWidth = (int)(WINDOW_WIDTH * monitorScale);
  int startHeight = (int)(WINDOW_HEIGHT * monitorScale);
  GLFWwindow* window = glfwCreateWindow(startWidth, startHeight, "Viewer", nullptr, nullptr);
  if (window == nullptr) {
    std::printf("Could not open a window with OpenGL 4.1\n");
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  // Load the OpenGL functions
  if (!gladLoadGL(glfwGetProcAddress)) {
    std::printf("Could not load OpenGL\n");
    glfwTerminate();
    return 1;
  }

  // Show the OpenGL version in the title
  std::string title = std::string("Viewer - OpenGL ") + (const char*)glGetString(GL_VERSION);
  glfwSetWindowTitle(window, title.c_str());
  std::printf("%s\n", title.c_str());

  // Our scroll callback goes in before the panel
  // so the panel passes wheel events on to it.
  glfwSetScrollCallback(window, scrollCallback);
  setupUi(window, monitorScale);
  glfwGetCursorPos(window, &lastMouseX, &lastMouseY);

  // Use the shaders folder in the current folder when the source folder is not on this computer
  if (!std::ifstream(vertexShaderPath)) {
    vertexShaderPath = "shaders/mesh.vert";
    fragmentShaderPath = "shaders/phong.frag";
  }

  // Load the shaders
  GLuint program = loadShaderProgram(vertexShaderPath, fragmentShaderPath);
  if (program == 0) {
    std::printf("Could not load the shaders, stopping\n");
    shutdownUi();
    glfwTerminate();
    return 1;
  }
  bool reloadKeyWasDown = false;

  // The sphere we light
  Mesh sphere = createSphere(64, 128);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // While the window is minimized there is nothing to draw, so wait for events
    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    if (framebufferWidth == 0 || framebufferHeight == 0) {
      glfwWaitEvents();
      continue;
    }

    // Build the panel first so it knows whether it has the mouse this frame
    // before the camera and the keys look at the input.
    buildUi(settings, camera);

    // Keys are ignored while a number is being typed into the panel
    bool typing = uiWantsKeyboard();

    // Esc closes the window
    if (!typing && glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    handleMouse(window);

    // R reloads the shaders once per key press, not every frame it is held
    bool reloadKeyDown = !typing && glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    if (reloadKeyDown && !reloadKeyWasDown) {
      reloadShaders(program);
    }
    reloadKeyWasDown = reloadKeyDown;

    // Use the framebuffer size, which differs from the window size on Mac Retina screens
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    // The scene gets the space left of the panel
    // The panel width is in window units, so scale it to framebuffer pixels.
    float pixelsPerUnit = (windowWidth > 0) ? (float)width / windowWidth : 1.0f;
    int sceneWidth = width - (int)(getPanelWidth() * pixelsPerUnit);
    if (sceneWidth < 1) {
      sceneWidth = 1;
    }

    // Neutral dark gray background, a little darker than the panel
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, sceneWidth, height);
    drawScene(program, sphere, sceneWidth, height);

    // The panel is drawn last so it sits on top
    drawUi();

    glfwSwapBuffers(window);
  }

  shutdownUi();
  glfwTerminate();
  return 0;
}
