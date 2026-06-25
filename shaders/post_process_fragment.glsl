#version 450 core

// Highest resolution (though there's only one)
#define HIGHEST_MIP 0

// Input texture to sample
uniform sampler2D uTexture;

// out: goes to framebuffer
out vec4 fragColor;

void main() {
  // Simply copy the texture into the framebuffer
  fragColor = texelFetch(uTexture, ivec2(gl_FragCoord.xy), HIGHEST_MIP);
}