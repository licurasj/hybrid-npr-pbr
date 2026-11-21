#pragma once

#include <glad/gl.h>

#include "camera.h"
#include "mesh.h"
#include "model.h"
#include "settings.h"

// Length of the axis lines in the corner indicator
// The indicator square goes from minus 1 to 1, and the letters sit a little past the line tips.
const float AXIS_LENGTH = 0.7f;

// Height of the floor grid
// Every model is moved so its lowest point rests on it, like the sphere with radius 1 does.
const float FLOOR_HEIGHT = -1.0f;

// Model matrix that centers a model over the middle of the grid and scales it
// so its largest side is 2 units long, the size of the sphere, with its bottom on the floor.
glm::mat4 fitModelMatrix(const Model& model);

// Draw the scene into the area left of the panel
// The area is split into one strip per view of the current view mode,
// and every strip uses the same camera and light.
// Every mesh and material pair of the model is drawn in every strip.
// The line program draws the floor grid and the axis indicator in every strip.
void drawScene(GLuint program, GLuint lineProgram, const Model& model, const LineMesh& grid,
               const LineMesh& axes, const Camera& camera, const Settings& settings,
               int sceneWidth, int sceneHeight);
