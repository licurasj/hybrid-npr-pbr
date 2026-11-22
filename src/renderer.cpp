#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "color.h"
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


// Number of views in one row of the table above
static int countViews(const int* views) {
  int count = 0;
  while (count < 3 && views[count] != NO_VIEW) {
    count++;
  }
  return count;
}


// Uniforms that are the same in every strip
static void setSharedUniforms(GLuint program, const Model& model, const Camera& camera,
                              const Settings& settings) {
  setUniform(program, "uModel", fitModelMatrix(model));
  setUniform(program, "uView", cameraView(camera));
  setUniform(program, "uCameraPos", cameraPosition(camera));

  // The light direction points from the surface toward the light
  setUniform(program, "uLightDir", directionFromAngles(settings.lightYaw, settings.lightPitch));
  setUniform(program, "uLightRadiance", glm::vec3(settings.lightIntensity));
  // The ambient color is picked by eye, so it is turned into a linear value like the base color
  setUniform(program, "uAmbientColor", srgbToLinear(settings.ambientColor));
  setUniform(program, "uShininess", settings.shininess);

  // Each texture in the shader reads from its own texture unit
  setUniform(program, "uBaseColorTexture", 0);
  setUniform(program, "uMetallicRoughnessTexture", 1);

  // Every strip shows the same debug view
  setUniform(program, "uDebugView", settings.debugView);
}


// Model matrix that puts the model in the middle of the grid at the size of the sphere
glm::mat4 fitModelMatrix(const Model& model) {
  glm::vec3 size = model.boundsMax - model.boundsMin;
  float largest = glm::max(size.x, glm::max(size.y, size.z));

  // A model that is a single point cannot be scaled to fit, so it keeps its size
  float scale = 1.0f;
  if (largest > 0.0f) {
    scale = 2.0f / largest;
  }

  // Read from right to left: move the middle of the bottom of the box to the origin,
  // scale it, then lift or lower it onto the floor.
  // The scale is the same along every axis, so the normals keep their directions.
  glm::vec3 center = (model.boundsMin + model.boundsMax) * 0.5f;
  glm::vec3 bottomCenter(center.x, model.boundsMin.y, center.z);
  glm::mat4 matrix(1.0f);
  matrix = glm::translate(matrix, glm::vec3(0.0f, FLOOR_HEIGHT, 0.0f));
  matrix = glm::scale(matrix, glm::vec3(scale));
  matrix = glm::translate(matrix, -bottomCenter);
  return matrix;
}


// Draw every mesh of the model with its own material
static void drawModel(GLuint program, const Model& model) {
  for (const ModelPart& part : model.parts) {
    const Material& material = part.material;
    setUniform(program, "uBaseColor", glm::vec3(material.baseColorFactor));
    setUniform(program, "uMetallic", material.metallicFactor);
    setUniform(program, "uRoughness", material.roughnessFactor);

    // Each texture says which set of texture coordinates it uses
    setUniform(program, "uBaseColorUVSet", material.baseColorTexture.uvSet);
    setUniform(program, "uMetallicRoughnessUVSet", material.metallicRoughnessTexture.uvSet);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material.baseColorTexture.id);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, material.metallicRoughnessTexture.id);

    drawMesh(part.mesh);
  }

  // Leave unit 0 active, which is what the panel code expects
  glActiveTexture(GL_TEXTURE0);
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

  // No depth test here so the model and the grid never hide the axes
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
void drawScene(GLuint program, GLuint lineProgram, const Model& model, const LineMesh& grid,
               const LineMesh& axes, const Camera& camera, const Settings& settings,
               int sceneWidth, int sceneHeight) {
  const int* views = VIEW_MODES[settings.viewMode];
  int count = countViews(views);

  glUseProgram(program);
  setSharedUniforms(program, model, camera, settings);

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

    // Each strip has its own width to height ratio so the model is not stretched
    float aspect = (float)stripWidth / (float)sceneHeight;
    glm::mat4 projection = cameraProjection(aspect);
    glUseProgram(program);
    setUniform(program, "uProjection", projection);

    setViewWeights(program, views[i], settings);
    drawModel(program, model);

    // The grid is drawn after the model with the depth test on, so the model hides it.
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


// Debug views in the order of the debug view menu
const int DEBUG_LIT = 0;
const int DEBUG_BASE_COLOR = 1;
const int DEBUG_SHADOW = 5;


// How much exposure and the tone curve one kind of view gets, 1 is all and 0 none
// PBR always gets them, NPR only when its tick is on,
// and Hybrid mixes the two by the tone mapping blend slider.
static float viewToneAmount(int view, const Settings& settings) {
  float toonAmount = settings.toonUsesToneMapping ? 1.0f : 0.0f;
  if (view == VIEW_REALISTIC) {
    return 1.0f;
  } else if (view == VIEW_TOON) {
    return toonAmount;
  }
  // The shader mixes the two results by this amount, so mixing the amounts
  // gives the same picture as mixing the PBR and NPR pictures.
  float realisticAmount = 1.0f;
  float w = settings.toneMappingWeight;
  return (1.0f - w) * realisticAmount + w * toonAmount;
}


// Draw the full-screen triangle with the tone mapping shader
void drawToneMap(GLuint toneMapProgram, GLuint hdrImage, int sampleCount, GLuint emptyVertexArray,
                 const Settings& settings, int sceneWidth, int width, int height) {
  // The viewport stays the whole window so each pixel reads its own place in the HDR picture.
  // The scissor test then lets each strip be drawn with its own amount.
  glViewport(0, 0, width, height);
  glEnable(GL_SCISSOR_TEST);

  // The triangle covers everything, so there is nothing for the depth test to do
  glDisable(GL_DEPTH_TEST);

  glUseProgram(toneMapProgram);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, hdrImage);
  glBindVertexArray(emptyVertexArray);
  setUniform(toneMapProgram, "uHdrImage", 0);
  setUniform(toneMapProgram, "uSampleCount", sampleCount);
  setUniform(toneMapProgram, "uExposure", settings.exposure);
  setUniform(toneMapProgram, "uToneCurve", settings.toneCurve);

  // Base color is a color, so it gets the sRGB step.
  // Metallic, roughness, normal and shadow are data, so they are shown as stored.
  // The lit picture and the light terms are colors that can go above 1.
  int sceneToSrgb = 1;
  if (settings.debugView > DEBUG_BASE_COLOR && settings.debugView <= DEBUG_SHADOW) {
    sceneToSrgb = 0;
  }
  setUniform(toneMapProgram, "uSceneToSrgb", sceneToSrgb);

  // A debug view gets exposure and the curve only when its tick is on
  bool useToneMapping = (settings.debugView == DEBUG_LIT) || settings.debugViewToneMapping;

  // One pass per strip, with the same strip edges as the scene
  const int* views = VIEW_MODES[settings.viewMode];
  int count = countViews(views);
  for (int i = 0; i < count; i++) {
    int left = i * sceneWidth / count;
    int right = (i + 1) * sceneWidth / count;
    if (right - left < 1) {
      continue;
    }

    float toneAmount = 0.0f;
    if (useToneMapping) {
      toneAmount = viewToneAmount(views[i], settings);
    }
    setUniform(toneMapProgram, "uToneAmount", toneAmount);

    glScissor(left, 0, right - left, height);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  // The space under the panel holds only background, which the amount does not change,
  // but it still needs a pass so nothing old is left there.
  if (sceneWidth < width) {
    setUniform(toneMapProgram, "uToneAmount", 0.0f);
    glScissor(sceneWidth, 0, width - sceneWidth, height);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }

  glDisable(GL_SCISSOR_TEST);
  glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
  glBindVertexArray(0);
  glEnable(GL_DEPTH_TEST);
}
