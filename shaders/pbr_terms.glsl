// Physical side of the hybrid shader
// Stand-in formulas for now, so the whole shader runs before the real ones are written.


// Shadow keeps the soft edge as it is
float shadowPhysical(float vis) {
  return vis;
}

// Lambert diffuse
vec3 diffusePhysical(Surface s, Light l, float shadow) {
  float NdotL = max(dot(s.N, l.L), 0.0);
  return s.baseColor / PI * NdotL * l.radiance * shadow;
}

// Phong highlight as a stand-in for the microfacet specular
vec3 specularPhysical(Surface s, Light l, float shadow) {
  return phongHighlight(s, l) * l.radiance * shadow;
}

// A small constant ambient until the environment lighting exists
vec3 ambientPhysical(Surface s) {
  return 0.03 * s.baseColor;
}
