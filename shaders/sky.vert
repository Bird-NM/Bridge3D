#version 330 core
out vec2 screenPosition;
void main() {
  vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
  screenPosition = p * 2. - 1.;
  gl_Position = vec4(screenPosition, 1, 1);
}
