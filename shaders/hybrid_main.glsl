// Main part of the hybrid shader
// Builds the surface and the light, then mixes the physical and stylized version of each term.


in vec3 worldPos;
in vec3 worldNormal;
in vec2 texCoord;
in vec2 texCoord1;

out vec4 fragColor;


void main() {
  // Inputs every term receives
  Surface s;
  s.position = worldPos;
  s.N = normalize(worldNormal);
  s.V = normalize(uCameraPos - worldPos);
  // Each texture reads the first or the second set of texture coordinates
  vec2 baseColorUV = (uBaseColorUVSet == 1) ? texCoord1 : texCoord;
  vec2 metallicRoughnessUV = (uMetallicRoughnessUVSet == 1) ? texCoord1 : texCoord;
  s.baseColor = texture(uBaseColorTexture, baseColorUV).rgb * uBaseColor;
  vec4 metallicRoughness = texture(uMetallicRoughnessTexture, metallicRoughnessUV);
  s.roughness = metallicRoughness.g * uRoughness;
  s.metallic = metallicRoughness.b * uMetallic;
  vec2 occlusionUV = (uOcclusionUVSet == 1) ? texCoord1 : texCoord;
  float occlusionValue = texture(uOcclusionTexture, occlusionUV).r;
  s.occlusion = mix(1.0, occlusionValue, uOcclusionStrength);

  // Glow is the same on both sides, so it is added after the blend
  vec2 emissiveUV = (uEmissiveUVSet == 1) ? texCoord1 : texCoord;
  vec3 emissive = texture(uEmissiveTexture, emissiveUV).rgb * uEmissiveFactor;

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
  vec3 reflections = mix(reflectionsPhysical(s), reflectionsStylized(s), uWeightReflections);

  // Debug views show one value on its own, in the same order as the debug view menu
  // The numbers are kept in step by hand with DEBUG_VIEW_NAMES in src/ui.cpp and the DEBUG_
  // numbers in src/renderer.cpp: a new view changes all three.
  // The term views show the blended term for the weights of this strip.
  // Alpha 1 marks the pixel as part of the scene for the last pass (the sky writes 1 too, the plain
  // background and lines write 0).
  if (uDebugView == 1) {
    fragColor = vec4(s.baseColor, 1.0);
  } else if (uDebugView == 2) {
    fragColor = vec4(emissive, 1.0);
  } else if (uDebugView == 3) {
    fragColor = vec4(vec3(s.metallic), 1.0);
  } else if (uDebugView == 4) {
    fragColor = vec4(vec3(s.roughness), 1.0);
  } else if (uDebugView == 5) {
    // Move the normal from minus 1 to 1 into the 0 to 1 color range
    fragColor = vec4(s.N * 0.5 + 0.5, 1.0);
  } else if (uDebugView == 6) {
    // The value in the texture, before the strength
    fragColor = vec4(vec3(occlusionValue), 1.0);
  } else if (uDebugView == 7) {
    fragColor = vec4(vec3(vis), 1.0);
  } else if (uDebugView == 8) {
    fragColor = vec4(diffuse, 1.0);
  } else if (uDebugView == 9) {
    fragColor = vec4(specular, 1.0);
  } else if (uDebugView == 10) {
    fragColor = vec4(ambient, 1.0);
  } else if (uDebugView == 11) {
    fragColor = vec4(reflections, 1.0);
  } else {
    // The lit picture adds the four light terms and the glow
    // The glow lights only its own pixels, not other surfaces.
    fragColor = vec4(diffuse + specular + ambient + reflections + emissive, 1.0);
  }

  // Keep the color inside what the HDR picture can hold (see HDR_MAX in environment.glsl)
  fragColor.rgb = min(fragColor.rgb, vec3(HDR_MAX));
}
