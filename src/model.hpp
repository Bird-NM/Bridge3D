#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
struct Vertex {
  glm::vec3 position, normal;
  float mobility = 1.f;
  glm::vec3 tint{1};
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
  float carriagewayWidth = 7, sidewalkWidth = 1.5f, sidewalkRaise = .12f;
  float towerLegWidth = 2, towerLegDepth = 3, lateralClearance = .6f;
  float vehicleClearanceHeight = 5, hieuTowerX = 0;
  void load(const std::string &path);
};
struct CableAttachment {
  glm::vec3 upper, lower;
};
struct Model {
  Parameters p;
  bool hieu = false;
  std::vector<Component> parts;
  std::vector<CableAttachment> cableAttachments;
  void build(bool useHieu);
};
void beam(Mesh &m, glm::vec3 a, glm::vec3 b, float radius, int sides = 4,
          float startMobility = 1.f, float endMobility = 1.f);
void box(Mesh &m, glm::vec3 center, glm::vec3 size);
float displacement(float x, float loadX, float amplitude, const Parameters &p,
                   bool hieu);

// Common geometric nodes used by the tower, decorative braces and stay anchors.
glm::vec3 hieuLegCenter(const Parameters &p, float y, float side);
void quad(Mesh &mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);
void updateBounds(Component &component);

void triangle(Mesh &mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c);

void ellipsoid(Mesh &mesh, glm::vec3 center, glm::vec3 radii, int rings = 8,
               int slices = 12);
