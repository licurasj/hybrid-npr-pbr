#pragma once

#include <glad/gl.h>

#include "camera.h"
#include "mesh.h"
#include "settings.h"

// Length of the axis lines in the corner indicator
// The indicator square goes from minus 1 to 1, and the letters sit a little past the line tips.
const float AXIS_LENGTH = 0.7f;

// Draw the scene into the area left of the panel
// The area is split into one strip per view of the current view mode,
// and every strip uses the same camera and light.
// The line program draws the floor grid and the axis indicator in every strip.
void drawScene(GLuint program, GLuint lineProgram, const Mesh& mesh, const LineMesh& grid,
               const LineMesh& axes, const Camera& camera, const Settings& settings,
               int sceneWidth, int sceneHeight);
