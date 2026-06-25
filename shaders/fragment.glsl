// Runs once per pixel to decide its colour
#version 450 core

// Phong lighting: finalColor = (ambient + diffuse + specular) * objectColor
// - Ambient: background lighting that illuminates all objects equally
// - Diffuse: general brightness of a spot, proportional to angle between
//            normal and light direction (direct hit by light source ->
//            brighter)
// - Specular: shiny spots, proportional to angle between reflected light
//             and observer (light enters your eye -> see bright spots)
#define AMBIENT_STRENGTH 0.7F
#define SPECULAR_STRENGTH 0.7F
#define SHININESS 64.0F
#define LIGHT_COLOR vec3(1.0F, 1.0F, 1.0F) // white light
#define MIN_CONTRIBUTION 0.0F

// Calculated by vertex shaders
in flat vec3 vNormal;
in flat vec3 vFragPos;
// Provided by CPU

uniform vec3 uLightPos;
uniform vec3 uCameraPos;
uniform bool uLighting; // Flag to turn on Phong lighting

// Declares output variable (a colour variable is mandatory)
// Variables are passed between shaders through interface matching by name and
// type, checked during linking (fragment shader interpolates values as it is
// run more frequently than the vertex shader)
in flat vec4 vColor;
// out: goes to framebuffer
out vec4 fragColor;

void main() {
  fragColor = vColor;

  if (uLighting) {
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);
    vec3 cameraDir = normalize(uCameraPos - vFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    vec3 ambient = AMBIENT_STRENGTH * LIGHT_COLOR;
    vec3 diffuse = max(dot(norm, lightDir), MIN_CONTRIBUTION) * LIGHT_COLOR;
    vec3 specular =
        SPECULAR_STRENGTH *
        pow(max(dot(cameraDir, reflectDir), MIN_CONTRIBUTION), SHININESS) *
        LIGHT_COLOR;

    // Component-wise multiplication
    // .rgba and .xyzw are GLSL's swizzling syntax
    fragColor = vec4((ambient + diffuse + specular) * vColor.rgb, vColor.a);
  }
}
