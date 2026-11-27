// Physical side of the hybrid shader
// Cook-Torrance specular and Lambert diffuse for the glTF metallic and roughness material,
// computed the way the Khronos glTF Sample Viewer does it, plus fill light and reflections
// from the environment image.
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

// Fresnel for light arriving from the whole environment instead of one direction
// F = F0 + (max(1 - roughness, F0) - F0) (1 - N.V)^5
// With no single light there is no half vector, so N.V takes the place of V.H. A rough surface
// reflects light from many directions, so its edges brighten less: the top value 1 of the plain
// Schlick formula is lowered to 1 - roughness (but never below F0).
// The same formula as the Khronos glTF Sample Viewer uses for its environment light.
vec3 fresnelSchlickRoughness(vec3 F0, float NdotV, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - NdotV, 5.0);
}

// Maps made from the environment at startup, for the prefiltered method
uniform samplerCube uIrradianceMap;   // texture unit 6, fill light for each normal direction
uniform samplerCube uPrefilteredMap;  // texture unit 7, one mip level per roughness step
uniform sampler2D uBrdfLookupTable;   // texture unit 8, A and B for each N.V and roughness
uniform int uPrefilteredMipCount;

// Which method the fill light and the reflections use: 0 simple, 1 prefiltered
// The same numbers are in src/settings.h (METHOD_SIMPLE, METHOD_PREFILTERED); change both together.
uniform int uEnvironmentMethod;
const int METHOD_SIMPLE = 0;

// Mip level the simple fill light is read from, counted down from the smallest level
// The second smallest level has 2 x 2 pixels per cube face, so it holds the average light of
// large parts of the environment but still changes with the direction (the smallest level,
// 1 x 1, would give a whole face one color).
const float FILL_LEVELS_FROM_TOP = 2.0;


// How much of the environment a surface with base reflectance F0 reflects, prefiltered method
// The lookup table gives A and B with F0 A + B = the reflection of a uniform environment (Karis
// 2013). As in the Khronos glTF Sample Renderer, F0 there is the roughness-aware Schlick term
// above, and light that bounces between the tiny facets more than once is added back
// (Fdez-Aguera 2019): one bounce reflects E = A + B, the rest 1 - E would be lost, and an
// average Fresnel F_avg = F0 + (1 - F0) / 21 of it comes out after more bounces.
vec3 environmentFresnel(vec3 F0, float NdotV, float roughness) {
  vec2 ab = texture(uBrdfLookupTable, vec2(NdotV, roughness)).rg;
  vec3 FssEss = fresnelSchlickRoughness(F0, NdotV, roughness) * ab.x + ab.y;
  float Ems = 1.0 - (ab.x + ab.y);
  vec3 Favg = F0 + (1.0 - F0) / 21.0;
  vec3 FmsEms = Ems * FssEss * Favg / (1.0 - Favg * Ems);
  return FssEss + FmsEms;
}

// Ambient: soft fill light from the whole environment
// Like the diffuse, only the non-metal part gets it, and the light the non-metal reflects at its
// surface (its environment Fresnel term with F0 = 0.04) does not enter it. The direct light's
// shadow does not dim it, the light comes from everywhere.
// Simple: a very blurred sample of the environment along the normal stands in for the light
// arriving from all directions above the surface.
// Prefiltered: the irradiance map holds exactly that light, cosine weighted, divided by pi.
// Both are multiplied by the occlusion, as glTF says for light that does not come from a lamp.
vec3 ambientPhysical(Surface s) {
  float NdotV = clamp(dot(s.N, s.V), 0.0, 1.0);
  float roughness = clamp(s.roughness, 0.0, 1.0);
  if (uEnvironmentMethod == METHOD_SIMPLE) {
    vec3 F = fresnelSchlickRoughness(vec3(0.04), NdotV, roughness);
    float fillLevel = float(uEnvironmentMipCount) - FILL_LEVELS_FROM_TOP;
    return (1.0 - F) * (1.0 - s.metallic) * s.baseColor * sampleEnvironment(s.N, fillLevel) * s.occlusion;
  }
  vec3 F = environmentFresnel(vec3(0.04), NdotV, roughness);
  vec3 fill = readEnvironmentMap(uIrradianceMap, s.N, 0.0);
  return (1.0 - F) * (1.0 - s.metallic) * s.baseColor * fill * s.occlusion;
}

// Reflections: the environment seen in the surface like in a mirror
// The camera direction is mirrored on the normal, and the environment is read in that direction.
// Rougher surfaces read a blurrier mip level: roughness 0 reads the sharp image, roughness 1 the
// blurriest level.
// Simple: the ordinary mip levels of the environment (each the average of 2 x 2 pixels of the
// one before), times the roughness-aware Schlick term with the mixed F0.
// Prefiltered: the prefiltered map, times the environment Fresnel above. As in the glTF blend of
// a non-metal and a metal, the non-metal part (F0 = 0.04) and the metal part (F0 = base color)
// get their own term, mixed by metallic.
// Both are multiplied by the occlusion like the fill light (the Khronos glTF Sample Renderer does
// the same for its environment reflections).
vec3 reflectionsPhysical(Surface s) {
  float NdotV = clamp(dot(s.N, s.V), 0.0, 1.0);
  float roughness = clamp(s.roughness, 0.0, 1.0);
  vec3 R = reflect(-s.V, s.N);
  if (uEnvironmentMethod == METHOD_SIMPLE) {
    float level = roughness * float(uEnvironmentMipCount - 1);
    vec3 F = fresnelSchlickRoughness(baseReflectance(s), NdotV, roughness);
    return F * sampleEnvironment(R, level) * s.occlusion;
  }
  vec3 dielectric = environmentFresnel(vec3(0.04), NdotV, roughness);
  vec3 metal = environmentFresnel(s.baseColor, NdotV, roughness);
  vec3 F = mix(dielectric, metal, s.metallic);
  float level = roughness * float(uPrefilteredMipCount - 1);
  return F * readEnvironmentMap(uPrefilteredMap, R, level) * s.occlusion;
}
