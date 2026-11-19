#pragma once
#include <glm/glm.hpp>
struct Camera {
  glm::vec3 target{0, 28, 0}, desiredTarget{0, 28, 0};
  float yaw = .25f, pitch = .27f, distance = 270, desiredYaw = .25f,
        desiredPitch = .27f, desiredDistance = 270;
  bool ortho = false, smooth = true;
  glm::vec3 eye() const;
  glm::mat4 matrix(float aspect) const;
  void update(float dt);
  void preset(int view);
  void fit(glm::vec3 center, float radius, float aspect);
  void orbit(float dx, float dy);
  void pan(float dx, float dy, float height);
};
