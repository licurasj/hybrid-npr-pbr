#pragma once

#include "camera.h"
#include "settings.h"

struct GLFWwindow;

// Size factor of the main monitor, for example 2 on a Windows screen set to 200 percent
// It is always 1 on a Mac, which scales windows by itself.
float getMonitorScale();

// Size factor used for the panel and the other sizes on screen
float getUiScale();

// Width of the control panel on the right side of the window, in window units
float getPanelWidth();

// Set up Dear ImGui for this window, scaled for the monitor
void setupUi(GLFWwindow* window, float monitorScale);

// Build the control panel for this frame
// The camera is passed in for the reset button.
void buildUi(Settings& settings, Camera& camera);

// Draw the control panel on top of the scene
void drawUi();

// Tell the camera code whether the mouse is over the panel
bool uiWantsMouse();

// Tell the key handling whether a text field in the panel is being typed into
bool uiWantsKeyboard();

// Clean up Dear ImGui at the end
void shutdownUi();
