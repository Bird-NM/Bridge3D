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
  bool hasRoadWidths = false;
  std::string line;
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    auto split = line.find('=');
    if (split == std::string::npos)
      continue;
    auto key = line.substr(0, split);
    float v = std::stof(line.substr(split + 1));
    if (!std::isfinite(v))
      throw std::runtime_error("Nonfinite bridge parameter");
    if (key == "sidewalkRaise")
      sidewalkRaise = v;
    else if (key == "carriagewayWidth") {
      carriagewayWidth = v;
      hasRoadWidths = true;
    } else if (key == "sidewalkWidth") {
      sidewalkWidth = v;
      hasRoadWidths = true;
    } else if (key == "towerLegWidth")
      towerLegWidth = v;
    else if (key == "towerLegDepth")
      towerLegDepth = v;
    else if (key == "lateralClearance")
      lateralClearance = v;
    else if (key == "vehicleClearanceHeight")
      vehicleClearanceHeight = v;
    else if (key == "hieuTowerX")
      hieuTowerX = v;
    else if (key == "length")
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
  if (hasRoadWidths)
    width = carriagewayWidth + 2 * sidewalkWidth;
  else
    carriagewayWidth = width - 2 * sidewalkWidth;
  if (sidewalkRaise < 0 || sidewalkRaise > .3f || carriagewayWidth <= 0 ||
      sidewalkWidth < 0 || towerLegWidth <= 0 || towerLegDepth <= 0 ||
      lateralClearance < 0 || vehicleClearanceHeight < 4 ||
      towerHeight < deckHeight + vehicleClearanceHeight + 10 ||
      std::abs(hieuTowerX) > length / 2 - 15)
    throw std::runtime_error("Invalid road clearance parameters");
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
    if (x < p.hieuTowerX)
      right = p.hieuTowerX;
    else
      left = p.hieuTowerX;
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
  cableAttachments.clear();
  auto add = [&](std::string n, std::string mat, glm::vec3 c,
                 std::string role) {
    parts.push_back({int(parts.size()), n, mat, role, c, {}});
  };
  add("Tower / pylons", "Painted steel / concrete",
      hieu ? glm::vec3(.84, .76, .61) : glm::vec3(.78, .12, .055),
      "Carries cable anchor nodes");
  add("Deck structure", "Steel / concrete",
      hieu ? glm::vec3(.55f, .55f, .51f) : glm::vec3(.56f, .22f, .13f),
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
  float cablePlane = W + (hieu ? .4f : 1.6f);
  // Segment deck surfaces so the shared deformation function bends all
  // representations.
  for (int i = 0; i < p.trussPanels; ++i) {
    float x = -L + p.length * i / p.trussPanels, dx = p.length / p.trussPanels;
    auto &road = parts[4].mesh;
    quad(road, {x, H + .6f, -W}, {x, H + .6f, W}, {x + dx, H + .6f, W},
         {x + dx, H + .6f, -W});
    quad(road, {x, H, -W}, {x + dx, H, -W}, {x + dx, H, W}, {x, H, W});
    for (float side : {-1.f, 1.f}) {
      float z = side * W;
      if (side < 0)
        quad(road, {x, H, z}, {x, H + .6f, z}, {x + dx, H + .6f, z},
             {x + dx, H, z});
      else
        quad(road, {x, H, z}, {x + dx, H, z}, {x + dx, H + .6f, z},
             {x, H + .6f, z});
      auto offset = road.vertices.size();
      if (p.sidewalkWidth > 0 && p.sidewalkRaise > 0)
        box(road,
            {x + dx / 2, H + .6f + p.sidewalkRaise / 2,
             side * (p.carriagewayWidth / 2 + p.sidewalkWidth / 2)},
            {dx, p.sidewalkRaise, p.sidewalkWidth});
      for (size_t v = offset; v < road.vertices.size(); ++v)
        road.vertices[v].tint = {2.3f, 2.1f, 1.8f};
    }
    if (i == 0)
      quad(road, {x, H, -W}, {x, H, W}, {x, H + .6f, W}, {x, H + .6f, -W});
    if (i == p.trussPanels - 1)
      quad(road, {x + dx, H, -W}, {x + dx, H + .6f, -W}, {x + dx, H + .6f, W},
           {x + dx, H, W});
    if (i % 4 < 2) {
      auto offset = road.vertices.size();
      quad(road, {x, H + .618f, -.055f}, {x, H + .618f, .055f},
           {x + dx, H + .618f, .055f}, {x + dx, H + .618f, -.055f});
      for (size_t v = offset; v < road.vertices.size(); ++v)
        road.vertices[v].tint = {4.f, 3.5f, 3.f};
    }
    box(d, {x + dx / 2, H - .5f, 0}, {dx, hieu ? 2.f : .35f, p.width * .9f});
    for (float z : {-cablePlane, cablePlane}) {
      if (hieu)
        box(d, {x + dx / 2, H - .5f, z}, {dx, 1, .7f});
      else {
        beam(d, {x, H, z}, {x + dx, H, z}, p.barRadius);
        beam(d, {x, H - 3, z}, {x + dx, H - 3, z}, p.barRadius);
        beam(d, {x, H, z}, {x + dx, H - 3, z}, p.barRadius);
        beam(d, {x, H - 3, z}, {x + dx, H, z}, p.barRadius);
      }
      float railZ = std::copysign(W, z);
      beam(parts[5].mesh, {x, H + .6f + p.sidewalkRaise, railZ},
           {x, H + 1.8f + p.sidewalkRaise, railZ}, .08f);
      for (float y : {H + 1.2f + p.sidewalkRaise, H + 1.8f + p.sidewalkRaise})
        beam(parts[5].mesh, {x, y, railZ}, {x + dx, y, railZ}, .07f);
    }
    if (!hieu)
      beam(d, {x, H - 3, -cablePlane}, {x, H - 3, cablePlane}, p.barRadius);
    beam(d, {x, H - .25f, -cablePlane}, {x, H - .25f, cablePlane}, p.barRadius);
    if (!hieu)
      beam(d, {x, H - 3, -cablePlane}, {x + dx, H - 3, cablePlane},
           p.barRadius);
  }
  if (!hieu) {
    for (float x : {-p.towerX, p.towerX}) {
      for (float z : {-cablePlane, cablePlane})
        box(t, {x, T / 2 + 2, z}, {2.4f, T - 4, 2.4f});
      for (float y : {H - 1, T * .55f, T * .78f, T - 2})
        box(t, {x, y, 0}, {2.5f, 2.2f, 2 * cablePlane});
      box(parts[6].mesh, {x, 2, 0}, {8, 4, 2 * cablePlane + 5});
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
    for (float z : {-cablePlane, cablePlane}) {
      for (size_t i = 1; i < nodes.size(); ++i) {
        float a = nodes[i - 1], b = nodes[i];
        beam(c, {a, cableY(a), z}, {b, cableY(b), z}, p.cableRadius, 16);
      }
      for (int i = 1; i < p.hangers; ++i) {
        float x = -L + p.length * i / p.hangers;
        CableAttachment attachment{{x, cableY(x), z}, {x, H, z}};
        cableAttachments.push_back(attachment);
        beam(h, attachment.lower, attachment.upper, p.cableRadius * .42f, 12);
      }
    }
    // Separate lateral anchor blocks leave the road and sidewalks unobstructed.
    for (float x : {-L, L})
      for (float z : {-cablePlane, cablePlane})
        box(parts[6].mesh, {x, H - 1, z}, {6, 5, 2.4f});
  } else {
    // Rectangular sections remain aligned with X/Z, making clearance
    // measurable.
    const int legSegments = 48;
    for (float side : {-1.f, 1.f}) {
      std::vector<float> levels{4.f, H + .6f,
                                H + .6f + p.vehicleClearanceHeight + 2.f};
      for (int i = 0; i <= legSegments; ++i)
        levels.push_back(4 + (T - 4) * i / legSegments);
      std::sort(levels.begin(), levels.end());
      levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
      auto ring = [&](float y) {
        auto center = hieuLegCenter(p, y, side);
        std::vector<glm::vec3> vertices;
        for (auto offset :
             std::vector<glm::vec2>{{-1, -1}, {1, -1}, {1, 1}, {-1, 1}})
          vertices.push_back(center +
                             glm::vec3(offset.x * p.towerLegDepth / 2, 0,
                                       offset.y * p.towerLegWidth / 2));
        return vertices;
      };
      for (size_t i = 1; i < levels.size(); ++i) {
        auto a = ring(levels[i - 1]), b = ring(levels[i]);
        for (int j = 0; j < 4; ++j)
          quad(t, a[j], b[j], b[(j + 1) % 4], a[(j + 1) % 4]);
      }
      auto bottom = ring(4), top = ring(T);
      quad(t, bottom[0], bottom[1], bottom[2], bottom[3]);
      quad(t, top[3], top[2], top[1], top[0]);
    }
    float baseHalf = hieuLegCenter(p, 4, 1).z + p.towerLegWidth / 2;
    box(parts[6].mesh, {p.hieuTowerX, 2, 0},
        {p.towerLegDepth + 5, 4, 2 * baseHalf + 3});
    box(parts[6].mesh, {p.hieuTowerX, H - 2.5f, 0},
        {p.towerLegDepth + 1, 2, 2 * baseHalf});
    for (float direction : {-1.f, 1.f})
      for (int i = 1; i <= p.stayCount; ++i)
        for (float side : {-1.f, 1.f}) {
          float end = direction < 0 ? -L : L;
          float x = glm::mix(p.hieuTowerX, end - .8f * direction,
                             float(i) / (p.stayCount + 1));
          float y = T - 3 - 12.f * (1 - float(i) / p.stayCount);
          auto anchor = hieuLegCenter(p, y, side);
          CableAttachment attachment{anchor, {x, H, side * (W + .4f)}};
          cableAttachments.push_back(attachment);
          beam(h, attachment.upper, attachment.lower, p.cableRadius * .42f, 12,
               0.f, 1.f);
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
        float baseY = std::max(y - 1, H + .6f + p.vehicleClearanceHeight + 3.f);
        beam(parts.back().mesh, {p.hieuTowerX, baseY + 3, 0},
             hieuLegCenter(p, baseY, side), 1.f, 4);
      }
  }
  for (auto &part : parts)
    updateBounds(part);
}

glm::vec3 hieuLegCenter(const Parameters &p, float y, float side) {
  float clearTop = p.deckHeight + .6f + p.vehicleClearanceHeight + 2.f;
  float u = glm::clamp((y - clearTop) / (p.towerHeight - clearTop), 0.f, 1.f);
  float taper = u * u * (3 - 2 * u);
  float foot = p.width / 2 + p.lateralClearance + p.towerLegWidth / 2;
  float tip = p.towerLegWidth * .3f;
  return {p.hieuTowerX, y, side * glm::mix(foot, tip, taper)};
}
void quad(Mesh &mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
  auto normal = glm::normalize(glm::cross(b - a, c - a));
  unsigned base = unsigned(mesh.vertices.size());
  for (auto point : {a, b, c, d})
    mesh.vertices.push_back({point, normal});
  mesh.triangles.insert(mesh.triangles.end(),
                        {base, base + 1, base + 2, base, base + 2, base + 3});
  mesh.edges.insert(mesh.edges.end(), {base, base + 1, base + 1, base + 2,
                                       base + 2, base + 3, base + 3, base});
}
void updateBounds(Component &part) {
  part.min = glm::vec3(1e9f);
  part.max = glm::vec3(-1e9f);
  for (auto &vertex : part.mesh.vertices) {
    part.min = glm::min(part.min, vertex.position);
    part.max = glm::max(part.max, vertex.position);
  }
}

void triangle(Mesh &mesh, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
  auto normal = glm::normalize(glm::cross(b - a, c - a));
  unsigned base = unsigned(mesh.vertices.size());
  for (auto p : {a, b, c})
    mesh.vertices.push_back({p, normal});
  mesh.triangles.insert(mesh.triangles.end(), {base, base + 1, base + 2});
  mesh.edges.insert(mesh.edges.end(),
                    {base, base + 1, base + 1, base + 2, base + 2, base});
}

void ellipsoid(Mesh &mesh, glm::vec3 center, glm::vec3 radii, int rings,
               int slices) {
  unsigned base = unsigned(mesh.vertices.size());
  auto vertex = [&](glm::vec3 q) {
    mesh.vertices.push_back({center + q * radii, glm::normalize(q / radii)});
  };
  vertex({0, 1, 0});
  for (int r = 1; r < rings; ++r)
    for (int c = 0; c < slices; ++c) {
      float phi = glm::pi<float>() * r / rings,
            theta = glm::two_pi<float>() * c / slices;
      vertex({std::sin(phi) * std::cos(theta), std::cos(phi),
              std::sin(phi) * std::sin(theta)});
    }
  unsigned bottom = unsigned(mesh.vertices.size());
  vertex({0, -1, 0});
  for (int c = 0; c < slices; ++c) {
    unsigned current = base + 1 + c, next = base + 1 + (c + 1) % slices;
    mesh.triangles.insert(mesh.triangles.end(), {base, next, current});
    mesh.edges.insert(mesh.edges.end(), {base, current});
    for (int r = 0; r < rings - 2; ++r) {
      unsigned a = current + r * slices, b = next + r * slices, cc = b + slices,
               d = a + slices;
      mesh.triangles.insert(mesh.triangles.end(), {a, b, cc, a, cc, d});
      mesh.edges.insert(mesh.edges.end(), {a, b, a, d});
    }
    unsigned a = current + (rings - 2) * slices,
             b = next + (rings - 2) * slices;
    mesh.triangles.insert(mesh.triangles.end(), {bottom, a, b});
    mesh.edges.insert(mesh.edges.end(), {a, b, a, bottom});
  }
}
