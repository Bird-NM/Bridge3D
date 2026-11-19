#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
struct Vertex {
  glm::vec3 position, normal;
  float mobility = 1.f;
};
struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<unsigned> triangles, edges;
};
struct Component {
  int id;
  std::string name, material, role;
  glm::vec3 color;
  Mesh mesh;
  bool visible = true;
  glm::vec3 min{1e9f}, max{-1e9f};
};
struct Parameters {
  float length = 240, width = 10, deckHeight = 17, towerHeight = 66,
        towerX = 62, cableSag = 35;
  int hangers = 96, cableSegments = 160, trussPanels = 80, stayCount = 22;
  float barRadius = .22f, cableRadius = .24f;
  void load(const std::string &path);
};
struct Model {
  Parameters p;
  bool hieu = false;
  std::vector<Component> parts;
  void build(bool useHieu);
};
void beam(Mesh &m, glm::vec3 a, glm::vec3 b, float radius, int sides = 4,
          float startMobility = 1.f, float endMobility = 1.f);
void box(Mesh &m, glm::vec3 center, glm::vec3 size);
float displacement(float x, float loadX, float amplitude, const Parameters &p,
                   bool hieu);
