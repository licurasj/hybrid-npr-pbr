#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "camera.h"
#include "color.h"
#include "framebuffer.h"
#include "mesh.h"
#include "model.h"
#include "renderer.h"
#include "settings.h"
#include "shader.h"
#include "texture.h"
#include "ui.h"


// Window size when it is not maximized
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 800;

// Values changed by the control panel
Settings settings;

// The orbit camera
Camera camera;

// Shader folder that CMake passes in as SHADER_DIR
// On another computer that folder does not exist, so main switches to the shaders folder next to the program.
std::string shaderFolder = SHADER_DIR;

// Asset folder that CMake passes in as ASSET_DIR, with the same fallback as the shaders
std::string assetFolder = ASSET_DIR;

// The fragment shader is joined from these files in this order
std::vector<std::string> fragmentFiles = { "common.glsl", "pbr_terms.glsl", "npr_terms.glsl", "hybrid_main.glsl" };


// Function declarations

// Mouse input
// Camera and light from mouse drags and the wheel
void handleMouse(GLFWwindow* window);
// Called by GLFW when the mouse wheel turns
void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

// Shaders
// Build the shader programs again and keep the old one of each that fails
void reloadShaders(GLuint& program, GLuint& lineProgram, GLuint& toneMapProgram);

// Models
// The sphere as a model with one part, drawn with white textures and the panel's material
Model createSphereModel();
// Paths of the model files in a folder, sorted by name
std::vector<std::string> findModelFiles(const std::string& folder);


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
// The sphere shader, the line shader and the tone mapping shader are checked one by one.
void reloadShaders(GLuint& program, GLuint& lineProgram, GLuint& toneMapProgram) {
  GLuint newProgram = loadShaderProgram(shaderFolder, "mesh.vert", fragmentFiles);
  if (newProgram == 0) {
    std::printf("Keeping the old shaders\n");
  } else {
    glDeleteProgram(program);
    program = newProgram;
    std::printf("Shaders reloaded\n");
  }

  GLuint newLineProgram = loadShaderPair(shaderFolder, "line.vert", "line.frag");
  if (newLineProgram == 0) {
    std::printf("Keeping the old line shaders\n");
  } else {
    glDeleteProgram(lineProgram);
    lineProgram = newLineProgram;
    std::printf("Line shaders reloaded\n");
  }

  GLuint newToneMapProgram = loadShaderPair(shaderFolder, "fullscreen.vert", "tonemap.frag");
  if (newToneMapProgram == 0) {
    std::printf("Keeping the old tone mapping shaders\n");
  } else {
    glDeleteProgram(toneMapProgram);
    toneMapProgram = newToneMapProgram;
    std::printf("Tone mapping shaders reloaded\n");
  }
}


// Models

// The sphere as a model with one part
// White 1 x 1 textures make the shader use the factors as they are,
// and the main loop copies the panel's base color, metallic and roughness into them.
Model createSphereModel() {
  Model sphere;
  GLuint white = createWhiteTexture();
  sphere.textures.push_back(white);

  ModelPart part;
  part.mesh = createSphere(64, 128);
  part.material.baseColorTexture.id = white;
  part.material.metallicRoughnessTexture.id = white;
  part.material.occlusionTexture.id = white;
  part.material.emissiveTexture.id = white;
  sphere.parts.push_back(part);

  // A sphere with radius 1 around the origin
  sphere.boundsMin = glm::vec3(-1.0f);
  sphere.boundsMax = glm::vec3(1.0f);
  return sphere;
}


// Paths of the glTF, GLB, OBJ and FBX files in a folder, sorted by name
// A missing folder gives an empty list. Entries that cannot be read are skipped.
std::vector<std::string> findModelFiles(const std::string& folder) {
  std::vector<std::string> paths;
  std::error_code error;
  std::filesystem::directory_iterator it(folder, error);
  std::filesystem::directory_iterator end;
  // With an error code the calls report problems there instead of throwing
  for (; !error && it != end; it.increment(error)) {
    if (!it->is_regular_file(error)) {
      continue;
    }
    // Lower case the extension, so ".GLB" counts too
    // tolower needs a value that fits in unsigned char, so each character is converted first.
    std::string extension = it->path().extension().string();
    for (char& c : extension) {
      c = (char)std::tolower((unsigned char)c);
    }
    if (extension == ".gltf" || extension == ".glb" || extension == ".obj" || extension == ".fbx") {
      paths.push_back(it->path().generic_string());
    }
  }
  std::sort(paths.begin(), paths.end());
  return paths;
}


// Open the window, load everything once and run the frame loop
// A model file can be named on the command line, for example: viewer model.glb
int main(int argc, char** argv) {
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

  // Four samples per pixel in the window too, which smooth the edges of the panel's arrows
  // The scene gets its own samples in the HDR target.
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
  if (!std::ifstream(shaderFolder + "mesh.vert")) {
    shaderFolder = "shaders/";
  }

  // Load the shaders
  GLuint program = loadShaderProgram(shaderFolder, "mesh.vert", fragmentFiles);
  GLuint lineProgram = loadShaderPair(shaderFolder, "line.vert", "line.frag");
  GLuint toneMapProgram = loadShaderPair(shaderFolder, "fullscreen.vert", "tonemap.frag");
  if (program == 0 || lineProgram == 0 || toneMapProgram == 0) {
    std::printf("Could not load the shaders, stopping\n");
    shutdownUi();
    glfwTerminate();
    return 1;
  }
  bool reloadKeyWasDown = false;

  // Use the assets folder in the current folder when the source folder is not on this computer
  if (!std::filesystem::is_directory(assetFolder)) {
    assetFolder = "assets/";
  }

  // The model menu: the sphere first, then the files in assets/models
  // modelPaths has the file of each menu entry, and an empty path for the sphere.
  std::vector<std::string> modelNames = { "Sphere" };
  std::vector<std::string> modelPaths = { "" };
  for (const std::string& path : findModelFiles(assetFolder + "models")) {
    modelNames.push_back(std::filesystem::path(path).filename().string());
    modelPaths.push_back(path);
  }

  // A model file from the command line is added at the end of the menu and shown first
  if (argc > 1) {
    modelNames.push_back(std::filesystem::path(argv[1]).filename().string());
    modelPaths.push_back(argv[1]);
    settings.modelChoice = (int)modelPaths.size() - 1;
  }

  // The sphere stays loaded the whole time, a model file only while it is shown
  Model sphere = createSphereModel();
  Model fileModel;
  int shownChoice = 0;

  // Why the last model did not load, shown in the panel
  std::string modelMessage;

  // A 20 by 20 floor grid, which every model rests on
  LineMesh grid = createGrid(20, FLOOR_HEIGHT);

  // Axes for the small indicator in the corner of each view
  LineMesh axes = createAxes(AXIS_LENGTH);

  // The HDR target the scene is drawn into, made for the size the window has now
  // Four samples per pixel are asked for, which smooth the jagged edges of the model.
  int startFramebufferWidth, startFramebufferHeight;
  glfwGetFramebufferSize(window, &startFramebufferWidth, &startFramebufferHeight);
  HdrTarget hdrTarget;
  if (!createHdrTarget(hdrTarget, std::max(startFramebufferWidth, 1), std::max(startFramebufferHeight, 1))) {
    std::printf("Could not make the HDR target, stopping\n");
    shutdownUi();
    glfwTerminate();
    return 1;
  }

  // The tone mapping triangle has no vertex buffer, but drawing needs a vertex array bound
  GLuint emptyVertexArray;
  glGenVertexArrays(1, &emptyVertexArray);

  // Neutral dark gray background, a little darker than the panel
  // It is picked by eye, so it is turned into a linear value like the other colors.
  // Alpha 0 marks it as not part of the scene, so the last pass gives it only the sRGB step.
  float background = srgbToLinear(0.15f);

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
    buildUi(settings, camera, modelNames, modelMessage);

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
      reloadShaders(program, lineProgram, toneMapProgram);
    }
    reloadKeyWasDown = reloadKeyDown;

    // Load the model picked in the menu
    // If it does not load, the model that was shown stays and the menu goes back to it.
    if (settings.modelChoice != shownChoice) {
      if (settings.modelChoice == 0) {
        deleteModel(fileModel);
        shownChoice = 0;
        modelMessage = "";
      } else {
        const std::string& path = modelPaths[settings.modelChoice];
        Model newModel = loadModel(path);
        if (newModel.parts.empty()) {
          modelMessage = "Could not load " + modelNames[settings.modelChoice] + ": " + newModel.error;
          settings.modelChoice = shownChoice;
        } else {
          deleteModel(fileModel);
          fileModel = newModel;
          shownChoice = settings.modelChoice;
          modelMessage = "";
        }
      }
    }

    // The sphere takes its material from the panel
    // The panel color is picked by eye, so it is turned into a linear value like the colors of a texture.
    Material& sphereMaterial = sphere.parts[0].material;
    sphereMaterial.baseColorFactor = glm::vec4(srgbToLinear(settings.baseColor), 1.0f);
    sphereMaterial.metallicFactor = settings.metallic;
    sphereMaterial.roughnessFactor = settings.roughness;

    // Use the framebuffer size read at the top of the frame,
    // which differs from the window size on Mac Retina screens
    int width = framebufferWidth;
    int height = framebufferHeight;
    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    // The scene gets the space left of the panel
    // The panel width is in window units, so scale it to framebuffer pixels.
    float pixelsPerUnit = (windowWidth > 0) ? (float)width / windowWidth : 1.0f;
    int sceneWidth = width - (int)(getPanelWidth() * pixelsPerUnit);
    if (sceneWidth < 1) {
      sceneWidth = 1;
    }

    // A new window size needs a new HDR target
    // If it cannot be made there is nothing to draw into, so the program stops.
    if (!resizeHdrTarget(hdrTarget, width, height)) {
      std::printf("Could not make the HDR target for the new window size, stopping\n");
      break;
    }

    // The scene goes into the HDR target, which is cleared as a whole first
    glBindFramebuffer(GL_FRAMEBUFFER, hdrTarget.framebuffer);
    glClearColor(background, background, background, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // The scene is drawn in strips, one per view of the view mode
    const Model& shownModel = (shownChoice == 0) ? sphere : fileModel;
    drawScene(program, lineProgram, shownModel, grid, axes, camera, settings, sceneWidth, height);

    // Draw the HDR picture onto the window with the tone mapping pass
    // It treats each sample on its own and averages them there.
    // The HDR texture cannot be read while it is being drawn into, so the window is bound first.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    drawToneMap(toneMapProgram, hdrTarget.colorTexture, hdrTarget.samples, emptyVertexArray, settings,
                sceneWidth, width, height);

    // The panel is drawn last so it sits on top
    // It goes straight onto the window, so the tone mapping does not change its colors.
    drawUi();

    glfwSwapBuffers(window);
  }

  // Free the buffers and shaders while the OpenGL context still exists
  deleteModel(sphere);
  deleteModel(fileModel);
  deleteLines(grid);
  deleteLines(axes);
  deleteHdrTarget(hdrTarget);
  glDeleteVertexArrays(1, &emptyVertexArray);
  glDeleteProgram(program);
  glDeleteProgram(lineProgram);
  glDeleteProgram(toneMapProgram);

  shutdownUi();
  glfwTerminate();
  return 0;
}
