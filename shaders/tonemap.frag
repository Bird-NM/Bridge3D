#version 330 core
in vec2 screenPosition;
out vec4 outputColor;
uniform sampler2D scene;
uniform int presentation;
vec3 displayColor(vec3 color) {
  color = max(color, vec3(0));
  return mix(color * 12.92, 1.055 * pow(color, vec3(1. / 2.4)) - .055,
             step(vec3(.0031308), color));
}
void main() {
  vec3 linear = texture(scene, screenPosition * .5 + .5).rgb;
  vec3 mapped =
      presentation == 1 ? 1. - exp(-linear * 1.15) : clamp(linear, 0., 1.);
  outputColor = vec4(displayColor(mapped), 1);
}
