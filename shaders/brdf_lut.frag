// BRDF lookup table: the part of the GGX reflection that does not depend on the environment
// The program puts the version line and sampling.glsl in front of this file.
// Across the table N.V goes from 0 to 1, upward roughness goes from 0 to 1. Each pixel stores
// two numbers A and B: a surface with base reflectance F0 reflects F0 A + B of a uniform
// environment (Karis 2013). The visibility term is the height-correlated Smith form, the same as
// our direct specular and the Khronos glTF Sample Renderer.


out vec4 fragColor;

uniform float uTableSize;  // width and height of the table in pixels
uniform int uSampleCount;


// Height-correlated Smith visibility, G / (4 N.L N.V) (Heitz 2014)
// The same formula as in pbr_terms.glsl, without the N.L factor that one has on top.
float visibilitySmith(float NdotL, float NdotV, float a) {
  float a2 = a * a;
  float viewPart = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
  float lightPart = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
  return 0.5 / (viewPart + lightPart);
}


void main() {
  // Pixel centers, so N.V never is exactly 0
  float NdotV = gl_FragCoord.x / uTableSize;
  float roughness = gl_FragCoord.y / uTableSize;
  float a = roughness * roughness;

  // Any normal works, so it is the z axis, and V lies in the x-z plane
  vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
  float A = 0.0;
  float B = 0.0;
  for (int i = 0; i < uSampleCount; i++) {
    vec3 H = sampleGGX(hammersley(i, uSampleCount), a);
    vec3 L = 2.0 * dot(V, H) * H - V;
    float NdotL = L.z;
    float NdotH = H.z;
    float VdotH = dot(V, H);
    if (NdotL > 0.0) {
      // The reflection D Vis N.L divided by the pdf of L, D N.H / (4 V.H): D cancels
      float term = visibilitySmith(NdotL, NdotV, a) * 4.0 * VdotH * NdotL / NdotH;
      // Schlick's Fresnel F0 + (1 - F0) Fc splits into an F0 part and a part without F0
      float Fc = pow(1.0 - VdotH, 5.0);
      A += (1.0 - Fc) * term;
      B += Fc * term;
    }
  }
  fragColor = vec4(A / float(uSampleCount), B / float(uSampleCount), 0.0, 1.0);
}
