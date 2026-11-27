#include "ui.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cmath>
#include <string>


// Panel width and text size before scaling for the monitor
const float PANEL_WIDTH = 380.0f;
const float UI_SIZE = 1.3f;

// Monitor scale used for the panel
float uiScale = 1.0f;

// OpenGL version text shown in the info block
std::string glVersionText;


// Size factor of the main monitor
float getMonitorScale() {
  return ImGui_ImplGlfw_GetContentScaleForMonitor(glfwGetPrimaryMonitor());
}


// Size factor used for the panel and the other sizes on screen
float getUiScale() {
  return uiScale;
}


// Width of the control panel in window units
float getPanelWidth() {
  return PANEL_WIDTH * uiScale;
}


// Neutral dark gray colors in the style of desktop graphics tools
// Every panel color is set here so the look can be changed in one place.
static void applyPanelTheme() {
  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;

  ImVec4 panel = ImVec4(0.196f, 0.196f, 0.196f, 1.0f);
  ImVec4 dark = ImVec4(0.125f, 0.125f, 0.125f, 1.0f);
  ImVec4 line = ImVec4(0.11f, 0.11f, 0.11f, 1.0f);
  ImVec4 accent = ImVec4(0.29f, 0.53f, 0.80f, 1.0f);
  ImVec4 accentBright = ImVec4(0.38f, 0.62f, 0.90f, 1.0f);

  colors[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.86f, 1.0f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
  colors[ImGuiCol_WindowBg] = panel;
  colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
  colors[ImGuiCol_Border] = line;
  colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

  // Input fields are darker than the panel, like recessed boxes
  colors[ImGuiCol_FrameBg] = dark;
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
  colors[ImGuiCol_FrameBgActive] = ImVec4(0.19f, 0.19f, 0.19f, 1.0f);

  // Plain gray title bar instead of the bright blue one
  colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
  colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
  colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);

  // Section headers are slightly lighter bars
  colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
  colors[ImGuiCol_HeaderActive] = ImVec4(0.33f, 0.33f, 0.33f, 1.0f);

  colors[ImGuiCol_Button] = ImVec4(0.27f, 0.27f, 0.27f, 1.0f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.33f, 0.33f, 0.33f, 1.0f);
  colors[ImGuiCol_ButtonActive] = accent;

  // Slider handles are light gray, a bit lighter while dragged
  colors[ImGuiCol_SliderGrab] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
  colors[ImGuiCol_SliderGrabActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
  colors[ImGuiCol_CheckMark] = accentBright;
  colors[ImGuiCol_TextSelectedBg] = ImVec4(0.29f, 0.53f, 0.80f, 0.4f);

  colors[ImGuiCol_Separator] = line;
  colors[ImGuiCol_ScrollbarBg] = panel;
  colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
  colors[ImGuiCol_ResizeGrip] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

  // Square panel with small rounding on the controls
  style.WindowRounding = 0.0f;
  style.WindowBorderSize = 1.0f;
  style.FrameRounding = 3.0f;
  style.GrabRounding = 3.0f;
}


// Names in the debug view menu, in the order the shader checks them
// Kept in step by hand with the uDebugView checks in shaders/hybrid_main.glsl and the DEBUG_
// numbers in renderer.cpp (and the 8 below): a new view changes all three.
static const char* const DEBUG_VIEW_NAMES[] = {
  "Default", "Base color", "Emissive", "Metallic", "Roughness", "Normal", "Occlusion",
  "Shadow visibility", "Diffuse only", "Specular only", "Ambient only", "Reflections only",
};


// Whether a debug view gets exposure and the tone curve when it is picked
// Base color, emissive, metallic, roughness, normal, occlusion and shadow are shown as they are stored,
// while the diffuse, specular, ambient and reflection terms are light that can go above 1.
static bool debugViewWantsToneMapping(int debugView) {
  return debugView == 0 || debugView >= 8;
}


// Width of the number box next to each slider, wide enough for -180.0
static float numberBoxWidth() {
  return ImGui::CalcTextSize("-180.0").x + ImGui::GetStyle().FramePadding.x * 2.0f;
}


// Small square button with a circular arrow drawn in it, returns true when clicked
// The default font has no arrow symbol, so the arrow is drawn with lines: three quarters of a
// circle and a small triangle at its end.
static bool resetArrowButton(const char* id, const char* tip) {
  const float PI = 3.14159265f;
  float size = ImGui::GetFrameHeight();
  bool clicked = ImGui::InvisibleButton(id, ImVec2(size, size));
  ImGui::SetItemTooltip("%s", tip);

  ImU32 color = ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_Text : ImGuiCol_TextDisabled);
  ImVec2 corner = ImGui::GetItemRectMin();
  ImVec2 center = ImVec2(corner.x + size * 0.5f, corner.y + size * 0.5f);
  float radius = size * 0.28f;
  float head = size * 0.14f;
  ImDrawList* draw = ImGui::GetWindowDrawList();

  // From the top, clockwise round to the left side
  draw->PathArcTo(center, radius, -0.5f * PI, PI, 16);
  draw->PathStroke(color, size * 0.08f);

  // Arrow head at the left end of the arc, pointing up
  ImVec2 end = ImVec2(center.x - radius, center.y);
  draw->AddTriangleFilled(ImVec2(end.x - head, end.y), ImVec2(end.x + head, end.y),
                          ImVec2(end.x, end.y - head * 1.3f), color);
  return clicked;
}


// Reset arrow after a control, shown only while the value differs from its default
// The first version returns true when the arrow was clicked. The others put the default back
// themselves, one for each kind of value the panel has.
static bool resetArrow(const char* label, bool changed) {
  if (!changed) {
    return false;
  }
  ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
  ImGui::PushID(label);
  bool clicked = resetArrowButton("##reset", "Back to the default");
  ImGui::PopID();
  return clicked;
}

// Whether two numbers differ by at least the given step
// The panel only shows a few digits, so a difference smaller than that counts as no change.
static bool differs(float a, float b, float tolerance) {
  return std::fabs(a - b) >= tolerance;
}

// Half of the last digit a number box shows, for a format like "%.2f" (0.005)
static float halfLastDigit(const char* format) {
  int digits = format[2] - '0';
  return 0.5f * std::pow(10.0f, (float)-digits);
}

// Number formats of the number boxes, also used to tell whether a value has changed
static const char* const ANGLE_FORMAT = "%.1f";
static const char* const VALUE_FORMAT = "%.2f";

// Whether a slider value differs from its default in the digits its number box shows
static bool sliderChanged(float value, float defaultValue, const char* format) {
  return differs(value, defaultValue, halfLastDigit(format));
}

static void resetArrow(const char* label, float* value, float defaultValue, const char* format) {
  if (resetArrow(label, sliderChanged(*value, defaultValue, format))) {
    *value = defaultValue;
  }
}

static void resetArrow(const char* label, int* value, int defaultValue) {
  if (resetArrow(label, *value != defaultValue)) {
    *value = defaultValue;
  }
}

static void resetArrow(const char* label, bool* value, bool defaultValue) {
  if (resetArrow(label, *value != defaultValue)) {
    *value = defaultValue;
  }
}

// Width of a color picker, one reset arrow narrower than a slider row
// The color labels are long, so this leaves room for the arrow inside the panel.
static float colorPickerWidth() {
  return ImGui::CalcItemWidth() - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemInnerSpacing.x;
}


// A color channel as the color box shows it, a whole number from 0 to 255
static int colorByte(float channel) {
  return (int)(channel * 255.0f + 0.5f);
}

// Colors count as changed only when the numbers in the color box differ
// A plain tolerance of half a step would not work here: the default green 0.3 is 76.5 on the
// 0 to 255 scale, exactly half a step from 77, which is what the box stores after an edit.
static bool colorChanged(glm::vec3 value, glm::vec3 defaultValue) {
  return colorByte(value.r) != colorByte(defaultValue.r) ||
         colorByte(value.g) != colorByte(defaultValue.g) ||
         colorByte(value.b) != colorByte(defaultValue.b);
}

static void resetArrow(const char* label, glm::vec3* value, glm::vec3 defaultValue) {
  if (resetArrow(label, colorChanged(*value, defaultValue))) {
    *value = defaultValue;
  }
}


// Color box that always shows red, green and blue from 0 to 255
// The right-click menu that switches to other number styles is turned off, so the reset arrow,
// which compares these 0 to 255 numbers, always matches what the box shows.
static void colorBox(const char* label, glm::vec3* value) {
  ImGui::SetNextItemWidth(colorPickerWidth());
  ImGui::ColorEdit3(label, &value->x, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_Uint8 |
                                      ImGuiColorEditFlags_NoOptions);
}


// Section header with a reset arrow at its right end, shown only while the section has changed
// Returns whether the section is open, and returns true in resetClicked when the arrow was clicked.
// The header and the arrow sit in two columns of a small table. ImGui only lets the mouse reach
// the part of an item inside its own column, so the header and the arrow never share a spot,
// and a click on the arrow can not fold the section.
static bool sectionHeader(const char* name, bool changed, bool* resetClicked) {
  *resetClicked = false;
  bool open = false;
  ImGui::PushID(name);
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
  if (ImGui::BeginTable("##header", 2)) {
    float arrowColumn = ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;
    ImGui::TableSetupColumn("##name", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##arrow", ImGuiTableColumnFlags_WidthFixed, arrowColumn);
    ImGui::TableNextColumn();
    open = ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::TableNextColumn();
    if (changed) {
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().ItemInnerSpacing.x);
      *resetClicked = resetArrowButton("##resetSection", "Reset this section");
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleVar();
  ImGui::PopID();
  return open;
}


// Slider that was double-clicked and is waiting for the mouse button to be let go
// The slider keeps following the mouse while the button is down, so the reset happens on release.
static float* resetOnRelease = nullptr;


// A slider bar, a small number box, the label and a reset arrow on one row
// Both change the same value, so dragging updates the box and typing moves the bar.
// A typed value is kept inside the slider range, and a value that is not a real number is ignored.
// A double-click on the bar or a click on the arrow puts the default value back.
static void sliderWithBox(const char* label, float* value, float defaultValue, float min, float max,
                          const char* format, const char* tip = nullptr) {
  float gap = ImGui::GetStyle().ItemInnerSpacing.x;
  ImGui::PushID(label);

  // The bar has no number inside, the box shows it instead
  // Typing into the bar itself is turned off because the box is there for typing,
  // which also leaves the double-click free for the reset.
  ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - numberBoxWidth() - gap);
  ImGui::SliderFloat("##bar", value, min, max, "", ImGuiSliderFlags_NoInput);
  if (tip != nullptr) {
    ImGui::SetItemTooltip("%s\nDouble-click resets, type in the box", tip);
  } else {
    ImGui::SetItemTooltip("Double-click resets, type in the box");
  }
  // Only when the double-click really grabbed this slider, see the end of buildUi
  if (ImGui::IsItemActive() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
    resetOnRelease = value;
  }
  if (ImGui::IsItemDeactivated() && resetOnRelease == value) {
    *value = defaultValue;
    resetOnRelease = nullptr;
  }

  ImGui::SameLine(0.0f, gap);
  ImGui::SetNextItemWidth(numberBoxWidth());
  float oldValue = *value;
  if (ImGui::InputFloat("##box", value, 0.0f, 0.0f, format)) {
    // Text such as nan or inf is not a number we can use, so keep the old value
    if (!std::isfinite(*value)) {
      *value = oldValue;
    }
    if (*value < min) {
      *value = min;
    }
    if (*value > max) {
      *value = max;
    }
  }

  ImGui::SameLine(0.0f, gap);
  ImGui::TextUnformatted(label);
  ImGui::PopID();
  resetArrow(label, value, defaultValue, format);
}


// Whether anything in a section differs from its default, as the panel shows it
// These use the same tests as the reset arrows after each control.
static bool viewChanged(const Settings& s, const Settings& d) {
  return s.viewMode != d.viewMode || s.debugView != d.debugView || s.showGrid != d.showGrid ||
         s.showAxes != d.showAxes;
}

static bool modelChanged(const Settings& s, const Settings& d) {
  return s.modelChoice != d.modelChoice;
}

// The camera has no number boxes, so any move of at least a hundredth counts
static bool cameraChanged(const Camera& c) {
  Camera d;
  float step = 0.01f;
  return differs(c.yaw, d.yaw, step) || differs(c.pitch, d.pitch, step) ||
         differs(c.distance, d.distance, step) || differs(c.target.x, d.target.x, step) ||
         differs(c.target.y, d.target.y, step) || differs(c.target.z, d.target.z, step);
}

static bool lightChanged(const Settings& s, const Settings& d) {
  return sliderChanged(s.lightYaw, d.lightYaw, ANGLE_FORMAT) ||
         sliderChanged(s.lightPitch, d.lightPitch, ANGLE_FORMAT) ||
         sliderChanged(s.lightIntensity, d.lightIntensity, VALUE_FORMAT) ||
         colorChanged(s.ambientColor, d.ambientColor);
}

// The default of the debug view tick depends on the debug view, the same as when a view is
// picked
static bool toneMappingChanged(const Settings& s, const Settings& d) {
  return sliderChanged(s.exposure, d.exposure, VALUE_FORMAT) || s.toneCurve != d.toneCurve ||
         s.toonUsesToneMapping != d.toonUsesToneMapping ||
         s.debugViewToneMapping != debugViewWantsToneMapping(s.debugView);
}

static bool environmentChanged(const Settings& s, const Settings& d) {
  return s.environmentChoice != d.environmentChoice || s.backgroundChoice != d.backgroundChoice ||
         sliderChanged(s.environmentRotation, d.environmentRotation, ANGLE_FORMAT) ||
         sliderChanged(s.environmentIntensity, d.environmentIntensity, VALUE_FORMAT) ||
         s.environmentMethod != d.environmentMethod;
}

static bool materialChanged(const Settings& s, const Settings& d) {
  return colorChanged(s.baseColor, d.baseColor) ||
         sliderChanged(s.metallic, d.metallic, VALUE_FORMAT) ||
         sliderChanged(s.roughness, d.roughness, VALUE_FORMAT);
}

static bool nprChanged(const Settings& s, const Settings& d) {
  return s.rampChoice != d.rampChoice ||
         sliderChanged(s.highlightThreshold, d.highlightThreshold, VALUE_FORMAT) ||
         sliderChanged(s.highlightStrength, d.highlightStrength, VALUE_FORMAT) ||
         s.highlightFollowsRamp != d.highlightFollowsRamp;
}

static bool blendChanged(const Settings& s, const Settings& d) {
  for (int i = 0; i < WEIGHT_COUNT; i++) {
    if (sliderChanged(s.weights[i], d.weights[i], VALUE_FORMAT)) {
      return true;
    }
  }
  return sliderChanged(s.toneMappingWeight, d.toneMappingWeight, VALUE_FORMAT);
}


// Put one section back to its defaults
static void resetView(Settings& s, const Settings& d) {
  s.viewMode = d.viewMode;
  s.debugView = d.debugView;
  s.debugViewToneMapping = debugViewWantsToneMapping(d.debugView);
  s.showGrid = d.showGrid;
  s.showAxes = d.showAxes;
}

static void resetLight(Settings& s, const Settings& d) {
  s.lightYaw = d.lightYaw;
  s.lightPitch = d.lightPitch;
  s.lightIntensity = d.lightIntensity;
  s.ambientColor = d.ambientColor;
}

static void resetToneMapping(Settings& s, const Settings& d) {
  s.exposure = d.exposure;
  s.toneCurve = d.toneCurve;
  s.toonUsesToneMapping = d.toonUsesToneMapping;
  s.debugViewToneMapping = debugViewWantsToneMapping(s.debugView);
}

static void resetEnvironment(Settings& s, const Settings& d) {
  s.environmentChoice = d.environmentChoice;
  s.backgroundChoice = d.backgroundChoice;
  s.environmentRotation = d.environmentRotation;
  s.environmentIntensity = d.environmentIntensity;
  s.environmentMethod = d.environmentMethod;
}

static void resetMaterial(Settings& s, const Settings& d) {
  s.baseColor = d.baseColor;
  s.metallic = d.metallic;
  s.roughness = d.roughness;
}

static void resetNpr(Settings& s, const Settings& d) {
  s.rampChoice = d.rampChoice;
  s.highlightThreshold = d.highlightThreshold;
  s.highlightStrength = d.highlightStrength;
  s.highlightFollowsRamp = d.highlightFollowsRamp;
}

static void resetBlend(Settings& s, const Settings& d) {
  for (int i = 0; i < WEIGHT_COUNT; i++) {
    s.weights[i] = d.weights[i];
  }
  s.toneMappingWeight = d.toneMappingWeight;
}


// Set up Dear ImGui for this window
void setupUi(GLFWwindow* window, float monitorScale) {
  uiScale = monitorScale;

  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  applyPanelTheme();

  // Make the text and the sliders a bit larger than the default
  // and scale everything for high resolution screens.
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(UI_SIZE * uiScale);
  style.FontScaleMain = UI_SIZE;
  style.FontScaleDpi = uiScale;

  glVersionText = (const char*)glGetString(GL_VERSION);

  // The second argument lets ImGui install its own mouse and keyboard callbacks
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 410 core");
}


// Build the control panel for this frame
void buildUi(Settings& settings, Camera& camera, const std::vector<std::string>& modelNames,
             const std::string& modelMessage) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // The panel fills the right side of the window from top to bottom
  // and the scene is drawn in the space to its left.
  ImVec2 display = ImGui::GetIO().DisplaySize;
  ImGui::SetNextWindowPos(ImVec2(display.x - getPanelWidth(), 0.0f));
  ImGui::SetNextWindowSize(ImVec2(getPanelWidth(), display.y));
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
  ImGui::Begin("Controls", nullptr, flags);

  // Default values for the reset arrows and buttons, all taken from Settings
  Settings defaults;
  bool reset = false;

  // Frame rate, and a button that resets the whole panel while anything has changed
  // The text is lined up with a button even when there is none, so the row keeps its height.
  bool anythingChanged = viewChanged(settings, defaults) || modelChanged(settings, defaults) ||
                         cameraChanged(camera) || lightChanged(settings, defaults) ||
                         environmentChanged(settings, defaults) ||
                         toneMappingChanged(settings, defaults) ||
                         materialChanged(settings, defaults) || nprChanged(settings, defaults) ||
                         blendChanged(settings, defaults);
  float fps = ImGui::GetIO().Framerate;
  ImGui::AlignTextToFramePadding();
  ImGui::Text("%.0f FPS   %.2f ms per frame", fps, 1000.0f / fps);
  if (anythingChanged) {
    const char* resetAllText = "Reset all";
    float buttonWidth = ImGui::CalcTextSize(resetAllText).x +
                        ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonWidth);
    if (ImGui::Button(resetAllText)) {
      resetView(settings, defaults);
      settings.modelChoice = defaults.modelChoice;
      camera = Camera();
      resetLight(settings, defaults);
      resetEnvironment(settings, defaults);
      resetToneMapping(settings, defaults);
      resetMaterial(settings, defaults);
      resetNpr(settings, defaults);
      resetBlend(settings, defaults);
    }
    ImGui::SetItemTooltip("Put every setting and the camera back to the default");
  }
  ImGui::TextDisabled("OpenGL %s", glVersionText.c_str());
  ImGui::Spacing();

  // Sliders stretch across the panel and leave room for their labels
  ImGui::PushItemWidth(-110.0f * uiScale);

  // PBR (the physical look) is always on the left and NPR (the stylized look) on the right
  bool open = sectionHeader("View", viewChanged(settings, defaults), &reset);
  if (reset) {
    resetView(settings, defaults);
  }
  if (open) {
    ImGui::Combo("View mode", &settings.viewMode,
                 "PBR | Hybrid | NPR\0PBR\0Hybrid\0NPR\0"
                 "PBR | Hybrid\0Hybrid | NPR\0PBR | NPR\0");
    resetArrow("View mode", &settings.viewMode, defaults.viewMode);

    // Shows one input or one lighting term on its own in every view
    // A new debug view also sets the exposure and tone curve tick to what suits it.
    if (ImGui::Combo("Debug view", &settings.debugView, DEBUG_VIEW_NAMES,
                     IM_ARRAYSIZE(DEBUG_VIEW_NAMES))) {
      settings.debugViewToneMapping = debugViewWantsToneMapping(settings.debugView);
    }
    if (resetArrow("Debug view", settings.debugView != defaults.debugView)) {
      settings.debugView = defaults.debugView;
      settings.debugViewToneMapping = debugViewWantsToneMapping(settings.debugView);
    }

    // Floor grid under the model and the small axes in the corner of each view
    ImGui::Checkbox("Show grid", &settings.showGrid);
    resetArrow("Show grid", &settings.showGrid, defaults.showGrid);
    ImGui::Checkbox("Show axes", &settings.showAxes);
    resetArrow("Show axes", &settings.showAxes, defaults.showAxes);
  }

  // The sphere, the sphere grid or one of the model files
  // Picking an entry only changes the choice, and the main loop then loads the file.
  open = sectionHeader("Model", modelChanged(settings, defaults), &reset);
  if (reset) {
    settings.modelChoice = defaults.modelChoice;
  }
  if (open) {
    const char* shownName = modelNames[settings.modelChoice].c_str();
    if (ImGui::BeginCombo("Model##choice", shownName)) {
      for (int i = 0; i < (int)modelNames.size(); i++) {
        bool selected = (i == settings.modelChoice);
        // "##" and the number make each entry unique for ImGui, even when two files have the same name
        std::string label = modelNames[i] + "##" + std::to_string(i);
        if (ImGui::Selectable(label.c_str(), selected)) {
          settings.modelChoice = i;
        }
      }
      ImGui::EndCombo();
    }
    resetArrow("Model", &settings.modelChoice, defaults.modelChoice);

    // A model that could not be loaded leaves a message in red
    if (!modelMessage.empty()) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.4f, 1.0f));
      ImGui::TextWrapped("%s", modelMessage.c_str());
      ImGui::PopStyleColor();
    }
  }

  // The camera is moved with the mouse, so it has no sliders, only the reset
  open = sectionHeader("Camera", cameraChanged(camera), &reset);
  if (reset) {
    camera = Camera();
  }
  if (open) {
    if (ImGui::Button("Reset camera")) {
      camera = Camera();
    }
  }

  open = sectionHeader("Light", lightChanged(settings, defaults), &reset);
  if (reset) {
    resetLight(settings, defaults);
  }
  if (open) {
    sliderWithBox("Yaw", &settings.lightYaw, defaults.lightYaw, -180.0f, 180.0f, ANGLE_FORMAT);
    sliderWithBox("Pitch", &settings.lightPitch, defaults.lightPitch, -89.0f, 89.0f, ANGLE_FORMAT);
    sliderWithBox("Intensity", &settings.lightIntensity, defaults.lightIntensity, 0.0f, 10.0f,
                  VALUE_FORMAT);
    colorBox("Ambient color", &settings.ambientColor);
    resetArrow("Ambient color", &settings.ambientColor, defaults.ambientColor);
  }

  // The environment image lights the PBR side from all around and is shown behind the model
  // Picking another image only changes the choice, and the main loop then builds its cubemap.
  open = sectionHeader("Environment", environmentChanged(settings, defaults), &reset);
  if (reset) {
    resetEnvironment(settings, defaults);
  }
  if (open) {
    ImGui::Combo("Environment", &settings.environmentChoice,
                 "Partly cloudy\0Overcast\0Small studio\0Photo studio\0Sunset\0");
    resetArrow("Environment", &settings.environmentChoice, defaults.environmentChoice);

    // The order matches BACKGROUND_SKY, BACKGROUND_BLURRED_SKY and BACKGROUND_PLAIN
    ImGui::Combo("Background", &settings.backgroundChoice, "Sky\0Blurred sky\0Plain\0");
    ImGui::SetItemTooltip("What is drawn behind the model in every view.\n"
                          "The environment lights the model either way. Debug views use Plain");
    resetArrow("Background", &settings.backgroundChoice, defaults.backgroundChoice);

    // The Light section has an Intensity slider too, so this section gets its own ID scope
    // to keep the two apart
    ImGui::PushID("Environment section");
    sliderWithBox("Rotation", &settings.environmentRotation, defaults.environmentRotation,
                  -180.0f, 180.0f, ANGLE_FORMAT, "Turns the sky, fill light and reflections together");
    sliderWithBox("Intensity", &settings.environmentIntensity, defaults.environmentIntensity,
                  0.0f, 4.0f, VALUE_FORMAT, "Brightness of the sky, fill light and reflections");

    // The order matches METHOD_SIMPLE and METHOD_PREFILTERED
    ImGui::Combo("Method", &settings.environmentMethod, "Simple\0Prefiltered\0");
    ImGui::SetItemTooltip("How the fill light and the reflections are worked out.\n"
                          "Simple: the blurrier copies of the sky, picked by roughness.\n"
                          "Prefiltered: maps blurred like a rough surface, made at startup");
    resetArrow("Method", &settings.environmentMethod, defaults.environmentMethod);
    ImGui::PopID();
  }

  // The lighting can be brighter than the screen can show.
  // Exposure scales the picture and the tone curve squeezes it back into the screen range.
  // Off just cuts off everything above 1.
  open = sectionHeader("Tone mapping", toneMappingChanged(settings, defaults), &reset);
  if (reset) {
    resetToneMapping(settings, defaults);
  }
  if (open) {
    sliderWithBox("Exposure", &settings.exposure, defaults.exposure, 0.0f, 4.0f, VALUE_FORMAT);
    ImGui::Combo("Tone curve", &settings.toneCurve, "Off\0Reinhard\0ACES fit\0");
    resetArrow("Tone curve", &settings.toneCurve, defaults.toneCurve);

    // Off shows the NPR colors as they are computed, only cut off at 1
    ImGui::Checkbox("NPR uses tone mapping", &settings.toonUsesToneMapping);
    ImGui::SetItemTooltip("Give the NPR view exposure and the tone curve too");
    resetArrow("NPR uses tone mapping", &settings.toonUsesToneMapping,
               defaults.toonUsesToneMapping);

    // The normal picture always gets exposure and the curve, so the tick is only for debug views
    ImGui::BeginDisabled(settings.debugView == 0);
    ImGui::Checkbox("Tone mapping on debug view", &settings.debugViewToneMapping);
    ImGui::SetItemTooltip("Data views (metallic, roughness, normal, shadow) get exposure and the curve\n"
                          "but no sRGB step, so their numbers stay as stored");
    resetArrow("Tone mapping on debug view", &settings.debugViewToneMapping,
               debugViewWantsToneMapping(settings.debugView));
    ImGui::EndDisabled();
  }

  open = sectionHeader("Material", materialChanged(settings, defaults), &reset);
  if (reset) {
    resetMaterial(settings, defaults);
  }
  if (open) {
    // A model file brings its own base color, metallic and roughness
    // The sphere grid uses the base color and sets metallic and roughness for each sphere itself.
    if (settings.modelChoice == MODEL_SPHERE_GRID) {
      ImGui::TextDisabled("The grid uses only Base color");
    } else if (settings.modelChoice != MODEL_SPHERE) {
      ImGui::TextDisabled("A model file uses its own values");
    }
    colorBox("Base color", &settings.baseColor);
    resetArrow("Base color", &settings.baseColor, defaults.baseColor);
    sliderWithBox("Metallic", &settings.metallic, defaults.metallic, 0.0f, 1.0f, VALUE_FORMAT);
    sliderWithBox("Roughness", &settings.roughness, defaults.roughness, 0.0f, 1.0f, VALUE_FORMAT);
  }

  // Settings of the stylized side, they work on every model
  open = sectionHeader("NPR shading", nprChanged(settings, defaults), &reset);
  if (reset) {
    resetNpr(settings, defaults);
  }
  if (open) {
    // The ramp image gives the color bands of the toon diffuse
    ImGui::Combo("Ramp", &settings.rampChoice, "Two bands\0Three bands\0");
    resetArrow("Ramp", &settings.rampChoice, defaults.rampChoice);

    // The toon highlight is a flat spot whose size follows the roughness
    ImGui::TextDisabled("Highlight");
    sliderWithBox("Threshold", &settings.highlightThreshold, defaults.highlightThreshold,
                  0.0f, 1.0f, VALUE_FORMAT,
                  "Higher makes the spot smaller: 0 covers the lit side,\n"
                  "0.5 puts the edge where the GGX peak has halved, 1 hides it");
    sliderWithBox("Strength", &settings.highlightStrength, defaults.highlightStrength,
                  0.0f, 4.0f, VALUE_FORMAT, "1 is as bright as a white surface facing the light");
    ImGui::Checkbox("Follows ramp bands", &settings.highlightFollowsRamp);
    ImGui::SetItemTooltip("Dim the highlight to the ramp band it falls in, like painted toon art.\n"
                          "Off, it is one shape that ends only where the light stops reaching");
    resetArrow("Follows ramp bands", &settings.highlightFollowsRamp, defaults.highlightFollowsRamp);
  }

  // 0 is the physical version of a term and 1 the stylized one
  open = sectionHeader("Blend", blendChanged(settings, defaults), &reset);
  if (reset) {
    resetBlend(settings, defaults);
  }
  if (open) {
    // PBR over the left end of the slider bars and NPR over the right end
    // The bars end where the number boxes start.
    float barLeft = ImGui::GetCursorPosX();
    float gap = ImGui::GetStyle().ItemInnerSpacing.x;
    float barRight = barLeft + ImGui::CalcItemWidth() - numberBoxWidth() - gap;
    ImGui::TextDisabled("PBR");
    ImGui::SameLine(barRight - ImGui::CalcTextSize("NPR").x);
    ImGui::TextDisabled("NPR");

    // The same tooltip on every blend slider
    const char* blendTip = "0 = PBR, 1 = NPR";
    sliderWithBox("Shadow", &settings.weights[0], defaults.weights[0], 0.0f, 1.0f, VALUE_FORMAT,
                  blendTip);
    sliderWithBox("Diffuse", &settings.weights[1], defaults.weights[1], 0.0f, 1.0f, VALUE_FORMAT,
                  blendTip);
    sliderWithBox("Specular", &settings.weights[2], defaults.weights[2], 0.0f, 1.0f, VALUE_FORMAT,
                  blendTip);
    sliderWithBox("Ambient", &settings.weights[3], defaults.weights[3], 0.0f, 1.0f, VALUE_FORMAT,
                  blendTip);
    sliderWithBox("Reflections", &settings.weights[4], defaults.weights[4], 0.0f, 1.0f, VALUE_FORMAT,
                  "0 = PBR, 1 = NPR\nNPR has no reflections yet, so toward NPR they fade out");

    // Exposure and the tone curve of the Hybrid view, between how PBR and NPR get them
    // When NPR uses tone mapping is ticked, both ends are the same and this slider changes nothing.
    sliderWithBox("Tone mapping", &settings.toneMappingWeight, defaults.toneMappingWeight,
                  0.0f, 1.0f, VALUE_FORMAT, blendTip);
  }

  // A double-click reset waits for the mouse button to be let go. Once the button is up and every
  // slider has had its turn this frame, nothing is waiting any more.
  if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
    resetOnRelease = nullptr;
  }

  ImGui::PopItemWidth();
  ImGui::End();
}


// Draw the control panel on top of the scene
void drawUi() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


// Tell the camera code whether the mouse is over the panel
bool uiWantsMouse() {
  return ImGui::GetIO().WantCaptureMouse;
}


// Tell the key handling whether a text field in the panel is being typed into
bool uiWantsKeyboard() {
  return ImGui::GetIO().WantTextInput;
}


// Clean up Dear ImGui at the end
void shutdownUi() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}
