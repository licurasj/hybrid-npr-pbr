// Main part of the hybrid shader
// Builds the surface and the light, then mixes the physical and stylized version of each term.


in vec3 worldPos;
in vec3 worldNormal;

out vec4 fragColor;


void main() {
  // Inputs every term receives
  Surface s;
  s.position = worldPos;
  s.N = normalize(worldNormal);
  s.V = normalize(uCameraPos - worldPos);
  s.baseColor = uBaseColor;
  s.metallic = uMetallic;
  s.roughness = uRoughness;

  Light l;
  l.L = normalize(uLightDir);
  l.radiance = uLightRadiance;

  // Shadow visibility is 1 everywhere until the shadow map exists
  float vis = 1.0;

  // Blend each term, 0 is physical and 1 is stylized
  float shadow = mix(shadowPhysical(vis), shadowStylized(vis), uWeightShadow);
  vec3 diffuse = mix(diffusePhysical(s, l, shadow), diffuseStylized(s, l, shadow), uWeightDiffuse);
  vec3 specular = mix(specularPhysical(s, l, shadow), specularStylized(s, l, shadow), uWeightSpecular);
  vec3 ambient = mix(ambientPhysical(s), ambientStylized(s), uWeightAmbient);

  // Debug views show one value on its own, in the same order as the debug view menu
  // The term views show the blended term for the weights of this strip.
  if (uDebugView == 1) {
    fragColor = vec4(s.baseColor, 1.0);
  } else if (uDebugView == 2) {
    fragColor = vec4(vec3(s.metallic), 1.0);
  } else if (uDebugView == 3) {
    fragColor = vec4(vec3(s.roughness), 1.0);
  } else if (uDebugView == 4) {
    // Move the normal from minus 1 to 1 into the 0 to 1 color range
    fragColor = vec4(s.N * 0.5 + 0.5, 1.0);
  } else if (uDebugView == 5) {
    fragColor = vec4(vec3(vis), 1.0);
  } else if (uDebugView == 6) {
    fragColor = vec4(diffuse, 1.0);
  } else if (uDebugView == 7) {
    fragColor = vec4(specular, 1.0);
  } else if (uDebugView == 8) {
    fragColor = vec4(ambient, 1.0);
  } else {
    // The lit picture adds the three terms
    fragColor = vec4(diffuse + specular + ambient, 1.0);
  }
}
