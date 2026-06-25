#version 450 core

// Highest resolution (though there's only one)
#define HIGHEST_MIP 0
#define MIN_ACCUM_ALPHA 1e-5F
#define REVEAL_COMPLEMENT 1.0F

#define ACCUM_COLOR_ATTACHMENT 0
#define REVEAL_COLOR_ATTACHMENT 1

// Input texture to sample
// bind your accum render target to this texture unit
layout(binding = ACCUM_COLOR_ATTACHMENT) uniform sampler2D uAccumTexture;

// bind your reveal render target to this texture unit
layout(binding = REVEAL_COLOR_ATTACHMENT) uniform sampler2D uRevealTexture;

// out: goes to framebuffer
out vec4 fragColor;

void main() {
  // Source: https://learnopengl.com/Guest-Articles/2020/OIT/Weighted-Blended
  const vec4 accum = texelFetch(uAccumTexture, ivec2(gl_FragCoord.xy), HIGHEST_MIP);
  const float reveal = texelFetch(uRevealTexture, ivec2(gl_FragCoord.xy), HIGHEST_MIP).r;
  fragColor = vec4(accum.rgb / max(accum.a, MIN_ACCUM_ALPHA), (REVEAL_COMPLEMENT - reveal));
}