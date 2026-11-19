#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in float mobility;
layout(location = 3) in vec3 vertexTint;
uniform mat4 vp, modelMatrix, lightVP;
uniform mat3 normalMatrix;
uniform float amplitude, loadX, length, towerX, towerHeight, deckHeight,
    explode, hieuTowerX;
uniform int hieu, part;
out vec3 N, worldPosition, tint;
out vec4 lightPosition;
float delta(float x) {
  float l = -length / 2., r = length / 2.;
  if (hieu == 1) {
    if (x < hieuTowerX)
      r = hieuTowerX;
    else
      l = hieuTowerX;
  } else {
    if (x < -towerX)
      r = -towerX;
    else if (x < towerX) {
      l = -towerX;
      r = towerX;
    } else
      l = towerX;
  }
  float t = clamp((x - l) / (r - l), 0., 1.);
  return -amplitude * pow(sin(3.14159265 * t), 2.) *
         exp(-pow((x - loadX) / 45., 2.));
}
void main() {
  vec3 p = pos;
  N = normal;
  if (part > 0 && part < 6) {
    float w =
        clamp((towerHeight - pos.y) / (towerHeight - deckHeight - 2.), 0., 1.) *
        mobility;
    p.y += delta(pos.x) * w;
    float dx = (delta(pos.x + .02) - delta(pos.x - .02)) / .04 * w;
    float dy = pos.y > deckHeight + 2. && pos.y < towerHeight
                   ? -delta(pos.x) * mobility / (towerHeight - deckHeight - 2.)
                   : 0.;
    N = vec3(normal.x - dx * normal.y / (1. + dy), normal.y / (1. + dy),
             normal.z);
  }
  p.y += explode * (part >= 8                ? 0.
                    : part == 0 || part == 7 ? 2.
                    : part == 2 || part == 3 ? 4.
                    : part == 6              ? 0.
                                             : 1.);
  worldPosition = (modelMatrix * vec4(p, 1.)).xyz;
  N = normalMatrix * N;
  tint = vertexTint;
  lightPosition = lightVP * vec4(worldPosition, 1.);
  gl_Position = vp * vec4(worldPosition, 1.);
  gl_PointSize = 3.;
}
