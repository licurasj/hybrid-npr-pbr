#version 410 core

in vec3 worldPos;
in vec3 worldNormal;
in vec2 texCoord;

uniform vec3 uLightDir;
uniform vec3 uCameraPos;
uniform vec3 uBaseColor;
uniform float uAmbient;
uniform float uDiffuse;
uniform float uSpecular;
uniform float uShininess;

out vec4 fragColor;

void main() {
  vec3 N = normalize(worldNormal);
  vec3 L = normalize(uLightDir);
  vec3 V = normalize(uCameraPos - worldPos);

  // Phong reflection model
  // R is the light direction mirrored around the normal.
  vec3 R = reflect(-L, N);
  float diffuseTerm = max(dot(N, L), 0.0);
  float specularTerm = pow(max(dot(R, V), 0.0), uShininess);

  // No highlight on the side facing away from the light
  if (diffuseTerm <= 0.0) {
    specularTerm = 0.0;
  }

  vec3 color = uAmbient * uBaseColor + uDiffuse * diffuseTerm * uBaseColor + uSpecular * specularTerm * vec3(1.0);
  fragColor = vec4(color, 1.0);
}
