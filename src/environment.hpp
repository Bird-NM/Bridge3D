#pragma once
#include "model.hpp"
struct EnvironmentConfig {
  float approachLength = 90, terrainExtension = 20, bankHalfWidth = 70,
        shoreRun = 14;
  float roadCurve = 20, roadDrop = 3, hillHeight = 15, shorelineHeight = 0;
  float promenadeWidth = 4, treeSpacing = 15, buildingSetback = 40;
  float coastVariation = 16, plantingDepth = 110, buildingStart = 35,
        buildingEnd = 100;
  float treeHeight = 6.5f, treeRadius = 2.1f, roadShoulder = 12;
  int resolution = 32, treeCount = 28, buildingCount = 5, seed = 73;
  glm::vec3 waterColor{.12f, .31f, .40f};
  void load(const std::string &path);
};
struct RoadConnection {
  glm::vec3 center, tangent;
  float carriagewayWidth, sidewalkWidth;
};
struct EnvironmentModel {
  EnvironmentConfig config;
  std::vector<Component> parts;
  std::vector<RoadConnection> connections;
  glm::vec3 foregroundMinimum{0}, foregroundMaximum{0};
  void build(const Model &bridge, const std::string &configPath);
  glm::vec3 roadCenter(const Parameters &p, float side, float distance) const;
  float terrainHeight(const Parameters &p, float side, float distance,
                      float z) const;
  float coastline(const Parameters &p, float z) const;
  void bounds(glm::vec3 &minimum, glm::vec3 &maximum) const;
};
