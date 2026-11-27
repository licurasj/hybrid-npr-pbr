// Prefiltered map: the environment blurred the way a GGX surface of one roughness blurs it
// The program puts the version line and sampling.glsl in front of this file.
// Each mip level of the map is drawn with its own roughness. Like the Khronos glTF Sample
// Renderer and Karis (2013) the camera is taken to look straight along the normal (N = V = R),
// which lets one map serve every viewing angle at the cost of losing the stretched reflections
// seen at grazing angles.


out vec4 fragColor;

uniform samplerCube uSource;  // the environment cubemap with its mip levels
uniform float uSourceSize;    // width of one face of the source, level 0
uniform int uSampleCount;
uniform float uRoughness;     // roughness of this level, 0 to 1


void main() {
  vec3 N = cubeFaceDirection();

  // Roughness 0 is a perfect mirror, which reflects the environment as it is
  if (uRoughness == 0.0) {
    fragColor = vec4(textureLod(uSource, N, 0.0).rgb, 1.0);
    return;
  }

  vec3 V = N;
  float a = uRoughness * uRoughness;
  vec3 sum = vec3(0.0);
  float weight = 0.0;
  for (int i = 0; i < uSampleCount; i++) {
    // A half vector picked by the GGX distribution, and the light direction it mirrors V into
    vec3 H = aroundNormal(sampleGGX(hammersley(i, uSampleCount), a), N);
    vec3 L = 2.0 * dot(V, H) * H - V;
    float NdotL = dot(N, L);
    if (NdotL > 0.0) {
      // With N = V, N.H equals V.H, so the pdf of L, D N.H / (4 V.H), is D / 4
      float NdotH = clamp(dot(N, H), 0.0, 1.0);
      float pdf = max(distributionGGX(NdotH, a) / 4.0, 1e-6);
      float level = sourceLevel(pdf, uSampleCount, uSourceSize);

      // Weighting by N.L as Karis (2013) does gives a better match at higher roughness
      sum += textureLod(uSource, L, level).rgb * NdotL;
      weight += NdotL;
    }
  }
  fragColor = vec4(sum / weight, 1.0);
}
