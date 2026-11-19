#include "environment.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <glm/gtc/constants.hpp>
#include <random>
#include <stdexcept>
void EnvironmentConfig::load(const std::string &path) {
  std::ifstream file(path);
  if (!file)
    throw std::runtime_error("Cannot open environment config: " + path);
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#')
      continue;
    auto split = line.find('=');
    if (split == std::string::npos)
      continue;
    auto key = line.substr(0, split);
    float value = std::stof(line.substr(split + 1));
    if (!std::isfinite(value))
      throw std::runtime_error("Nonfinite environment parameter");
    if (key == "coastVariation")
      coastVariation = value;
    else if (key == "plantingDepth")
      plantingDepth = value;
    else if (key == "buildingStart")
      buildingStart = value;
    else if (key == "buildingEnd")
      buildingEnd = value;
    else if (key == "treeHeight")
      treeHeight = value;
    else if (key == "treeRadius")
      treeRadius = value;
    else if (key == "roadShoulder")
      roadShoulder = value;
    else if (key == "approachLength")
      approachLength = value;
    else if (key == "terrainExtension")
      terrainExtension = value;
    else if (key == "bankHalfWidth")
      bankHalfWidth = value;
    else if (key == "shoreRun")
      shoreRun = value;
    else if (key == "roadCurve")
      roadCurve = value;
    else if (key == "roadDrop")
      roadDrop = value;
    else if (key == "hillHeight")
      hillHeight = value;
    else if (key == "promenadeWidth")
      promenadeWidth = value;
    else if (key == "treeSpacing")
      treeSpacing = value;
    else if (key == "buildingSetback")
      buildingSetback = value;
    else if (key == "resolution")
      resolution = int(value);
    else if (key == "treeCount")
      treeCount = int(value);
    else if (key == "buildingCount")
      buildingCount = int(value);
    else if (key == "seed")
      seed = int(value);
    else if (key == "waterR")
      waterColor.r = value;
    else if (key == "waterG")
      waterColor.g = value;
    else if (key == "waterB")
      waterColor.b = value;
  }
  if (coastVariation < 0 || coastVariation > 40 || plantingDepth < 30 ||
      buildingEnd < buildingStart + 20 || treeHeight < 3 || treeRadius < 1 ||
      roadShoulder < 8)
    throw std::runtime_error("Invalid landscape detail parameters");
  if (approachLength < 30 || terrainExtension < 10 || bankHalfWidth < 30 ||
      shoreRun < 4 || shoreRun > 30 || resolution < 8 || resolution > 128 ||
      treeCount < 0 || treeCount > 300 || buildingCount < 0 ||
      buildingCount > 60 || treeSpacing < 5 || buildingSetback < 20 ||
      roadDrop < 0 || roadDrop > 10 || hillHeight < 0 || promenadeWidth < 1 ||
      std::abs(roadCurve) > bankHalfWidth / 2)
    throw std::runtime_error("Invalid environment parameters");
}
glm::vec3 EnvironmentModel::roadCenter(const Parameters &p, float side,
                                       float distance) const {
  float u = glm::clamp(distance / config.approachLength, 0.f, 1.f);
  // Straight transition at the abutment, followed by a smooth quadratic curve.
  float curve = std::max(0.f, (u - .18f) / .82f);
  float drop = u * u * (3 - 2 * u);
  return {side * (p.length / 2 + distance),
          p.deckHeight + .6f - config.roadDrop * drop,
          side * config.roadCurve * curve * curve};
}
float EnvironmentModel::terrainHeight(const Parameters &p, float side,
                                      float distance, float z) const {
  auto road = roadCenter(p, side, std::max(0.f, distance));
  float roadGap = std::abs(z - road.z);
  float hillBlend =
      glm::smoothstep(p.width / 2 + config.roadShoulder,
                      p.width / 2 + config.roadShoulder + 32, roadGap);
  auto peak = [](float x, float z, float cx, float cz, float spread) {
    return std::exp(-((x - cx) * (x - cx) + (z - cz) * (z - cz)) /
                    (spread * spread));
  };
  float hill = config.hillHeight * hillBlend *
               (.15f + .95f * peak(distance, z, 90, -85, 85) +
                .8f * peak(distance, z, 160, 125, 110) +
                .7f * peak(distance, z, 330, -130, 180));
  if (config.hillHeight > 2) {
    float padX = 1 - glm::smoothstep(10.f, 15.f, std::abs(distance - 27));
    float padZ = 1 - glm::smoothstep(10.f, 15.f,
                                     std::abs(z - (road.z + p.width / 2 + 5)));
    hill *= 1 - padX * padZ;
  }
  if (config.hillHeight <= 2 &&
      std::abs(distance - config.approachLength) < p.carriagewayWidth / 2 + 6)
    hill = 0;
  float extent = config.approachLength + config.terrainExtension;
  float radius = std::sqrt(std::pow(std::max(distance, 0.f) / extent, 2.f) +
                           std::pow(z / config.bankHalfWidth, 2.f));
  float land = 1 - glm::smoothstep(.68f, 1.f, radius);
  float platform = glm::mix(-3.f, road.y - .35f + hill, land);
  float coastalDistance = distance - coastline(p, z);
  if (coastalDistance < 0) {
    float u = glm::clamp((coastalDistance + config.shoreRun) / config.shoreRun,
                         0.f, 1.f);
    return glm::mix(-1.f, platform, u);
  }
  return platform;
}
static void roadStrip(Mesh &mesh, glm::vec3 a, glm::vec3 b, glm::vec3 rightA,
                      glm::vec3 rightB, float lo, float hi, float thickness) {
  if (hi - lo <= 1e-6f)
    return;
  glm::vec3 down{0, -thickness, 0};
  auto al = a + lo * rightA, ar = a + hi * rightA, bl = b + lo * rightB,
       br = b + hi * rightB;
  auto center = (al + ar + bl + br) / 4.f + down * .5f;
  auto face = [&](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
    auto outward = (p0 + p1 + p2 + p3) / 4.f - center;
    if (glm::dot(glm::cross(p1 - p0, p2 - p0), outward) < 0)
      quad(mesh, p3, p2, p1, p0);
    else
      quad(mesh, p0, p1, p2, p3);
  };
  face(al, ar, br, bl);
  face(al + down, bl + down, br + down, ar + down);
  face(al, al + down, bl + down, bl);
  face(ar, br, br + down, ar + down);
  face(al, ar, ar + down, al + down);
  face(bl, bl + down, br + down, br);
}
static void append(Mesh &target, const Mesh &source, glm::vec3 position,
                   float scale = 1, float angle = 0) {
  unsigned offset = unsigned(target.vertices.size());
  for (auto vertex : source.vertices) {
    auto rotate = [&](glm::vec3 p) {
      return glm::vec3(std::cos(angle) * p.x - std::sin(angle) * p.z, p.y,
                       std::sin(angle) * p.x + std::cos(angle) * p.z);
    };
    vertex.position = position + scale * rotate(vertex.position);
    vertex.normal = rotate(vertex.normal);
    target.vertices.push_back(vertex);
  }
  for (auto i : source.triangles)
    target.triangles.push_back(offset + i);
  for (auto i : source.edges)
    target.edges.push_back(offset + i);
}
void EnvironmentModel::build(const Model &bridge,
                             const std::string &configPath) {
  config = EnvironmentConfig{};
  config.load(configPath);
  parts.clear();
  connections.clear();
  auto add = [&](const char *name, const char *material, glm::vec3 color) {
    parts.push_back({100 + int(parts.size()),
                     name,
                     material,
                     "Assumed educational landscape",
                     color,
                     {}});
  };
  add("Bank A", "Grass", {.39f, .43f, .29f});
  add("Bank B", "Grass", {.40f, .44f, .31f});
  add("Approach roads", "Asphalt", {.20f, .23f, .25f});
  add("Walkways / lookout", "Concrete", {.60f, .58f, .49f});
  add("Shore / embankment", "Rock / concrete",
      bridge.hieu ? glm::vec3(.52f, .53f, .49f) : glm::vec3(.46f, .43f, .37f));
  add("Vegetation", "Foliage", {.28f, .36f, .21f});
  add("Buildings", "Plaster", {.78f, .73f, .60f});
  add("Water", "Simple water", config.waterColor);
  add("Road markings", "Paint", {.88f, .83f, .59f});
  add("Landscape barriers", "Steel", {.60f, .65f, .64f});
  add("Roofs", "Terracotta", {.48f, .23f, .16f});
  add("Tree trunks", "Bark", {.31f, .25f, .17f});
  add("Windows", "Glass", {.27f, .38f, .43f});
  const auto &p = bridge.p;
  foregroundMinimum = {-p.length / 2 - config.approachLength - 20, -1, -90};
  foregroundMaximum = {
      p.length / 2 + config.approachLength + 20,
      std::max(p.deckHeight + config.hillHeight, p.towerHeight), 90};
  int n = config.resolution;
  float extent = config.approachLength + config.terrainExtension;
  // Three reusable crown arrangements, plus a common branching trunk mesh.
  Mesh trunks;
  beam(trunks, {0, 0, 0}, {0, config.treeHeight * .73f, 0}, .18f, 10);
  for (int branch = 0; branch < 6; ++branch) {
    float angle = glm::two_pi<float>() * branch / 6;
    beam(trunks, {0, config.treeHeight * .32f, 0},
         {std::cos(angle) * config.treeRadius * .6f,
          config.treeHeight * (.58f + .08f * (branch % 3)),
          std::sin(angle) * config.treeRadius * .6f},
         .075f, 8);
  }
  std::vector<Mesh> trees(3);
  for (int variant = 0; variant < 3; ++variant) {
    for (int crown = 0; crown < 9; ++crown) {
      float angle = glm::two_pi<float>() * (crown % 6) / 6;
      float radius =
          crown < 6 ? config.treeRadius * .6f : config.treeRadius * .22f;
      glm::vec3 center{std::cos(angle) * radius,
                       config.treeHeight *
                           (crown < 6 ? .58f + .08f * (crown % 3) : .80f),
                       std::sin(angle) * radius};
      glm::vec3 size{config.treeRadius * (.5f + variant * .035f),
                     config.treeHeight * .18f,
                     config.treeRadius * (.48f + variant * .025f)};
      unsigned offset = unsigned(trees[variant].vertices.size());
      ellipsoid(trees[variant], center, size);
      for (size_t k = offset; k < trees[variant].vertices.size(); ++k)
        trees[variant].vertices[k].tint =
            glm::vec3(.93f + variant * .035f, 1.f, .94f);
    }
  }
  std::mt19937 random(config.seed);
  std::uniform_real_distribution<float> unit(0, 1);
  for (int bank = 0; bank < 2; ++bank) {
    float side = bank == 0 ? -1.f : 1.f;
    auto start = roadCenter(p, side, 0);
    connections.push_back(
        {start, {side, 0, 0}, p.carriagewayWidth, p.sidewalkWidth});
    // Concentrate grid resolution around the bridge connections and curved
    // roads.
    std::vector<float> zGrid;
    float localWidth = std::min(config.bankHalfWidth, 90.f);
    if (config.bankHalfWidth > localWidth)
      for (int j = 0; j < n / 2; ++j)
        zGrid.push_back(
            glm::mix(-config.bankHalfWidth, -localWidth, float(j) / (n / 2)));
    for (int j = 0; j <= n; ++j)
      zGrid.push_back(glm::mix(-localWidth, localWidth, float(j) / n));
    if (config.bankHalfWidth > localWidth)
      for (int j = 1; j <= n / 2; ++j)
        zGrid.push_back(
            glm::mix(localWidth, config.bankHalfWidth, float(j) / (n / 2)));
    for (int section = 0; section < 2; ++section) {
      std::vector<float> rows;
      if (section == 0) {
        for (int i = 0; i <= n / 4; ++i)
          rows.push_back(glm::mix(-config.shoreRun, 0.f, float(i) / (n / 4)));
      } else {
        float closeExtent = std::min(extent, config.approachLength + 30);
        for (int i = 0; i <= n; ++i)
          rows.push_back(closeExtent * i / n);
        if (extent > closeExtent)
          for (int i = 1; i <= n / 2; ++i)
            rows.push_back(glm::mix(closeExtent, extent, float(i) / (n / 2)));
      }
      for (size_t i = 1; i < rows.size(); ++i)
        for (size_t j = 1; j < zGrid.size(); ++j) {
          auto point = [&](float d, float z) {
            float shifted =
                d + coastline(p, z) * std::exp(-std::max(d, 0.f) / 100.f);
            return glm::vec3(side * (p.length / 2 + shifted),
                             terrainHeight(p, side, shifted, z), z);
          };
          auto &mesh = section == 0 ? parts[4].mesh : parts[bank].mesh;
          auto a = point(rows[i - 1], zGrid[j - 1]),
               b = point(rows[i], zGrid[j - 1]), c = point(rows[i], zGrid[j]),
               d = point(rows[i - 1], zGrid[j]);
          if (side > 0)
            quad(mesh, a, d, c, b);
          else
            quad(mesh, a, b, c, d);
        }
    }
    auto right = [&](float d) {
      float u = glm::clamp(d / config.approachLength, 0.f, 1.f);
      float curve = std::max(0.f, (u - .18f) / .82f);
      float dz =
          side * config.roadCurve * 2 * curve / (.82f * config.approachLength);
      return glm::normalize(glm::vec3(-dz, 0, side));
    };
    int segments = n * 2;
    for (int i = 0; i < segments; ++i) {
      float da = config.approachLength * i / segments,
            db = config.approachLength * (i + 1) / segments;
      auto a = roadCenter(p, side, da), b = roadCenter(p, side, db),
           ra = right(da), rb = right(db);
      roadStrip(parts[2].mesh, a, b, ra, rb, -p.carriagewayWidth / 2,
                p.carriagewayWidth / 2, .5f);
      for (float sign : {-1.f, 1.f}) {
        float lo = p.carriagewayWidth / 2, hi = p.width / 2;
        roadStrip(parts[3].mesh, a + glm::vec3(0, p.sidewalkRaise, 0),
                  b + glm::vec3(0, p.sidewalkRaise, 0), ra * sign, rb * sign,
                  lo, hi, .5f + p.sidewalkRaise);
        auto va = a + ra * sign * hi + glm::vec3(0, p.sidewalkRaise, 0),
             vb = b + rb * sign * hi + glm::vec3(0, p.sidewalkRaise, 0);
        if (!bridge.hieu || da < config.approachLength - 9) {
          beam(parts[9].mesh, va + glm::vec3(0, 1.1f, 0),
               vb + glm::vec3(0, 1.1f, 0), .065f);
          beam(parts[9].mesh, va, va + glm::vec3(0, 1.1f, 0), .07f);
        }
      }
      if (i % 4 < 2)
        roadStrip(parts[8].mesh, a + glm::vec3(0, .018f, 0),
                  b + glm::vec3(0, .018f, 0), ra, rb, -.06f, .06f, .01f);
    }
    if (!bridge.hieu)
      for (int rock = 0; rock < 14; ++rock) {
        float z = (rock % 2 ? -1.f : 1.f) * (22 + unit(random) * 75);
        float d =
            coastline(p, z) - config.shoreRun * (.35f + unit(random) * .55f);
        float y = terrainHeight(p, side, d, z) - 1.1f;
        ellipsoid(parts[4].mesh, {side * (p.length / 2 + d), y, z},
                  {1.1f + unit(random) * 1.3f, 1.5f + unit(random) * 1.4f,
                   1.1f + unit(random) * 1.4f},
                  6, 10);
      }
    if (bridge.hieu) {
      // Promenade stops on each side of the approach instead of crossing its
      // abutment.
      for (float zSide : {-1.f, 1.f})
        for (int j = 0; j < 16; ++j) {
          float z0 = zSide *
                     (p.width / 2 + 7 +
                      (std::min(config.bankHalfWidth, 90.f) - p.width / 2 - 7) *
                          j / 16);
          float z1 = zSide *
                     (p.width / 2 + 7 +
                      (std::min(config.bankHalfWidth, 90.f) - p.width / 2 - 7) *
                          (j + 1) / 16);
          auto path = [&](float z) {
            float d = coastline(p, z) + 2;
            float y =
                std::max(terrainHeight(p, side, d, z),
                         terrainHeight(p, side, d + config.promenadeWidth, z)) +
                .2f;
            return glm::vec3(side * (p.length / 2 + d), y, z);
          };
          auto a = path(z0), b = path(z1);
          glm::vec3 lateral{side, 0, 0};
          roadStrip(parts[3].mesh, a, b, lateral, lateral, 0,
                    config.promenadeWidth, .3f);
          beam(parts[9].mesh, a + glm::vec3(0, 1.1f, 0),
               b + glm::vec3(0, 1.1f, 0), .07f);
          beam(parts[9].mesh, a, a + glm::vec3(0, 1.1f, 0), .08f);
        }
      // Cross street joins the curved approach at its end, at the same
      // elevation.
      auto end = roadCenter(p, side, config.approachLength);
      box(parts[2].mesh, {end.x, end.y - .25f, 0},
          {p.carriagewayWidth, .5f, 2 * std::min(config.bankHalfWidth, 90.f)});
    } else {
      float distance = 27;
      auto road = roadCenter(p, side, distance);
      float z = road.z + p.width / 2 + 5;
      // Flat connector and a small lookout, supported on a locally flattened
      // pad.
      float y = road.y;
      box(parts[3].mesh, {road.x, y + p.sidewalkRaise - .2f, z}, {10, .4f, 10});
      box(parts[3].mesh,
          {road.x, y + p.sidewalkRaise - .2f, road.z + p.width / 2 + 1.5f},
          {4, .4f, 3});
      for (float sign : {-1.f, 1.f})
        beam(parts[9].mesh, {road.x + 5 * sign, y + 1, z - 4},
             {road.x + 5 * sign, y + 1, z + 5}, .08f);
      for (float dx : {-5.f, 5.f})
        for (float dz : {-4.f, 5.f})
          beam(parts[9].mesh, {road.x + dx, y, z + dz},
               {road.x + dx, y + 1, z + dz}, .08f);
      beam(parts[9].mesh, {road.x - 5, y + 1, z + 5},
           {road.x + 5, y + 1, z + 5}, .08f);
    }
    std::vector<glm::vec3> occupied;
    for (int attempt = 0, placed = 0;
         attempt < config.treeCount * 80 && placed < config.treeCount;
         ++attempt) {
      int cluster = attempt % 4;
      float distance = 25 + config.plantingDepth * (cluster / 2) * .55f +
                       (unit(random) + unit(random) - 1) * 22;
      distance = glm::clamp(distance, 12.f, config.plantingDepth);
      float z = (cluster % 2 ? -1.f : 1.f) * (34 + cluster * 9) +
                (unit(random) + unit(random) - 1) * 19;
      auto road = roadCenter(p, side, distance);
      if (std::abs(z - road.z) < p.width / 2 + 17 ||
          (distance > config.approachLength - 8 && bridge.hieu))
        continue;
      bool nearHouse = false;
      for (int b = 0; b < config.buildingCount; ++b) {
        float houseDistance = config.buildingStart +
                              (config.buildingEnd - config.buildingStart) *
                                  (b + .5f) / std::max(1, config.buildingCount);
        float houseZ = (b % 2 ? -1.f : 1.f) * config.buildingSetback;
        if (std::abs(distance - houseDistance) < 10 && std::abs(z - houseZ) < 9)
          nearHouse = true;
      }
      if (nearHouse)
        continue;
      glm::vec3 pos{side * (p.length / 2 + distance),
                    terrainHeight(p, side, distance, z), z};
      bool clear =
          std::all_of(occupied.begin(), occupied.end(), [&](glm::vec3 other) {
            return glm::length(pos - other) > config.treeSpacing;
          });
      if (!clear)
        continue;
      occupied.push_back(pos);
      float scale = .85f + unit(random) * .3f;
      float angle = unit(random) * glm::two_pi<float>();
      append(parts[5].mesh, trees[placed % 3], pos, scale, angle);
      append(parts[11].mesh, trunks, pos, scale, angle);
      ++placed;
    }
    for (int i = 0; i < config.buildingCount; ++i) {
      float distance = config.buildingStart +
                       (config.buildingEnd - config.buildingStart) * (i + .5f) /
                           std::max(1, config.buildingCount);
      auto road = roadCenter(p, side, distance);
      float z = (i % 2 ? -1.f : 1.f) * config.buildingSetback;
      if (std::abs(z - road.z) < p.width / 2 + 15)
        continue;
      if (bridge.hieu && std::abs(distance - config.approachLength) <
                             p.carriagewayWidth / 2 + 8)
        continue;
      float y = terrainHeight(p, side, distance, z),
            height = bridge.hieu ? 5.f + (i % 3) * 2 : 4.f;
      float x = side * (p.length / 2 + distance);
      float width = 8 + (i % 3) * 2, depth = 7 + (i % 2) * 2;
      box(parts[6].mesh, {x, y + height / 2, z}, {width, height, depth});
      box(parts[6].mesh, {x, y + .2f, z}, {width + .4f, .4f, depth + .4f});
      float hw = width / 2 + .4f, hd = depth / 2 + .4f, roofY = y + height;
      quad(parts[10].mesh, {x - hw, roofY + 2, z}, {x + hw, roofY + 2, z},
           {x + hw, roofY, z - hd}, {x - hw, roofY, z - hd});
      quad(parts[10].mesh, {x - hw, roofY, z + hd}, {x + hw, roofY, z + hd},
           {x + hw, roofY + 2, z}, {x - hw, roofY + 2, z});
      triangle(parts[10].mesh, {x - hw, roofY, z - hd}, {x - hw, roofY, z + hd},
               {x - hw, roofY + 2, z});
      triangle(parts[10].mesh, {x + hw, roofY, z + hd}, {x + hw, roofY, z - hd},
               {x + hw, roofY + 2, z});
      for (float wall : {-1.f, 1.f})
        for (int floor = 0; floor < int(height / 2.8f); ++floor)
          for (int window = 0; window < 3; ++window) {
            float wx = x + (window - 1) * width * .27f;
            box(parts[12].mesh,
                {wx, y + 1.6f + floor * 2.8f, z + wall * (depth / 2 + .015f)},
                {1.1f, 1.4f, .03f});
          }
    }
  }
  // Water is exactly at Y=0, bounded independently from scene-fit bounds.
  quad(parts[7].mesh, {-1600, 0, -1600}, {-1600, 0, 1600}, {1600, 0, 1600},
       {1600, 0, -1600});
  for (auto &part : parts)
    updateBounds(part);
}
void EnvironmentModel::bounds(glm::vec3 &minimum, glm::vec3 &maximum) const {
  minimum = glm::min(minimum, foregroundMinimum);
  maximum = glm::max(maximum, foregroundMaximum);
  for (auto &part : parts)
    if (part.id != 100 && part.id != 101 && part.id != 104 && part.id != 107 &&
        !part.mesh.vertices.empty()) {
      minimum = glm::min(minimum, part.min);
      maximum = glm::max(maximum, part.max);
    }
}

float EnvironmentModel::coastline(const Parameters &p, float z) const {
  float blend =
      glm::smoothstep(p.width / 2 + 18, p.width / 2 + 75, std::abs(z));
  return config.coastVariation * blend *
         (std::sin(z * .018f) + .35f * std::sin(z * .043f));
}
