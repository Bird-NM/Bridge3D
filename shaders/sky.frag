#version 330 core
in vec2 screenPosition;
out vec4 outputColor;
uniform mat4 inverseVP;
uniform vec3 eye, sunDirection;
void main() {
  vec4 farPoint = inverseVP * vec4(screenPosition, 1, 1);
  vec3 direction = normalize(farPoint.xyz / farPoint.w - eye);
  vec3 sky = mix(vec3(.69, .76, .81), vec3(.16, .36, .63),
                 pow(clamp(direction.y, 0., 1.), .35));
  float sun = pow(max(dot(direction, sunDirection), 0.), 600.);
  sky += vec3(1, .87, .65) * sun * .35;
  outputColor = vec4(sky, 1);
}
