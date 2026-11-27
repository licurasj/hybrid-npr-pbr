// The sky behind the model
// The program puts the version line and environment.glsl in front of this file.


in vec3 skyDirection;

out vec4 fragColor;

// Mip level the sky is read from: 0 for the sharp sky, higher for the blurred sky
uniform float uSkyLod;


void main() {
  // A cubemap is read with any direction, it does not have to be unit length
  vec3 color = sampleEnvironment(skyDirection, uSkyLod);
  // Keep it inside what the HDR picture can hold (see HDR_MAX in environment.glsl)
  color = min(color, vec3(HDR_MAX));

  // Alpha 1 marks the sky as part of the scene, so it gets exposure and the tone curve like the model
  fragColor = vec4(color, 1.0);
}
