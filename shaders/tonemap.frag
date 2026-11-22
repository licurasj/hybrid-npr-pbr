#version 410 core

// Last pass: turns the HDR picture into colors for the screen
// Scene pixels get exposure, a tone curve and the linear to sRGB step.
// Background, grid and axes only get the sRGB step, so they look the same whatever the settings.
// The alpha channel says which is which: the model writes 1 and everything else 0.
// Each pixel of the HDR picture has several samples. On the edge of the model some belong to
// the model and some to what is behind it, so every sample gets its own treatment first and
// the results are averaged at the end.

out vec4 fragColor;

// The multisampled HDR picture, read one sample at a time
uniform sampler2DMS uHdrImage;

// How many samples each pixel has
uniform int uSampleCount;

// Brightness factor before the curve, 1 leaves the picture as it is
uniform float uExposure;

// 0 Off (just cut off at 1), 1 Reinhard, 2 ACES fit
uniform int uToneCurve;

// How much of exposure and the curve scene pixels get
// 1 is all of it, 0 none (the color is only cut off at 1), in between mixes the two results.
uniform float uToneAmount;

// 1 turns scene pixels from linear to sRGB, 0 shows the values as they are stored,
// which the data debug views (metallic, roughness, normal, shadow) need
uniform int uSceneToSrgb;


// Squeeze a color that can go above 1 into 0 to 1
vec3 applyToneCurve(vec3 c) {
  if (uToneCurve == 1) {
    // Reinhard: bright values get closer to 1 but never reach it
    return c / (1.0 + c);
  } else if (uToneCurve == 2) {
    // ACES fit by Krzysztof Narkowicz, a curve that looks like film
    vec3 curve = (c * (2.51 * c + 0.03)) / (c * (2.43 * c + 0.59) + 0.14);
    return clamp(curve, 0.0, 1.0);
  }
  // Off: everything above 1 is cut off
  return clamp(c, 0.0, 1.0);
}


// Linear to sRGB for each channel, the formula of the sRGB standard
// The input must be between 0 and 1.
vec3 linearToSrgb(vec3 c) {
  vec3 low = 12.92 * c;
  vec3 high = 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055;
  return mix(high, low, lessThanEqual(c, vec3(0.0031308)));
}


// The screen color of one sample
vec3 treatSample(vec4 hdr) {
  // Negative values cannot be shown, and pow does not work on them
  vec3 color = max(hdr.rgb, 0.0);

  // Background, grid and axes: only the sRGB step
  if (hdr.a < 0.5) {
    return linearToSrgb(clamp(color, 0.0, 1.0));
  }

  // Scene: with and without exposure and the curve, then mixed
  vec3 plain = clamp(color, 0.0, 1.0);
  vec3 toned = applyToneCurve(color * uExposure);
  if (uSceneToSrgb == 1) {
    plain = linearToSrgb(plain);
    toned = linearToSrgb(toned);
  }
  return mix(plain, toned, uToneAmount);
}


void main() {
  // The HDR picture has the same size as the window, so the pixel position is also the texel
  ivec2 pixel = ivec2(gl_FragCoord.xy);

  // Treat every sample on its own, then average
  vec3 sum = vec3(0.0);
  for (int i = 0; i < uSampleCount; i++) {
    sum += treatSample(texelFetch(uHdrImage, pixel, i));
  }
  fragColor = vec4(sum / float(uSampleCount), 1.0);
}
