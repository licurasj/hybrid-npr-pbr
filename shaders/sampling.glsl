// Shared part of the startup passes that build the environment lighting maps
// The program puts the version line in front and one of irradiance.frag, prefilter.frag or
// brdf_lut.frag after this file. Each pass draws one full-screen triangle into one face of a
// cubemap (or into the lookup table) and works out every pixel by averaging many samples.


const float PI = 3.14159265;

// Which cube face is being drawn, 0 to 5 in the OpenGL order +X, -X, +Y, -Y, +Z, -Z,
// and its width in pixels
uniform int uFace;
uniform float uFaceSize;


// Direction through the middle of the pixel being drawn on the current cube face
// The same face table as the program uses when it builds the cubemap from the panorama
// (OpenGL 4.1 specification, section 3.8.10). gl_FragCoord is the pixel center, row 0 at the
// bottom of the drawing, which is also the first row of the face.
vec3 cubeFaceDirection() {
  float a = 2.0 * gl_FragCoord.x / uFaceSize - 1.0;
  float b = 2.0 * gl_FragCoord.y / uFaceSize - 1.0;
  vec3 d;
  if (uFace == 0) {
    d = vec3(1.0, -b, -a);
  } else if (uFace == 1) {
    d = vec3(-1.0, -b, a);
  } else if (uFace == 2) {
    d = vec3(a, 1.0, b);
  } else if (uFace == 3) {
    d = vec3(a, -1.0, -b);
  } else if (uFace == 4) {
    d = vec3(a, -b, 1.0);
  } else {
    d = vec3(-a, -b, -1.0);
  }
  return normalize(d);
}

// Sample point i of n, spread evenly over the unit square (the Hammersley set)
// The first number steps evenly from 0 to 1. The second mirrors the bits of i around the binary
// point (the van der Corput sequence), so the points fill the square without clumping, which
// gives a smoother average than random numbers for the same count.
vec2 hammersley(int i, int n) {
  uint bits = bitfieldReverse(uint(i));
  return vec2(float(i) / float(n), float(bits) * 2.3283064365386963e-10);
}

// Turn a direction given around the z axis into the same direction around N
vec3 aroundNormal(vec3 v, vec3 N) {
  // Any vector not parallel to N gives a side direction
  vec3 up = (abs(N.z) < 0.999) ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  vec3 tangent = normalize(cross(up, N));
  vec3 bitangent = cross(N, tangent);
  return tangent * v.x + bitangent * v.y + N * v.z;
}

// A half vector around the z axis, picked so that more of them point where the GGX
// distribution is large (importance sampling); a = roughness squared
// cos(theta) = sqrt((1 - u) / (1 + (a^2 - 1) u)) turns an even u into the GGX spread of angles.
vec3 sampleGGX(vec2 u, float a) {
  float phi = 2.0 * PI * u.x;
  float cosTheta = sqrt((1.0 - u.y) / (1.0 + (a * a - 1.0) * u.y));
  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
  return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

// GGX normal distribution D, the same formula as in pbr_terms.glsl
float distributionGGX(float NdotH, float a) {
  float a2 = a * a;
  float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
  return a2 / (PI * d * d);
}

// Mip level of the source cubemap to read for one sample, the formula of the Khronos glTF Sample
// Renderer (its version of Colbert and Krivanek, GPU Gems 3, chapter 20)
// A sample that is picked rarely (small pdf) stands for a large patch of directions, so it reads
// a blurrier level whose pixels cover about that patch. This keeps a small bright spot such as
// the sun from turning into speckles. The patch is 1 / (sampleCount pdf) and one source pixel
// covers about 4 pi / (6 w^2). Khronos leaves the 4 pi out, lod = 0.5 log2(6 w^2 / (sampleCount pdf)),
// which reads about 1.8 levels blurrier than the plain ratio (GPU Gems adds 1 level for the same reason).
float sourceLevel(float pdf, int sampleCount, float sourceSize) {
  float lod = 0.5 * log2(6.0 * sourceSize * sourceSize / (float(sampleCount) * pdf));
  // Never sharper than level 0 or blurrier than the 1 x 1 level
  return clamp(lod, 0.0, log2(sourceSize));
}
