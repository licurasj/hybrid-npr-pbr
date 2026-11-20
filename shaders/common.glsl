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
uniform vec3 uBaseColor;
uniform float uMetallic;
uniform float uRoughness;
uniform float uShininess;

// Which value to show on its own, 0 is the normal lit picture
uniform int uDebugView;

// Blend weights, 0 is physical and 1 is stylized
uniform float uWeightShadow;
uniform float uWeightDiffuse;
uniform float uWeightSpecular;
uniform float uWeightAmbient;


// Phong highlight used by both specular stand-ins
// It goes away once the real specular terms are written.
float phongHighlight(Surface s, Light l) {
  // No highlight on the side facing away from the light
  if (dot(s.N, l.L) <= 0.0) {
    return 0.0;
  }
  vec3 R = reflect(-l.L, s.N);
  return pow(max(dot(R, s.V), 0.0), uShininess);
}
