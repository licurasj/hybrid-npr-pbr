// Physical side of the hybrid shader
// Cook-Torrance specular and Lambert diffuse for the glTF metallic and roughness material,
// computed the way the Khronos glTF Sample Viewer does it.
// D and F are the ones in the glTF 2.0 specification (its Appendix B). The visibility term is
// the more accurate height-correlated form the specification points to (Heitz 2014).
// The diffuse is the paint (non-metal) part of the glTF blend of a non-metal and a metal.


// Smallest roughness the physical terms use
// At roughness 0 the highlight of this light would be one point of endless brightness and the
// formulas would divide by zero. 0.05 is also the lowest roughness in the LearnOpenGL sphere grid.
// Only these terms use the clamped value, so the roughness debug view still shows the real one.
const float MIN_ROUGHNESS = 0.05;

// Largest specular value written to the HDR picture
// The HDR picture stores half floats, which stop at 65504. A very sharp highlight seen at a
// grazing angle can go past that, so it is capped a little below. It is white on screen either way.
const float MAX_SPECULAR = 60000.0;


// Roughness as the formulas use it: alpha = roughness squared
float alphaFromRoughness(float roughness) {
  float r = clamp(roughness, MIN_ROUGHNESS, 1.0);
  return r * r;
}

// How much light the surface reflects when it is seen straight on (F0), for the specular
// Non-metals reflect about 4 percent in every color, metals reflect their base color,
// and a metallic value in between mixes the two.
vec3 baseReflectance(Surface s) {
  return mix(vec3(0.04), s.baseColor, s.metallic);
}

// Half vector, the direction halfway between the light and the camera
// When the two point almost exactly opposite ways their sum has almost no length, there is no
// clear halfway direction and normalize would divide by zero or by rounding noise. Then a zero
// vector is returned, so V.H is 0, which is what V.H tends to as the two turn opposite. This only
// happens when the light or the camera is behind the surface or right at its edge, where the
// specular is zero or tiny.
vec3 halfVector(Surface s, Light l) {
  vec3 sum = l.L + s.V;
  if (dot(sum, sum) < 1e-12) {
    return vec3(0.0);
  }
  return normalize(sum);
}

// Schlick's Fresnel: more light is reflected at grazing angles
// F = F0 + (1 - F0) (1 - V.H)^5
vec3 fresnelSchlick(vec3 F0, float VdotH) {
  return F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);
}

// GGX normal distribution D: how many tiny facets point along the half vector
// D = a^2 / (pi ((N.H)^2 (a^2 - 1) + 1)^2)
float distributionGGX(float NdotH, float a) {
  float a2 = a * a;
  float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
  return a2 / (PI * d * d);
}

// Height-correlated Smith visibility (Heitz 2014), as in the Khronos glTF Sample Viewer,
// already multiplied by N.L
// The visibility is the geometry term G (facets hiding each other) divided by 4 N.L N.V,
// so the specular below has no divisor of its own. N.L is multiplied on here, before the
// division, because for very small N.L and N.V the division alone could overflow.
float visibilitySmithTimesNdotL(float NdotL, float NdotV, float a) {
  float a2 = a * a;
  float viewPart = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
  float lightPart = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
  float denominator = viewPart + lightPart;
  if (denominator <= 0.0) {
    return 0.0;
  }
  return 0.5 * NdotL / denominator;
}


// Shadow keeps the soft edge as it is
float shadowPhysical(float vis) {
  return vis;
}

// Lambert diffuse
// Only the non-metal part of the material has a diffuse color, so it gets (1 - metallic).
// The light that the non-metal reflects at its surface does not enter it, so it also gets
// (1 - F) with the non-metal's own F0 of 0.04, not the mixed F0 the specular uses.
vec3 diffusePhysical(Surface s, Light l, float shadow) {
  float NdotL = dot(s.N, l.L);
  if (NdotL <= 0.0) {
    return vec3(0.0);
  }
  vec3 H = halfVector(s, l);
  float VdotH = clamp(dot(s.V, H), 0.0, 1.0);
  vec3 F = fresnelSchlick(vec3(0.04), VdotH);
  vec3 weight = (1.0 - F) * (1.0 - s.metallic);
  return weight * s.baseColor / PI * NdotL * l.radiance * shadow;
}

// Cook-Torrance microfacet specular: D times visibility times F
vec3 specularPhysical(Surface s, Light l, float shadow) {
  float NdotL = dot(s.N, l.L);
  float NdotV = dot(s.N, s.V);
  // No highlight when the light or the camera is behind the surface
  if (NdotL <= 0.0 || NdotV <= 0.0) {
    return vec3(0.0);
  }
  vec3 H = halfVector(s, l);
  float NdotH = clamp(dot(s.N, H), 0.0, 1.0);
  float VdotH = clamp(dot(s.V, H), 0.0, 1.0);
  float a = alphaFromRoughness(s.roughness);

  float D = distributionGGX(NdotH, a);
  float visTimesNdotL = visibilitySmithTimesNdotL(NdotL, NdotV, a);
  vec3 F = fresnelSchlick(baseReflectance(s), VdotH);
  vec3 specular = D * visTimesNdotL * F * l.radiance * shadow;
  return min(specular, vec3(MAX_SPECULAR));
}

// A small constant ambient until the environment lighting exists
vec3 ambientPhysical(Surface s) {
  return 0.03 * s.baseColor;
}
