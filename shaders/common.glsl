// Shared part of the hybrid shader
// The program puts the version line in front and the side files after this one.


// GLSL has no built in pi
const float PI = 3.14159265;


// What every term function knows about the point being shaded
struct Surface {
  vec3 position;    // world space
  vec3 N;           // unit surface normal
  vec3 V;           // unit direction from the surface to the camera
  vec3 baseColor;   // linear
  float metallic;   // 0 non-metal, 1 metal
  float roughness;  // 0 mirror, 1 fully rough
};

// What every term function knows about the light
struct Light {
  vec3 L;           // unit direction from the surface to the light
  vec3 radiance;    // light color times intensity, linear
};


// Uniforms used by both sides
uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightRadiance;

// Material of the part being drawn
// Each texture value is multiplied by its factor. A material without a texture gets a 1 x 1 white
// texture, so then the factor is used as it is.
// glTF keeps roughness in the green channel and metallic in the blue channel of one texture.
uniform sampler2D uBaseColorTexture;          // texture unit 0
uniform sampler2D uMetallicRoughnessTexture;  // texture unit 1
uniform int uBaseColorUVSet;                  // which texture coordinates each texture uses, 0 or 1
uniform int uMetallicRoughnessUVSet;
uniform vec3 uBaseColor;
uniform float uMetallic;
uniform float uRoughness;

// Which value to show on its own, 0 is the normal lit picture
uniform int uDebugView;

// Blend weights, 0 is physical and 1 is stylized
uniform float uWeightShadow;
uniform float uWeightDiffuse;
uniform float uWeightSpecular;
uniform float uWeightAmbient;


// Half vector, the direction halfway between the light and the camera
// Both sides use it for their highlight.
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
