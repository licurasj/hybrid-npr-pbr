#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;
layout(location = 4) in vec2 uv1;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;
out vec2 texCoord1;

void main() {
  vec4 world = uModel * vec4(position, 1.0);
  worldPos = world.xyz;

  // The normal matrix keeps normals correct when the model is scaled unevenly
  worldNormal = mat3(transpose(inverse(uModel))) * normal;
  texCoord = uv;
  texCoord1 = uv1;

  gl_Position = uProjection * uView * world;
}
