#include "model.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
void require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
int main() {
  try {
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
      for (float x :
           {-model.p.length / 2, model.p.length / 2,
            hieu ? 0.f : -model.p.towerX, hieu ? 0.f : model.p.towerX})
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
      require(displacement(30, 30, 5, model.p, hieu) < 0,
              "No illustrative deck displacement");
      std::cout << (hieu ? "Hieu" : "Golden Gate")
                << ": mesh bounds, indices, normals, cable system and fixed "
                   "supports passed; "
                << count << " vertices\n";
    }
  } catch (const std::exception &e) {
    std::cerr << e.what();
    return 1;
  }
}
