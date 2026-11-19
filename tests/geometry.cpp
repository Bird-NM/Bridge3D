#include "model.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
#include "clearance.hpp"
int main() {
  try {
    require(triangleBox({-2, 0, -2}, {2, 0, -2}, {0, 0, 2}, {-.1f, -.1f, -.1f},
                        {.1f, .1f, .1f}),
            "Triangle/volume interior intersection missed");
    require(!triangleBox({-2, 3, -2}, {2, 3, -2}, {0, 3, 2}, {-.1f, -.1f, -.1f},
                         {.1f, .1f, .1f}),
            "Separated triangle reported inside volume");
    Model model;
    model.p.load(std::string(BRIDGE_ROOT) + "/config/bridges.cfg");
    for (bool hieu : {false, true}) {
      model.build(hieu);
      size_t count = 0;
      for (auto &part : model.parts) {
        require(part.mesh.triangles.size() % 3 == 0, "Triangle index count");
        require(part.mesh.edges.size() % 2 == 0, "Edge index count");
        for (auto index : part.mesh.triangles)
          require(index < part.mesh.vertices.size(), "Invalid triangle index");
        for (size_t i = 0; i < part.mesh.triangles.size(); i += 3) {
          auto a = part.mesh.vertices[part.mesh.triangles[i]],
               b = part.mesh.vertices[part.mesh.triangles[i + 1]],
               c = part.mesh.vertices[part.mesh.triangles[i + 2]];
          auto face =
              glm::cross(b.position - a.position, c.position - a.position);
          require(glm::length(face) > 1e-8f, "Degenerate bridge triangle");
          require(glm::dot(face, a.normal + b.normal + c.normal) > 0,
                  "Bridge winding/normal mismatch");
        }
        for (auto index : part.mesh.edges)
          require(index < part.mesh.vertices.size(), "Invalid edge index");
        for (auto v : part.mesh.vertices) {
          require(std::isfinite(glm::length(v.position)), "Nonfinite vertex");
          require(std::abs(glm::length(v.normal) - 1) < .001f,
                  "Nonunit normal");
        }
        count += part.mesh.vertices.size();
      }
      require(count > 1000, "Missing geometry");
      require(model.parts[3].mesh.vertices.size() > 0, "Missing hangers/stays");
      require(hieu == model.parts[2].mesh.vertices.empty(),
              "Bridge cable systems mixed");
      for (float x : {-model.p.length / 2, model.p.length / 2,
                      hieu ? model.p.hieuTowerX : -model.p.towerX,
                      hieu ? model.p.hieuTowerX : model.p.towerX})
        require(std::abs(displacement(x, 30, 5, model.p, hieu)) < .0001f,
                "Support moved");
      if (hieu) {
        size_t fixed = 0;
        for (auto v : model.parts[3].mesh.vertices)
          if (v.mobility == 0)
            ++fixed;
        require(fixed * 2 == model.parts[3].mesh.vertices.size(),
                "Stay anchor endpoints are not fixed");
      }
      require(
          model.cableAttachments.size() ==
              size_t(hieu ? 4 * model.p.stayCount : 2 * (model.p.hangers - 1)),
          "Missing cable attachment metadata");
      for (auto attachment : model.cableAttachments) {
        require(std::abs(attachment.lower.y - model.p.deckHeight) < 1e-5f,
                "Cable lower node left deck elevation");
        require(std::abs(std::abs(attachment.lower.z) -
                         (model.p.width / 2 + (hieu ? .4f : 1.6f))) < 1e-5f,
                "Cable misses longitudinal edge girder");
        if (hieu)
          require(glm::length(attachment.upper -
                              hieuLegCenter(
                                  model.p, attachment.upper.y,
                                  attachment.upper.z < 0 ? -1.f : 1.f)) < 1e-5f,
                  "Stay misses tower node");
        else
          require(attachment.upper.x == attachment.lower.x &&
                      attachment.upper.z == attachment.lower.z,
                  "Hanger not vertical");
      }
      EnvironmentModel environment;
      environment.build(model, std::string(BRIDGE_ROOT) +
                                   (hieu ? "/config/hieu-environment.cfg"
                                         : "/config/golden-environment.cfg"));
      checkEnvironment(model, environment);
      if (hieu)
        checkClearance(model);
      require(displacement(30, 30, 5, model.p, hieu) < 0,
              "No illustrative deck displacement");
      std::cout << (hieu ? "Hieu" : "Golden Gate")
                << ": mesh bounds, indices, normals, cable system and fixed "
                   "supports passed; "
                << count << " vertices\n";
    }
    model.p.carriagewayWidth = 10;
    model.p.sidewalkWidth = 2;
    model.p.width = 14;
    model.p.vehicleClearanceHeight = 7;
    model.p.hieuTowerX = 12;
    model.build(true);
    checkClearance(model);
  } catch (const std::exception &e) {
    std::cerr << e.what();
    return 1;
  }
}
