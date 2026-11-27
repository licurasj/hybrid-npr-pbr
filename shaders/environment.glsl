// Environment image, shared by the hybrid shader and the sky
// The program puts the version line in front. In the hybrid shader this file comes after
// common.glsl, so the term files can use it.


// The environment as a cubemap, with its full chain of smaller, blurrier copies (mip levels)
uniform samplerCube uEnvironment;    // texture unit 4

// Number of mip levels of the cubemap
// GLSL 4.10 has no textureQueryLevels, so the program passes it in.
uniform int uEnvironmentMipCount;

// Turn of the environment around the vertical axis, in radians
uniform float uEnvironmentRotation;

// Brightness factor for the whole environment: sky, fill light and reflections
uniform float uEnvironmentIntensity;

// Largest value the HDR picture can hold
// It stores 16-bit floats, which stop at 65504. The sun in the environment is already lowered to
// that when the image is loaded, but the Intensity slider can push it past again. Some graphics
// cards store such a value as infinity, and the tone curve then turns it into NaN (a black dot),
// so the model and the sky lower their color to this value before writing it.
const float HDR_MAX = 65504.0;


// Direction in the cubemap for a direction in the world
// Turning the environment by an angle is the same as turning the lookup the other way,
// so the world direction is turned by minus the angle around the Y axis.
vec3 environmentDirection(vec3 d) {
  float c = cos(uEnvironmentRotation);
  float s = sin(uEnvironmentRotation);
  return vec3(c * d.x - s * d.z, d.y, s * d.x + c * d.z);
}

// Light from a world direction in one of the environment cubemaps, from mip level lod
// The environment itself and the maps made from it share the turn and the brightness.
vec3 readEnvironmentMap(samplerCube map, vec3 d, float lod) {
  return textureLod(map, environmentDirection(d), lod).rgb * uEnvironmentIntensity;
}

// Environment light coming from a world direction, from mip level lod
// Level 0 is the sharp image, each level up is half as wide and so blurrier.
vec3 sampleEnvironment(vec3 d, float lod) {
  return readEnvironmentMap(uEnvironment, d, lod);
}
