#pragma once
#include "environment.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
// Triangle/AABB separating-axis test, including triangle interiors and edges.
static bool triangleBox(glm::vec3 a, glm::vec3 b, glm::vec3 c,
                        glm::vec3 minimum, glm::vec3 maximum) {
  auto center = (minimum + maximum) / 2.f, half = (maximum - minimum) / 2.f;
  a -= center;
  b -= center;
  c -= center;
  std::array<glm::vec3, 3> axes{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
  auto separated = [&](glm::vec3 axis) {
    if (glm::dot(axis, axis) < 1e-12f)
      return false;
    float x = glm::dot(a, axis), y = glm::dot(b, axis), z = glm::dot(c, axis);
    float radius = glm::dot(glm::abs(axis), half);
    return std::min({x, y, z}) > radius + 1e-5f ||
           std::max({x, y, z}) < -radius - 1e-5f;
  };
  for (auto axis : axes)
    if (separated(axis))
      return false;
  auto ab = b - a, bc = c - b, ca = a - c;
  if (separated(glm::cross(ab, bc)))
    return false;
  for (auto edge : {ab, bc, ca})
    for (auto axis : axes)
      if (separated(glm::cross(edge, axis)))
        return false;
  return true;
}
static void requireClear(const Component &part, glm::vec3 minimum,
                         glm::vec3 maximum) {
  for (size_t i = 0; i < part.mesh.triangles.size(); i += 3) {
    auto a = part.mesh.vertices[part.mesh.triangles[i]].position;
    auto b = part.mesh.vertices[part.mesh.triangles[i + 1]].position;
    auto c = part.mesh.vertices[part.mesh.triangles[i + 2]].position;
    if (triangleBox(a, b, c, minimum, maximum))
      throw std::runtime_error("Clearance intersection in " + part.name);
  }
}
static void checkClearance(Model &model) {
  const auto &p = model.p;
  float floor = p.deckHeight + .6f;
  float half = p.carriagewayWidth / 2 + p.lateralClearance;
  glm::vec3 minimum{-p.length / 2, floor + .01f, -half},
      maximum{p.length / 2, floor + p.vehicleClearanceHeight, half};
  for (int part : {0, 3, 6, 7})
    requireClear(model.parts[part], minimum, maximum);
  // Pedestrian envelope stays open on both sidewalks as well.
  for (float side : {-1.f, 1.f}) {
    float lo = side < 0 ? -p.width / 2 : p.carriagewayWidth / 2;
    float hi = side < 0 ? -p.carriagewayWidth / 2 : p.width / 2;
    for (int part : {0, 6, 7})
      requireClear(model.parts[part], {-p.length / 2, floor + .01f, lo},
                   {p.length / 2, floor + 2.2f, hi});
  }
  for (int i = 0; i <= 60; ++i) {
    float x = p.hieuTowerX - 15 + i * .5f;
    // Bounding envelope of the actual sample truck, swept across the tower.
    for (int part : {0, 3, 6, 7})
      requireClear(model.parts[part], {x - 3.5f, floor + .01f, -1.6f},
                   {x + 5.3f, floor + 3.7f, 1.6f});
  }
  for (float side : {-1.f, 1.f}) {
    auto node = hieuLegCenter(p, floor, side);
    require(std::abs(node.x - p.hieuTowerX) < 1e-6f,
            "Legs offset along bridge axis");
    require(std::abs(node.z) - p.towerLegWidth / 2 >=
                p.width / 2 + p.lateralClearance - .001f,
            "Leg encroaches on sidewalk");
  }
  std::cout
      << "Full vehicle/pedestrian volumes and 61 swept truck positions clear\n";
}
static float terrainMeshHeight(const EnvironmentModel &scene, glm::vec3 point) {
  float height = -1e9f;
  for (int group : {0, 1, 4}) {
    auto &mesh = scene.parts[group].mesh;
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
      auto a = mesh.vertices[mesh.triangles[i]].position;
      auto b = mesh.vertices[mesh.triangles[i + 1]].position;
      auto c = mesh.vertices[mesh.triangles[i + 2]].position;
      glm::vec2 e1{b.x - a.x, b.z - a.z}, e2{c.x - a.x, c.z - a.z},
          delta{point.x - a.x, point.z - a.z};
      float det = e1.x * e2.y - e1.y * e2.x;
      if (std::abs(det) < 1e-8f)
        continue;
      float u = (delta.x * e2.y - delta.y * e2.x) / det,
            v = (e1.x * delta.y - e1.y * delta.x) / det;
      if (u >= -.0001f && v >= -.0001f && u + v <= 1.0001f)
        height = std::max(height, a.y + u * (b.y - a.y) + v * (c.y - a.y));
    }
  }
  return height;
}
static void checkEnvironment(const Model &model,
                             const EnvironmentModel &scene) {
  const auto &p = model.p;
  for (int i = 0; i < 2; ++i) {
    float side = i == 0 ? -1.f : 1.f;
    auto point = scene.roadCenter(p, side, 0);
    require(glm::length(point - glm::vec3(side * p.length / 2,
                                          p.deckHeight + .6f, 0)) < 1e-5f,
            "Approach start disconnected");
    auto tangent = glm::normalize(scene.roadCenter(p, side, .001f) - point);
    require(glm::dot(tangent, glm::vec3(side, 0, 0)) > .999f,
            "Approach tangent discontinuity");
    require(scene.connections[i].carriagewayWidth == p.carriagewayWidth,
            "Road width mismatch");
    for (int j = 0; j <= 100; ++j) {
      float d = scene.config.approachLength * j / 100.f;
      auto center = scene.roadCenter(p, side, d);
      require(scene.terrainHeight(p, side, d, center.z) < center.y - .1f,
              "Ground penetrates approach");
    }
  }
  for (int group : {2, 3})
    for (auto vertex : scene.parts[group].mesh.vertices)
      if (vertex.normal.y > .9f) {
        float terrain = terrainMeshHeight(scene, vertex.position);
        if (terrain >= vertex.position.y - .01f) {
          std::cerr << "group " << group << " at " << vertex.position.x << ","
                    << vertex.position.y << "," << vertex.position.z
                    << " terrain=" << terrain << "\n";
          throw std::runtime_error(
              "Terrain triangles penetrate road/walkway top");
        }
      }

  for (auto &part : scene.parts) {
    require(part.id >= 100, "Environment may deform with bridge");
    for (size_t i = 0; i < part.mesh.triangles.size(); i += 3) {
      auto a = part.mesh.vertices[part.mesh.triangles[i]],
           b = part.mesh.vertices[part.mesh.triangles[i + 1]],
           c = part.mesh.vertices[part.mesh.triangles[i + 2]];
      auto face = glm::cross(b.position - a.position, c.position - a.position);
      require(glm::length(face) > 1e-8f, "Degenerate landscape triangle");
      require(glm::dot(face, a.normal + b.normal + c.normal) > 0,
              "Landscape winding/normal mismatch");
    }
    for (auto v : part.mesh.vertices)
      require(std::isfinite(glm::length(v.position)) &&
                  std::abs(glm::length(v.normal) - 1) < .001f,
              "Invalid scene vertex/normal");
  }
  // Landscape placement must remain outside the entire bridge span/cable
  // volume.
  for (int group : {5, 6, 10})
    requireClear(scene.parts[group], {-p.length / 2, 0, -p.width},
                 {p.length / 2, p.towerHeight, p.width});
  EnvironmentModel again;
  again.build(model, std::string(BRIDGE_ROOT) +
                         (model.hieu ? "/config/hieu-environment.cfg"
                                     : "/config/golden-environment.cfg"));
  for (size_t i = 0; i < scene.parts.size(); ++i) {
    require(scene.parts[i].mesh.vertices.size() ==
                again.parts[i].mesh.vertices.size(),
            "Scene seed count changed");
    for (size_t v = 0; v < scene.parts[i].mesh.vertices.size(); ++v)
      require(glm::length(scene.parts[i].mesh.vertices[v].position -
                          again.parts[i].mesh.vertices[v].position) < 1e-6f,
              "Scene seed changed geometry");
  }
  std::cout << "Approach position/tangent/width, terrain corridor, landscape "
               "exclusion and deterministic seed passed\n";
}
