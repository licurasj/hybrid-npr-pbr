#version 410 core

// The sky: a cube around the camera, drawn behind everything
// The camera's position is left out of the view matrix, so the cube moves with the camera
// and only its turn shows. Each corner's position is also its direction from the camera.

layout(location = 0) in vec3 position;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 skyDirection;

void main() {
  skyDirection = position;

  // mat3 keeps only the turn of the view matrix, the move part is dropped
  mat4 turnOnly = mat4(mat3(uView));
  vec4 clip = uProjection * turnOnly * vec4(position, 1.0);

  // z = w gives a depth of exactly 1 after the divide, the farthest depth there is,
  // so the model and the grid are always in front of the sky
  gl_Position = clip.xyww;
}
