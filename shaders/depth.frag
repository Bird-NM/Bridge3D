#version 330 core
in vec3 worldPosition;
uniform int part, clearanceCutaway;
uniform float cutawayY;
void main() {
  if (clearanceCutaway == 1 && (part == 0 || part == 3 || part == 7) &&
      worldPosition.y > cutawayY)
    discard;
}
