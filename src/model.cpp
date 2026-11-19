#include "model.hpp"
#include <algorithm>
#include <fstream>
#include <glm/gtc/constants.hpp>
#include <sstream>
#include <stdexcept>
void Parameters::load(const std::string &path) {
  std::ifstream f(path);
  if (!f)
    throw std::runtime_error("Cannot open parameters: " + path);
  std::string line;
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    auto split = line.find('=');
    if (split == std::string::npos)
      continue;
    auto key = line.substr(0, split);
    float v = std::stof(line.substr(split + 1));
    if (key == "length")
      length = v;
    else if (key == "width")
      width = v;
    else if (key == "deckHeight")
      deckHeight = v;
    else if (key == "towerHeight")
      towerHeight = v;
    else if (key == "towerX")
      towerX = v;
    else if (key == "cableSag")
      cableSag = v;
    else if (key == "hangers")
      hangers = int(v);
    else if (key == "cableSegments")
      cableSegments = int(v);
    else if (key == "trussPanels")
      trussPanels = int(v);
    else if (key == "stayCount")
      stayCount = int(v);
    else if (key == "barRadius")
      barRadius = v;
    else if (key == "cableRadius")
      cableRadius = v;
  }
  if (length <= 0 || width <= 0 || deckHeight <= 4 || towerX <= 0 ||
      towerX >= length / 2 || towerHeight <= deckHeight + cableSag ||
      cableSag <= 0 || hangers < 4 || hangers > 1000 || cableSegments < 4 ||
      cableSegments > 2000 || trussPanels < 4 || trussPanels > 1000 ||
      stayCount < 2 || stayCount > 200 || barRadius <= 0 || cableRadius <= 0)
    throw std::runtime_error("Invalid bridge parameters");
}
void beam(Mesh &m, glm::vec3 a, glm::vec3 b, float r, int sides,
          float startMobility, float endMobility) {
  auto d = glm::normalize(b - a);
  auto u = glm::normalize(glm::cross(
      d, std::abs(d.y) < .9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)));
  auto v = glm::cross(d, u);
  unsigned base = unsigned(m.vertices.size());
  for (int end = 0; end < 2; ++end)
    for (int i = 0; i < sides; ++i) {
      float t = glm::two_pi<float>() * i / sides + glm::pi<float>() / 4;
      auto n = u * std::cos(t) + v * std::sin(t);
      m.vertices.push_back(
          {(end ? b : a) + r * n, n, end ? endMobility : startMobility});
    }
  for (int i = 0; i < sides; ++i) {
    unsigned j = (i + 1) % sides, A = base + i, B = base + j, C = B + sides,
             D = A + sides;
    m.triangles.insert(m.triangles.end(), {A, B, C, A, C, D});
    m.edges.insert(m.edges.end(), {A, B, A, D, C, D});
  }
  for (int end = 0; end < 2; ++end) {
    unsigned cap = unsigned(m.vertices.size());
    for (int i = 0; i < sides; ++i)
      m.vertices.push_back({m.vertices[base + end * sides + i].position,
                            end ? d : -d, end ? endMobility : startMobility});
    for (int i = 1; i < sides - 1; ++i) {
      if (end)
        m.triangles.insert(m.triangles.end(),
                           {cap, cap + unsigned(i), cap + unsigned(i + 1)});
      else
        m.triangles.insert(m.triangles.end(),
                           {cap, cap + unsigned(i + 1), cap + unsigned(i)});
    }
  }
}
void box(Mesh &m, glm::vec3 c, glm::vec3 s) {
  const glm::vec3 normals[] = {{1, 0, 0},  {-1, 0, 0}, {0, 1, 0},
                               {0, -1, 0}, {0, 0, 1},  {0, 0, -1}};
  for (auto n : normals) {
    auto u = glm::vec3(n.y, n.z, n.x);
    auto v = glm::cross(n, u);
    unsigned base = unsigned(m.vertices.size());
    for (auto q : std::vector<glm::vec2>{{-1, -1}, {1, -1}, {1, 1}, {-1, 1}})
      m.vertices.push_back({c + (n + q.x * u + q.y * v) * s * .5f, n});
    m.triangles.insert(m.triangles.end(),
                       {base, base + 1, base + 2, base, base + 2, base + 3});
    m.edges.insert(m.edges.end(), {base, base + 1, base + 1, base + 2, base + 2,
                                   base + 3, base + 3, base});
  }
}
float displacement(float x, float loadX, float amplitude, const Parameters &p,
                   bool hieu) {
  float left = -p.length / 2, right = p.length / 2;
  if (hieu) {
    if (x < 0)
      right = 0;
    else
      left = 0;
  } else {
    if (x < -p.towerX)
      right = -p.towerX;
    else if (x < p.towerX) {
      left = -p.towerX;
      right = p.towerX;
    } else
      left = p.towerX;
  }
  float t = glm::clamp((x - left) / (right - left), 0.f, 1.f);
  return -amplitude * std::pow(std::sin(glm::pi<float>() * t), 2.f) *
         std::exp(-std::pow((x - loadX) / 45.f, 2.f));
}
void Model::build(bool useHieu) {
  hieu = useHieu;
  parts.clear();
  auto add = [&](std::string n, std::string mat, glm::vec3 c,
                 std::string role) {
    parts.push_back({int(parts.size()), n, mat, role, c, {}});
  };
  add("Tower / pylons", "Painted steel / concrete",
      hieu ? glm::vec3(.84, .76, .61) : glm::vec3(.78, .12, .055),
      "Carries cable anchor nodes");
  add("Deck structure", "Steel", {.53, .24, .12},
      "Longitudinal and transverse load path");
  add("Main cables", "Steel", {.78, .17, .07},
      "Parabolic suspension cable approximation");
  add(hieu ? "Stay cables" : "Vertical hangers", "Steel", {.54, .61, .65},
      "Connects tower or cable to deck nodes");
  add("Road and walkways", "Asphalt", {.20, .24, .27}, "Traffic surface");
  add("Railings", "Steel", {.66, .72, .74}, "Edge barriers");
  add("Foundations and anchors", "Concrete", {.58, .55, .48},
      "Illustrative supports and abutments");
  auto &t = parts[0].mesh;
  auto &d = parts[1].mesh;
  auto &c = parts[2].mesh;
  auto &h = parts[3].mesh;
  float L = p.length / 2, W = p.width / 2, H = p.deckHeight, T = p.towerHeight;
  // Segment deck surfaces so the shared deformation function bends all
  // representations.
  for (int i = 0; i < p.trussPanels; ++i) {
    float x = -L + p.length * i / p.trussPanels, dx = p.length / p.trussPanels;
    box(parts[4].mesh, {x + dx / 2, H + .3f, 0}, {dx, .6f, p.width});
    box(d, {x + dx / 2, H - .5f, 0}, {dx, hieu ? 2.f : .35f, p.width * .9f});
    for (float z : {-W, W}) {
      beam(d, {x, H - 3, z}, {x + dx, H - 3, z}, p.barRadius);
      if (!hieu) {
        beam(d, {x, H, z}, {x + dx, H - 3, z}, p.barRadius);
        beam(d, {x, H - 3, z}, {x + dx, H, z}, p.barRadius);
      }
      beam(parts[5].mesh, {x, H + .6f, z}, {x, H + 1.8f, z}, .08f);
      for (float y : {H + 1.2f, H + 1.8f})
        beam(parts[5].mesh, {x, y, z}, {x + dx, y, z}, .07f);
    }
    beam(d, {x, H - 3, -W}, {x, H - 3, W}, p.barRadius);
    if (!hieu)
      beam(d, {x, H - 3, -W}, {x + dx, H - 3, W}, p.barRadius);
  }
  if (!hieu) {
    for (float x : {-p.towerX, p.towerX}) {
      for (float z : {-W, W})
        box(t, {x, T / 2 + 2, z}, {2.4f, T - 4, 2.4f});
      for (float y : {H - 1, T * .55f, T * .78f, T - 2})
        box(t, {x, y, 0}, {2.5f, 2.2f, p.width});
      box(parts[6].mesh, {x, 2, 0}, {8, 4, p.width + 7});
    }
    auto cableY = [&](float x) {
      return std::abs(x) <= p.towerX
                 ? T - p.cableSag * (1 - x * x / (p.towerX * p.towerX))
                 : glm::mix(T, H + 1,
                            (std::abs(x) - p.towerX) / (L - p.towerX));
    };
    std::vector<float> nodes{-p.towerX, p.towerX};
    for (int i = 0; i <= p.cableSegments; ++i)
      nodes.push_back(-L + p.length * i / p.cableSegments);
    for (int i = 1; i < p.hangers; ++i)
      nodes.push_back(-L + p.length * i / p.hangers);
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    for (float z : {-W, W}) {
      for (size_t i = 1; i < nodes.size(); ++i) {
        float a = nodes[i - 1], b = nodes[i];
        beam(c, {a, cableY(a), z}, {b, cableY(b), z}, p.cableRadius, 8);
      }
      for (int i = 1; i < p.hangers; ++i) {
        float x = -L + p.length * i / p.hangers;
        beam(h, {x, H, z}, {x, cableY(x), z}, p.cableRadius * .6f, 6);
      }
    }
  } else {
    // Two curved volumetric legs leave actual open space between them.
    for (float side : {-1.f, 1.f})
      for (int i = 0; i < 24; ++i) {
        auto node = [&](int k) {
          float u = k / 24.f;
          return glm::vec3(side * (W * 1.1f * (1 - u) + .35f +
                                   2 * std::sin(u * glm::pi<float>())),
                           4 + u * (T - 4), 0);
        };
        beam(t, node(i), node(i + 1), 1.35f, 4);
      }

    box(parts[6].mesh, {0, 2, 0}, {13, 4, p.width + 10});
    for (float side : {-1.f, 1.f})
      for (int i = 1; i <= p.stayCount; ++i)
        for (float z : {-W, W}) {
          float x = side * (9 + (L - 14) * i / p.stayCount),
                y = T - 3 - 12.f * (1 - float(i) / p.stayCount);
          float u = (y - 4) / (T - 4);
          beam(h,
               {side * (W * 1.1f * (1 - u) + .35f +
                        2 * std::sin(u * glm::pi<float>())),
                y, (z / W) * .65f},
               {x, H, z}, p.cableRadius * .6f, 6, 0.f, 1.f);
        }
    for (float x : {-L + 12, -L + 25, -L + 38, L - 12, L - 25}) {
      box(parts[6].mesh, {x, H / 2, 0}, {2, H, p.width * .65f});
      box(parts[6].mesh, {x, 1, 0}, {6, 2, p.width});
    }
  }
  for (float x : {-L, L})
    box(parts[6].mesh, {x, H / 2, 0}, {7, H, p.width + 4});
  if (hieu) {
    add("Terracotta crossheads", "Painted concrete", {.78f, .30f, .17f},
        "Illustrative chevron crossheads");
    for (float y : {T * .56f, T * .8f})
      for (float side : {-1.f, 1.f}) {
        float u = (y - 5) / (T - 4);
        beam(parts.back().mesh, {0, y + 2, 0},
             {side * (W * 1.1f * (1 - u) + .35f +
                      2 * std::sin(u * glm::pi<float>())),
              y - 1, 0},
             1.f, 4);
      }
  }
  for (auto &part : parts)
    for (auto &v : part.mesh.vertices) {
      part.min = glm::min(part.min, v.position);
      part.max = glm::max(part.max, v.position);
    }
}
