// Stylized side of the hybrid shader
// Stand-in formulas for now, so the whole shader runs before the real ones are written.


// Uniforms used only by the stylized side
uniform vec3 uAmbientColor;


// Shadow with a hard edge
float shadowStylized(float vis) {
  return step(0.5, vis);
}

// Two band toon diffuse
// N dot L is moved from minus 1 to 1 into 0 to 1 without clamping first.
// The shadow moves this lookup position instead of being multiplied on afterward.
vec3 diffuseStylized(Surface s, Light l, float shadow) {
  float position = (dot(s.N, l.L) * 0.5 + 0.5) * shadow;
  return s.baseColor / PI * step(0.5, position) * l.radiance;
}

// Phong highlight cut into a flat spot
vec3 specularStylized(Surface s, Light l, float shadow) {
  return step(0.5, phongHighlight(s, l)) * l.radiance * shadow;
}

// Flat ambient color times the base color
vec3 ambientStylized(Surface s) {
  return uAmbientColor * s.baseColor;
}
