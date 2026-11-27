// Stylized side of the hybrid shader
// Toon diffuse from a ramp image and a flat highlight whose size follows the roughness.


// Uniforms used only by the stylized side
uniform vec3 uAmbientColor;
uniform sampler2D uRampTexture;     // texture unit 5, a gray ramp, 256 x 1
uniform float uHighlightThreshold;  // where the highlight edge sits, 0 to 1
uniform float uHighlightStrength;   // 1 is as bright as a white surface facing the light
uniform bool uHighlightFollowsRamp; // when on, the highlight is dimmed by the ramp band it is in

// Smallest roughness the stylized highlight uses
// Below about 0.15 the highlight would be smaller than a pixel, so smooth surfaces get a small
// but visible one. Only the highlight uses it, the roughness debug view still shows the real value.
const float TOON_MIN_ROUGHNESS = 0.2;

// Smallest soft edge width
// smoothstep needs its first edge below its second. Where a value does not change between pixels
// (the highlight value of a fully rough surface is the same everywhere) its change is 0, so a tiny
// width is kept.
const float MIN_EDGE_WIDTH = 1e-4;


// Shadow with a hard edge
float shadowStylized(float vis) {
  return step(0.5, vis);
}

// Half width of a soft edge for the value x, about 0.7 pixels (Gustavson's aastep)
// dFdx and dFdy are how much x changes to the next pixel across and down, so their length is the
// change per pixel across the edge. smoothstep(edge - w, edge + w, x) then goes from 0 to 1 over
// about 1.4 pixels in every direction. This must run for every pixel, so not inside an if.
float edgeWidth(float x) {
  return max(0.7 * length(vec2(dFdx(x), dFdy(x))), MIN_EDGE_WIDTH);
}

// Brightness of the ramp at position t, from 0 (dark end) to 1 (lit end)
// The texture reads the image as sRGB, so the value comes out linear.
vec3 rampLookup(float t) {
  return texture(uRampTexture, vec2(t, 0.5)).rgb;
}

// Position in the ramp for this pixel, from 0 (dark end) to 1 (lit end)
// N dot L is moved from minus 1 to 1 into 0 to 1 without clamping first, so the side facing
// away from the light still has ramp positions below 0.5.
// The shadow moves this lookup position instead of being multiplied on afterward.
float rampPosition(Surface s, Light l, float shadow) {
  return (dot(s.N, l.L) * 0.5 + 0.5) * shadow;
}

// Toon diffuse: the light falls into the bands of the ramp
// Metals keep their base color here, a black toon metal would look wrong.
vec3 diffuseStylized(Surface s, Light l, float shadow) {
  return s.baseColor / PI * rampLookup(rampPosition(s, l, shadow)) * l.radiance;
}

// Toon highlight: the GGX highlight divided by its peak, cut at a threshold
// GGX is D = a^2 / (pi ((N.H)^2 (a^2 - 1) + 1)^2) with a = roughness^2, and its peak at N.H = 1
// is 1 / (pi a^2). Divided by the peak it goes from 1 in the middle of the highlight toward 0,
// so the threshold sits at the same place for every roughness, and a rougher surface gets a
// larger spot.
vec3 specularStylized(Surface s, Light l, float shadow) {
  float roughness = clamp(s.roughness, 0.0, 1.0);
  float r = max(roughness, TOON_MIN_ROUGHNESS);
  float a = r * r;
  float a2 = a * a;
  float NdotH = clamp(dot(s.N, halfVector(s, l)), 0.0, 1.0);
  float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
  // The ratio can not be above 1, but rounding in a2 - 1 + 1 can push it just over. Then
  // Threshold 1 would still leave a tiny spot in the middle, so it is kept at 1 at most.
  float ratio = min((a2 * a2) / (d * d), 1.0);

  // Soft edge about 1.4 pixels wide, so it is not jagged
  // The soft window slides with the threshold: at 0.5 it is centered on the edge, at 1 it lies
  // fully above the peak, so there is no highlight at all, and at 0 fully below 0, so the whole
  // lit side is covered flat.
  float T = uHighlightThreshold;
  float w = edgeWidth(ratio);
  float mask = smoothstep(T - 2.0 * w * (1.0 - T), T + 2.0 * w * T, ratio);

  // No highlight on the side facing away from the light. Instead of a hard cut at N.L = 0 the
  // highlight fades out over the last 1.4 pixels or so of the lit side.
  float NdotL = dot(s.N, l.L);
  float fade = smoothstep(0.0, 2.0 * edgeWidth(NdotL), NdotL);

  // Rougher surfaces get a dimmer highlight, like on the physical side
  // Dividing by pi makes strength 1 as bright as a white surface facing the light.
  float brightness = uHighlightStrength * (1.0 - roughness) / PI;

  // Metals color their highlight with the base color, non-metals keep it white
  vec3 tint = mix(vec3(1.0), s.baseColor, s.metallic);
  vec3 highlight = mask * fade * brightness * tint * l.radiance * shadow;

  // Optional: a large highlight that reaches past a band edge takes that band's brightness,
  // like a painted toon highlight. Off, it is one clean shape that ends only at N.L = 0.
  if (uHighlightFollowsRamp) {
    highlight *= rampLookup(rampPosition(s, l, shadow));
  }
  return highlight;
}

// Flat ambient color times the base color
vec3 ambientStylized(Surface s) {
  return uAmbientColor * s.baseColor;
}

// No stylized reflections yet, so moving the Reflections slider toward NPR fades them out
vec3 reflectionsStylized(Surface s) {
  return vec3(0.0);
}
