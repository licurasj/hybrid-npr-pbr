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
static const char* const DEBUG_VIEW_NAMES[] = {
  "Default", "Base color", "Metallic", "Roughness", "Normal",
  "Shadow visibility", "Diffuse only", "Specular only", "Ambient only",
};


// Whether a debug view gets exposure and the tone curve when it is picked
// Base color, metallic, roughness, normal and shadow are shown as they are stored,
// while the diffuse, specular and ambient terms are light that can go above 1.
static bool debugViewWantsToneMapping(int debugView) {
  return debugView == 0 || debugView >= 6;
}


// Width of the number box next to each slider, wide enough for -180.0
static float numberBoxWidth() {
  return ImGui::CalcTextSize("-180.0").x + ImGui::GetStyle().FramePadding.x * 2.0f;
}


// A slider bar, a small number box and the label on one row
// Both change the same value, so dragging updates the box and typing moves the bar.
// A typed value is kept inside the slider range, and a value that is not a real number is ignored.
static void sliderWithBox(const char* label, float* value, float min, float max,
                          const char* format, const char* tip = nullptr) {
  float gap = ImGui::GetStyle().ItemInnerSpacing.x;
  ImGui::PushID(label);

  // The bar has no number inside, the box shows it instead
  // Typing into the bar itself is turned off because the box is there for typing.
  ImGui::SetNextItemWidth(ImGui::CalcItemWidth() - numberBoxWidth() - gap);
  ImGui::SliderFloat("##bar", value, min, max, "", ImGuiSliderFlags_NoInput);
  if (tip != nullptr) {
    ImGui::SetItemTooltip("%s", tip);
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

  // Frame rate and OpenGL version
  float fps = ImGui::GetIO().Framerate;
  ImGui::Text("%.0f FPS   %.2f ms per frame", fps, 1000.0f / fps);
  ImGui::TextDisabled("OpenGL %s", glVersionText.c_str());
  ImGui::Spacing();

  // Sliders stretch across the panel and leave room for their labels
  ImGui::PushItemWidth(-110.0f * uiScale);

  // Default values to go back to with the reset buttons
  Settings defaults;

  // PBR (the physical look) is always on the left and NPR (the stylized look) on the right
  if (ImGui::CollapsingHeader("View", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::Combo("View mode", &settings.viewMode,
                 "PBR | Hybrid | NPR\0PBR\0Hybrid\0NPR\0"
                 "PBR | Hybrid\0Hybrid | NPR\0PBR | NPR\0");

    // Shows one input or one lighting term on its own in every view
    // A new debug view also sets the exposure and tone curve tick to what suits it.
    if (ImGui::Combo("Debug view", &settings.debugView, DEBUG_VIEW_NAMES,
                     IM_ARRAYSIZE(DEBUG_VIEW_NAMES))) {
      settings.debugViewToneMapping = debugViewWantsToneMapping(settings.debugView);
    }

    // Floor grid under the model and the small axes in the corner of each view
    ImGui::Checkbox("Show grid", &settings.showGrid);
    ImGui::Checkbox("Show axes", &settings.showAxes);
  }

  // The sphere or one of the model files
  // Picking an entry only changes the choice, and the main loop then loads the file.
  if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
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

    // A model that could not be loaded leaves a message in red
    if (!modelMessage.empty()) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.4f, 1.0f));
      ImGui::TextWrapped("%s", modelMessage.c_str());
      ImGui::PopStyleColor();
    }
  }

  if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Button("Reset camera")) {
      camera = Camera();
    }
  }

  if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
    sliderWithBox("Yaw", &settings.lightYaw, -180.0f, 180.0f, "%.1f");
    sliderWithBox("Pitch", &settings.lightPitch, -89.0f, 89.0f, "%.1f");
    sliderWithBox("Intensity", &settings.lightIntensity, 0.0f, 10.0f, "%.2f");
    ImGui::ColorEdit3("Ambient color", &settings.ambientColor.x);
    if (ImGui::Button("Reset light")) {
      settings.lightYaw = defaults.lightYaw;
      settings.lightPitch = defaults.lightPitch;
      settings.lightIntensity = defaults.lightIntensity;
      settings.ambientColor = defaults.ambientColor;
    }
  }

  // The lighting can be brighter than the screen can show.
  // Exposure scales the picture and the tone curve squeezes it back into the screen range.
  // Off just cuts off everything above 1.
  if (ImGui::CollapsingHeader("Tone mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
    sliderWithBox("Exposure", &settings.exposure, 0.0f, 4.0f, "%.2f");
    ImGui::Combo("Tone curve", &settings.toneCurve, "Off\0Reinhard\0ACES fit\0");

    // Off shows the NPR colors as they are computed, only cut off at 1
    ImGui::Checkbox("NPR uses tone mapping", &settings.toonUsesToneMapping);
    ImGui::SetItemTooltip("Give the NPR view exposure and the tone curve too");

    // The normal picture always gets exposure and the curve, so the tick is only for debug views
    ImGui::BeginDisabled(settings.debugView == 0);
    ImGui::Checkbox("Exposure and tone curve on debug view", &settings.debugViewToneMapping);
    ImGui::SetItemTooltip("Data views (metallic, roughness, normal, shadow) get exposure and the curve\n"
                          "but no sRGB step, so their numbers stay as stored");
    ImGui::EndDisabled();
  }

  if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
    // A model file brings its own base color, metallic and roughness
    if (settings.modelChoice != 0) {
      ImGui::TextDisabled("These values are used by the sphere only");
    }
    ImGui::ColorEdit3("Base color", &settings.baseColor.x);
    sliderWithBox("Metallic", &settings.metallic, 0.0f, 1.0f, "%.2f");
    sliderWithBox("Roughness", &settings.roughness, 0.0f, 1.0f, "%.2f");
    sliderWithBox("Shininess", &settings.shininess, 1.0f, 256.0f, "%.1f");
  }

  // 0 is the physical version of a term and 1 the stylized one
  if (ImGui::CollapsingHeader("Blend", ImGuiTreeNodeFlags_DefaultOpen)) {
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
    sliderWithBox("Shadow", &settings.weights[0], 0.0f, 1.0f, "%.2f", blendTip);
    sliderWithBox("Diffuse", &settings.weights[1], 0.0f, 1.0f, "%.2f", blendTip);
    sliderWithBox("Specular", &settings.weights[2], 0.0f, 1.0f, "%.2f", blendTip);
    sliderWithBox("Ambient", &settings.weights[3], 0.0f, 1.0f, "%.2f", blendTip);

    // Exposure and the tone curve of the Hybrid view, between how PBR and NPR get them
    // When NPR uses tone mapping is ticked, both ends are the same and this slider changes nothing.
    sliderWithBox("Tone mapping", &settings.toneMappingWeight, 0.0f, 1.0f, "%.2f", blendTip);
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
