// Irradiance map: the light a matte (Lambert) surface facing each direction receives
// The program puts the version line and sampling.glsl in front of this file.
// For every pixel the environment is averaged over the half of all directions in front of the
// normal, weighted by cos(theta), the Lambert law. The samples are picked with that same weight
// (cosine importance sampling), so their plain average is the result. As in the Khronos glTF
// Sample Renderer, the value stored is irradiance / pi, so base color times it is the diffuse.


out vec4 fragColor;

uniform samplerCube uSource;  // the environment cubemap with its mip levels
uniform float uSourceSize;    // width of one face of the source, level 0
uniform int uSampleCount;


void main() {
  vec3 N = cubeFaceDirection();
  vec3 sum = vec3(0.0);
  for (int i = 0; i < uSampleCount; i++) {
    vec2 u = hammersley(i, uSampleCount);

    // Direction with probability cos(theta) / pi around the z axis
    float phi = 2.0 * PI * u.x;
    float cosTheta = sqrt(1.0 - u.y);
    float sinTheta = sqrt(u.y);
    vec3 L = aroundNormal(vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta), N);

    // cos(theta) can be exactly 0 at u.y = 1, then the pdf is 0 and the level is the blurriest
    float pdf = max(cosTheta / PI, 1e-6);
    sum += textureLod(uSource, L, sourceLevel(pdf, uSampleCount, uSourceSize)).rgb;
  }
  fragColor = vec4(sum / float(uSampleCount), 1.0);
}
