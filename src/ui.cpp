#include "ui.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

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

  // The blue accent only marks the parts you drag
  colors[ImGuiCol_SliderGrab] = accent;
  colors[ImGuiCol_SliderGrabActive] = accentBright;
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
void buildUi(Settings& settings, Camera& camera) {
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

  if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::Button("Reset camera")) {
      camera = Camera();
    }
  }

  if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::SliderFloat("Yaw", &settings.lightYaw, -180.0f, 180.0f);
    ImGui::SliderFloat("Pitch", &settings.lightPitch, -89.0f, 89.0f);
    if (ImGui::Button("Reset light")) {
      settings.lightYaw = defaults.lightYaw;
      settings.lightPitch = defaults.lightPitch;
    }
  }

  if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
    ImGui::ColorEdit3("Base color", &settings.baseColor.x);
    ImGui::SliderFloat("Ambient", &settings.ambient, 0.0f, 1.0f);
    ImGui::SliderFloat("Diffuse", &settings.diffuse, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular", &settings.specular, 0.0f, 1.0f);
    ImGui::SliderFloat("Shininess", &settings.shininess, 1.0f, 256.0f);
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
