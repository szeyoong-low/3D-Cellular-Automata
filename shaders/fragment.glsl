// Runs once per pixel to decide its colour
#version 450 core

#define ACCUM_COLOR_ATTACHMENT 0 // Also a normal output if uOpacity is false
#define REVEAL_COLOR_ATTACHMENT 1

#define MIN_WEIGHT 1e-2F
#define MAX_WEIGHT 3e3F
#define MIN_COMPONENT 1.0F
#define ALPHA_FACTOR 10.0F
#define ALPHA_BIAS 0.01F
#define ALPHA_POWER 3.0F
#define WEIGHT_FACTOR 1e8F
#define Z_POWER 3.0F
#define Z_FACTOR 0.9F
#define Z_RECIPROCAL_CONSTANT 1.0F

// Phong lighting: finalColor = (ambient + diffuse + specular) * objectColor
// - Ambient: background lighting that illuminates all objects equally
// - Diffuse: general brightness of a spot, proportional to angle between
//            normal and light direction (direct hit by light source ->
//            brighter)
// - Specular: shiny spots, proportional to angle between reflected light
//             and observer (light enters your eye -> see bright spots)
#define AMBIENT_STRENGTH 0.7F
#define SPECULAR_STRENGTH 0.7F
#define SHININESS 128.0F
#define LIGHT_COLOR vec3(1.0F, 1.0F, 1.0F) // white light
#define MIN_CONTRIBUTION 0.0F

// Calculated by vertex shaders
in flat vec3 vNormal;
in flat vec3 vFragPos;
// Provided by CPU

uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform bool uLighting; // Flag to turn on Phong lighting

// Flag to determine whether post-processing is needed
uniform bool uOpacity;

// Declares output variable (a colour variable is mandatory)
// Variables are passed between shaders through interface matching by name and
// type, checked during linking (fragment shader interpolates values as it is
// run more frequently than the vertex shader)
in flat vec4 vColor;
// out: goes to framebuffer
// ACCUM_COLOR_ATTACHMENT is just fragColor if uOpacity is false
layout(location = ACCUM_COLOR_ATTACHMENT) out vec4 accum;    // Pre-multiplied colours
// Discarded if no attachment is bound here
layout(location = REVEAL_COLOR_ATTACHMENT) out float reveal; // Pixel revealage

void main() {
  vec4 fragColor = vColor;

  if (uLighting) {
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);
    vec3 cameraDir = normalize(uCameraPos - vFragPos);
    vec3 halfwayDir = normalize(lightDir + cameraDir);

    vec3 ambient = AMBIENT_STRENGTH * LIGHT_COLOR;
    vec3 diffuse = max(dot(norm, lightDir), MIN_CONTRIBUTION) * LIGHT_COLOR;
    vec3 specular =
        SPECULAR_STRENGTH *
        pow(max(dot(norm, halfwayDir), MIN_CONTRIBUTION), SHININESS) *
        LIGHT_COLOR;

    // Component-wise multiplication
    // .rgba and .xyzw are GLSL's swizzling syntax
    fragColor = vec4((ambient + diffuse + specular) * vColor.rgb, vColor.a);
  }

  if (uOpacity) {
    // Source: https://learnopengl.com/Guest-Articles/2020/OIT/Weighted-Blended
    const float weight = clamp(
        pow(min(MIN_COMPONENT, fragColor.a * ALPHA_FACTOR) + ALPHA_BIAS,
            ALPHA_POWER) *
            WEIGHT_FACTOR *
            pow(Z_RECIPROCAL_CONSTANT - gl_FragCoord.z * Z_FACTOR, Z_POWER),
        MIN_WEIGHT, MAX_WEIGHT);

    accum = vec4(fragColor.rgb * fragColor.a, fragColor.a) * weight;
    reveal = fragColor.a;
  } else {
    accum = fragColor;
  }
}
