#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "shader.h"
#include "ui.h"


// The three kinds of view
// PBR is the physical side and uses only the physical terms,
// NPR is the stylized side and uses only the stylized terms
// and Hybrid uses the blend slider values.
const int VIEW_REALISTIC = 0;
const int VIEW_HYBRID = 1;
const int VIEW_TOON = 2;

// Marks an unused place in the table below
const int NO_VIEW = -1;

// Views drawn by each view mode, from left to right
// The rows are in the same order as the view mode menu in the panel.
const int VIEW_MODES[7][3] = {
  { VIEW_REALISTIC, VIEW_HYBRID, VIEW_TOON },  // PBR | Hybrid | NPR
  { VIEW_REALISTIC, NO_VIEW, NO_VIEW },        // PBR
  { VIEW_HYBRID, NO_VIEW, NO_VIEW },           // Hybrid
  { VIEW_TOON, NO_VIEW, NO_VIEW },             // NPR
  { VIEW_REALISTIC, VIEW_HYBRID, NO_VIEW },    // PBR | Hybrid
  { VIEW_HYBRID, VIEW_TOON, NO_VIEW },         // Hybrid | NPR
  { VIEW_REALISTIC, VIEW_TOON, NO_VIEW },      // PBR | NPR
};


// Uniforms that are the same in every strip
static void setSharedUniforms(GLuint program, const Camera& camera, const Settings& settings) {
  setUniform(program, "uModel", glm::mat4(1.0f));
  setUniform(program, "uView", cameraView(camera));
  setUniform(program, "uCameraPos", cameraPosition(camera));

  // The light direction points from the surface toward the light
  setUniform(program, "uLightDir", directionFromAngles(settings.lightYaw, settings.lightPitch));
  setUniform(program, "uLightRadiance", glm::vec3(settings.lightIntensity));
  setUniform(program, "uAmbientColor", settings.ambientColor);
  setUniform(program, "uBaseColor", settings.baseColor);
  setUniform(program, "uMetallic", settings.metallic);
  setUniform(program, "uRoughness", settings.roughness);
  setUniform(program, "uShininess", settings.shininess);

  // Every strip shows the same debug view
  setUniform(program, "uDebugView", settings.debugView);
}


// Blend weights for one kind of view, 0 is physical and 1 is stylized
static void setViewWeights(GLuint program, int view, const Settings& settings) {
  float weights[4];
  for (int i = 0; i < 4; i++) {
    if (view == VIEW_REALISTIC) {
      weights[i] = 0.0f;
    } else if (view == VIEW_TOON) {
      weights[i] = 1.0f;
    } else {
      weights[i] = settings.weights[i];
    }
  }
  setUniform(program, "uWeightShadow", weights[0]);
  setUniform(program, "uWeightDiffuse", weights[1]);
  setUniform(program, "uWeightSpecular", weights[2]);
  setUniform(program, "uWeightAmbient", weights[3]);
}


// Side of the axis indicator square before scaling for the monitor
const float AXIS_BOX_SIZE = 90.0f;

// Where the letters go along each axis, a little past the line tips
const float AXIS_LETTER_DISTANCE = AXIS_LENGTH * 1.2f;


// Draw the small axis indicator in the bottom left corner of a strip
// It uses only the turn of the camera and no perspective, so the axes keep their length
// and turn with the camera but do not move with it.
static void drawAxisIndicator(GLuint lineProgram, const LineMesh& axes, const Camera& camera,
                              int stripLeft, int stripWidth, int sceneHeight) {
  // The panel sizes are in window units, so the square is scaled to framebuffer pixels
  // which are twice as many on a Mac Retina screen.
  float framebufferScale = ImGui::GetIO().DisplayFramebufferScale.x;
  int size = (int)(AXIS_BOX_SIZE * getUiScale() * framebufferScale);

  // In a strip smaller than the square the letters would land on the next strip or the panel,
  // so the indicator is left out there.
  if (stripWidth < size || sceneHeight < size) {
    return;
  }

  glViewport(stripLeft, 0, size, size);

  // The view matrix without its move part keeps only the turn of the camera
  glm::mat4 rotation = glm::mat4(glm::mat3(cameraView(camera)));
  glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
  setUniform(lineProgram, "uMVP", projection * rotation);

  // No depth test here so the sphere and the grid never hide the axes
  // Afterwards the depth test is put back the way it was.
  GLboolean depthWasOn = glIsEnabled(GL_DEPTH_TEST);
  glDisable(GL_DEPTH_TEST);
  drawLines(axes);
  if (depthWasOn) {
    glEnable(GL_DEPTH_TEST);
  }

  // The letters at the tips are drawn as panel text, which is simpler than drawing them with lines
  // Panel text is placed in window units from the top left, while the viewport counts
  // framebuffer pixels from the bottom left, so each tip is converted.
  const char* letters[3] = { "X", "Y", "Z" };
  ImU32 colors[3] = { IM_COL32(240, 100, 100, 255), IM_COL32(120, 220, 100, 255),
                      IM_COL32(110, 150, 255, 255) };
  for (int i = 0; i < 3; i++) {
    glm::vec4 axis(0.0f, 0.0f, 0.0f, 1.0f);
    axis[i] = AXIS_LETTER_DISTANCE;
    glm::vec4 tip = projection * rotation * axis;

    float pixelX = stripLeft + (tip.x * 0.5f + 0.5f) * size;
    float pixelY = (tip.y * 0.5f + 0.5f) * size;
    float windowX = pixelX / framebufferScale;
    float windowY = (sceneHeight - pixelY) / framebufferScale;

    // Center the letter on the point
    ImVec2 textSize = ImGui::CalcTextSize(letters[i]);
    ImVec2 position(windowX - textSize.x * 0.5f, windowY - textSize.y * 0.5f);
    ImGui::GetForegroundDrawList()->AddText(position, colors[i], letters[i]);
  }
}


// Draw the scene once per strip of the current view mode
void drawScene(GLuint program, GLuint lineProgram, const Mesh& mesh, const LineMesh& grid,
               const LineMesh& axes, const Camera& camera, const Settings& settings,
               int sceneWidth, int sceneHeight) {
  const int* views = VIEW_MODES[settings.viewMode];

  // Count the views of this mode
  int count = 0;
  while (count < 3 && views[count] != NO_VIEW) {
    count++;
  }

  glUseProgram(program);
  setSharedUniforms(program, camera, settings);

  // The scissor test keeps each strip from drawing into its neighbours
  glEnable(GL_SCISSOR_TEST);
  for (int i = 0; i < count; i++) {
    // Strip edges are worked out from the full width so no pixel column is left over
    int left = i * sceneWidth / count;
    int right = (i + 1) * sceneWidth / count;
    int stripWidth = right - left;

    // A very narrow window can leave a strip with no pixels, and it has no shape to draw
    if (stripWidth < 1 || sceneHeight < 1) {
      continue;
    }

    glViewport(left, 0, stripWidth, sceneHeight);
    glScissor(left, 0, stripWidth, sceneHeight);

    // Each strip has its own width to height ratio so the sphere stays round
    float aspect = (float)stripWidth / (float)sceneHeight;
    glm::mat4 projection = cameraProjection(aspect);
    glUseProgram(program);
    setUniform(program, "uProjection", projection);

    setViewWeights(program, views[i], settings);
    drawMesh(mesh);

    // The grid is drawn after the sphere with the depth test on, so the sphere hides it.
    // It has its own shader, so the blend weights and the debug view do not change it.
    glUseProgram(lineProgram);
    if (settings.showGrid) {
      setUniform(lineProgram, "uMVP", projection * cameraView(camera));
      drawLines(grid);
    }

    // The axis indicator changes the viewport, and the next strip sets it again
    if (settings.showAxes) {
      drawAxisIndicator(lineProgram, axes, camera, left, stripWidth, sceneHeight);
    }
  }

  // The panel code keeps whatever scissor state it finds,
  // so turn it off or the next clear only reaches the last strip.
  glDisable(GL_SCISSOR_TEST);
}
