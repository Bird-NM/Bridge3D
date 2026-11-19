#include "camera.hpp"
#include "renderer.hpp"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <memory>
#include <stb_image_write.h>
struct View {
  int x, y, w, h;
};
static glm::vec3 transformed(glm::vec3 v, const Component &part, const Model &m,
                             const Renderer &r, float mobility = 1.f) {
  if (part.id > 0 && part.id < 6)
    v.y += mobility * displacement(v.x, r.loadX, r.amplitude, m.p, m.hieu) *
           glm::clamp((m.p.towerHeight - v.y) /
                          (m.p.towerHeight - m.p.deckHeight - 2),
                      0.f, 1.f);
  v.y += r.explode * ((part.id == 0 || part.id == 7) ? 2
                      : part.id == 2 || part.id == 3 ? 4
                      : part.id == 6                 ? 0
                                                     : 1);
  return v;
}
static int pick(glm::vec2 mouse, View view, const Camera &camera,
                const Model &model, const Renderer &renderer) {
  auto inv = glm::inverse(camera.matrix(float(view.w) / view.h));
  glm::vec2 ndc{2 * (mouse.x - view.x) / view.w - 1,
                1 - 2 * (mouse.y - view.y) / view.h};
  auto a = inv * glm::vec4(ndc, -1, 1), b = inv * glm::vec4(ndc, 1, 1);
  glm::vec3 origin = a / a.w, dir = glm::normalize(glm::vec3(b / b.w) - origin);
  float best = std::numeric_limits<float>::max();
  int id = -1;
  for (auto &part : model.parts)
    if (part.visible)
      for (size_t i = 0; i < part.mesh.triangles.size(); i += 3) {
        auto vertex = [&](int j) {
          auto vertex = part.mesh.vertices[part.mesh.triangles[i + j]];
          return transformed(vertex.position, part, model, renderer,
                             vertex.mobility);
        };
        auto v = vertex(0), e1 = vertex(1) - v, e2 = vertex(2) - v,
             p = glm::cross(dir, e2);
        float det = glm::dot(e1, p);
        if (std::abs(det) < 1e-7f)
          continue;
        auto t = origin - v;
        float u = glm::dot(t, p) / det;
        if (u < 0 || u > 1)
          continue;
        auto q = glm::cross(t, e1);
        float w = glm::dot(dir, q) / det;
        if (w < 0 || u + w > 1)
          continue;
        float distance = glm::dot(e2, q) / det;
        if (distance > 0 && distance < best) {
          best = distance;
          id = part.id;
        }
      }
  return id;
}
static bool screenshot(const std::string &file, View v, int framebufferHeight) {
  std::vector<unsigned char> pixels(size_t(v.w) * v.h * 3),
      flipped(pixels.size());
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(v.x, framebufferHeight - v.y - v.h, v.w, v.h, GL_RGB,
               GL_UNSIGNED_BYTE, pixels.data());
  for (int y = 0; y < v.h; ++y)
    std::copy_n(pixels.data() + size_t(y) * v.w * 3, v.w * 3,
                flipped.data() + size_t(v.h - y - 1) * v.w * 3);
  return stbi_write_png(file.c_str(), v.w, v.h, 3, flipped.data(), v.w * 3) !=
         0;
}
int main(int argc, char **argv) {
  try {
    bool smoke = argc > 1 && std::string(argv[1]) == "--smoke";
    if (!glfwInit())
      throw std::runtime_error("GLFW initialization failed");
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    auto window = glfwCreateWindow(
        1600, 960, "Bridge3D | Structural geometry studio", nullptr, nullptr);
    if (!window)
      throw std::runtime_error("OpenGL 3.3 window unavailable");
    glfwMakeContextCurrent(window);
    glfwSwapInterval(smoke ? 0 : 1);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
      throw std::runtime_error("GLAD failed");
    std::cout << "OpenGL " << glGetString(GL_VERSION) << " / "
              << glGetString(GL_RENDERER) << std::endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 0;
    style.FrameRounding = 4;
    style.ItemSpacing = {8, 9};
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    {
      Model model;
      model.p.load(std::string(BRIDGE_ROOT) + "/config/bridges.cfg");
      Renderer renderer;
      Camera camera;
      std::vector<std::unique_ptr<GpuMesh>> meshes;
      auto rebuild = [&](bool hieu) {
        model.build(hieu);
        meshes.clear();
        for (auto &part : model.parts)
          meshes.push_back(std::make_unique<GpuMesh>(part.mesh));
        renderer.selected = -1;
      };
      rebuild(false);
      Component environment{8,  "Environment",   "Backdrop",
                            "", {.15, .20, .22}, {}};
      for (int n = -12; n <= 12; ++n) {
        float x = n * 12.f;
        beam(environment.mesh, {x, -.2f, -90}, {x, -.2f, 90}, .035f);
        beam(environment.mesh, {-144, -.2f, x}, {144, -.2f, x}, .035f);
      }
      GpuMesh gridMesh(environment.mesh);
      Component water{8, "Water", "Backdrop", "", {.10, .25, .32}, {}};
      box(water.mesh, {0, -1, 0}, {300, .4f, 160});
      GpuMesh waterMesh(water.mesh);
      Component truck{9, "Truck", "Illustrative load", "", {.94, .61, .13}, {}};
      box(truck.mesh, {0, 2, 0}, {7, 3, 2.5f});
      box(truck.mesh, {4, 1.5f, 0}, {2.5f, 2.5f, 2.5f});
      for (float x : {-2.f, 3.f})
        for (float z : {-1.4f, 1.4f})
          beam(truck.mesh, {x, .4f, z - .2f}, {x, .4f, z + .2f}, .6f, 10);
      GpuMesh truckMesh(truck.mesh);
      int bridge = 0, mode = 1, viewIndex = 0;
      bool compare = false, grid = false, background = true, truckOn = false,
           running = false, deform = false, ghost = false;
      float speed = 18, load = 50, amplification = 8;
      int frame = 0;
      double last = glfwGetTime();
      float drag = 0;
      std::string feedback = "Ready";
      std::filesystem::create_directories(std::string(BRIDGE_ROOT) +
                                          "/captures");
      double measured = 0;
      int measuredFrames = 0;
      while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double now = glfwGetTime();
        float elapsed = float(now - last);
        float dt = std::min(elapsed, .1f);
        last = now;
        int ww, wh, fw, fh;
        glfwGetWindowSize(window, &ww, &wh);
        glfwGetFramebufferSize(window, &fw, &fh);
        if (ww < 1 || wh < 1 || fw < 1 || fh < 1) {
          glfwWaitEventsTimeout(.05);
          continue;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        float sidebar = 220, rightbar = 265, top = 90, bottom = 34;
        View region{int(sidebar), int(top),
                    std::max(1, ww - int(sidebar + rightbar)),
                    std::max(1, wh - int(top + bottom))};
        auto fit = [&]() {
          camera.fit({0, model.p.towerHeight * .45f, 0}, model.p.length * .51f,
                     float(region.w) / (compare ? 2 : 1) / region.h);
        };
        if (frame == 0) {
          fit();
          camera.smooth = false;
          camera.update(1);
          camera.smooth = true;
        }
        auto panel = [&](const char *name, ImVec2 pos, ImVec2 size) {
          ImGui::SetNextWindowPos(pos);
          ImGui::SetNextWindowSize(size);
          ImGui::Begin(name, nullptr,
                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoTitleBar);
        };
        panel("Toolbar", {0, 0}, {float(ww), top});
        ImGui::TextColored({1, .65f, .3f, 1}, "BRIDGE3D");
        ImGui::SameLine();
        ImGui::TextDisabled("Geometry & structural illustration");
        ImGui::SetNextItemWidth(180);
        if (ImGui::Combo("##bridge", &bridge, "Golden Gate\0Song Hieu\0")) {
          rebuild(bridge == 1);
          fit();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(135);
        ImGui::Combo("##mode", &mode, "Wireframe\0Surface\0Surface + edges\0");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(135);
        if (ImGui::Combo("##view", &viewIndex,
                         "Perspective\0Front\0Side\0Top\0Along road\0"))
          camera.preset(viewIndex);
        ImGui::SameLine();
        if (ImGui::Button("Fit [F]"))
          fit();
        ImGui::SameLine();
        if (ImGui::Button("Reset [R]")) {
          camera.preset(0);
          fit();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Compare", &compare))
          fit();
        ImGui::SameLine();
        bool capture = ImGui::Button("Save PNG");
        ImGui::End();
        panel("Components", {0, top}, {sidebar, float(region.h)});
        ImGui::Text("COMPONENTS");
        ImGui::Separator();
        for (auto &part : model.parts) {
          if (part.mesh.vertices.empty())
            continue;
          ImGui::PushID(part.id);
          ImGui::Checkbox("##visible", &part.visible);
          ImGui::SameLine();
          if (ImGui::Selectable(part.name.c_str(),
                                renderer.selected == part.id))
            renderer.selected = part.id;
          ImGui::PopID();
        }
        if (ImGui::Button("Show all"))
          for (auto &part : model.parts)
            part.visible = true;
        if (renderer.selected >= 0) {
          if (ImGui::Button("Isolate selected"))
            for (auto &part : model.parts)
              part.visible = part.id == renderer.selected;
          if (ImGui::Button("Focus selected")) {
            auto &p = model.parts[renderer.selected];
            camera.fit((p.min + p.max) / 2.f, glm::length(p.max - p.min) / 2,
                       float(region.w) / (compare ? 2 : 1) / region.h);
          }
        }
        ImGui::Separator();
        ImGui::Checkbox("Grid", &grid);
        ImGui::Checkbox("Water backdrop", &background);
        ImGui::Checkbox("Orthographic", &camera.ortho);
        ImGui::Checkbox("Smooth camera", &camera.smooth);
        ImGui::Checkbox("Hide occluded edges", &renderer.hidden);
        ImGui::Checkbox("Triangle edges", &renderer.fullTriangles);
        ImGui::Checkbox("Vertex points", &renderer.points);
        ImGui::SliderFloat("Explode", &renderer.explode, 0, 8);
        ImGui::TextWrapped("Exploded view is a presentation of geometry.");
        ImGui::End();
        panel("Properties", {float(ww) - rightbar, top},
              {rightbar, float(region.h)});
        ImGui::Text("PROPERTIES");
        ImGui::Separator();
        if (renderer.selected >= 0) {
          auto &p = model.parts[renderer.selected];
          ImGui::TextWrapped("%s", p.name.c_str());
          ImGui::TextWrapped("Material: %s", p.material.c_str());
          auto s = p.max - p.min;
          ImGui::Text("Bounds %.1f x %.1f x %.1f", s.x, s.y, s.z);
          ImGui::Text("Vertices %zu", p.mesh.vertices.size());
          ImGui::Text("Edges %zu", p.mesh.edges.size() / 2);
          ImGui::Text("Triangles %zu", p.mesh.triangles.size() / 3);
          ImGui::TextWrapped("%s", p.role.c_str());
        } else
          ImGui::TextWrapped(
              "Click a component in the scene or list to inspect it.");
        ImGui::TextDisabled("All dimensions are assumed.");
        ImGui::Separator();
        ImGui::Text("DEFORMATION ILLUSTRATION");
        ImGui::Checkbox("Truck load", &truckOn);
        ImGui::Checkbox("Animate", &running);
        ImGui::SliderFloat("Speed", &speed, 0, 45);
        ImGui::SliderFloat("Load position", &renderer.loadX,
                           -model.p.length / 2, model.p.length / 2);
        ImGui::SliderFloat("Assumed load", &load, 0, 100);
        ImGui::Checkbox("Deform", &deform);
        ImGui::SliderFloat("Magnification", &amplification, 1, 30);
        ImGui::Checkbox("Original outline", &ghost);
        if (ImGui::Button("Reset simulation")) {
          running = false;
          renderer.loadX = -model.p.length / 2;
          load = 50;
          deform = false;
        }
        ImGui::TextWrapped(
            "Prescribed kinematic deformation. No force equilibrium or safety "
            "assessment. Load is a relative illustration parameter.");
        ImGui::TextWrapped("%s", feedback.c_str());
        ImGui::End();
        bool hovered =
            io.MousePos.x >= region.x && io.MousePos.x < region.x + region.w &&
            io.MousePos.y >= region.y && io.MousePos.y < region.y + region.h;
        View active = region;
        if (compare) {
          active.w = region.w / 2;
          if (io.MousePos.x >= region.x + active.w)
            active.x += active.w;
        }
        if (hovered && !io.WantCaptureMouse) {
          if (ImGui::IsMouseClicked(0))
            drag = 0;
          if (ImGui::IsMouseDown(0)) {
            drag += glm::length(glm::vec2(io.MouseDelta.x, io.MouseDelta.y));
            if (drag > 3)
              camera.orbit(io.MouseDelta.x, io.MouseDelta.y);
          }
          if (ImGui::IsMouseReleased(0) && drag <= 3)
            renderer.selected = pick({io.MousePos.x, io.MousePos.y}, active,
                                     camera, model, renderer);
          if (ImGui::IsMouseDown(2))
            camera.pan(io.MouseDelta.x, io.MouseDelta.y, float(region.h));
          camera.desiredDistance = glm::clamp(
              camera.desiredDistance * std::exp(-io.MouseWheel * .12f), 3.f,
              2000.f);
        }
        if (!io.WantCaptureKeyboard) {
          if (ImGui::IsKeyPressed(ImGuiKey_F))
            fit();
          if (ImGui::IsKeyPressed(ImGuiKey_R)) {
            camera.preset(0);
            fit();
          }
          for (int k = 0; k < 5; ++k)
            if (ImGui::IsKeyPressed(ImGuiKey(ImGuiKey_1 + k)))
              camera.preset(k);
        }
        if (running && truckOn) {
          renderer.loadX += speed * elapsed;
          if (renderer.loadX > model.p.length / 2)
            renderer.loadX = -model.p.length / 2;
        }
        renderer.amplitude =
            deform && truckOn ? load / 100 * amplification * .25f : 0;
        camera.update(dt);
        if (smoke) {
          int scenario = frame / 12;
          if (scenario > 5)
            break;
          bridge = scenario == 2 || scenario == 3;
          bool targetHieu = bridge == 1;
          if (model.hieu != targetHieu)
            rebuild(targetHieu);
          compare = scenario == 4;
          mode = (scenario == 0 || scenario == 2) ? 0 : 1;
          deform = scenario == 5;
          truckOn = deform;
          ghost = deform;
          amplification = deform ? 20.f : 8.f;
          load = deform ? 100.f : 50.f;
          renderer.amplitude = deform ? 5 : 0;
          renderer.loadX = 32;
          camera.smooth = false;
          fit();
          camera.update(1);
        }
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glDisable(GL_BLEND);
        glClearColor(.055f, .073f, .093f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        size_t vertices = 0, triangles = 0;
        for (auto &p : model.parts)
          if (p.visible) {
            vertices += p.mesh.vertices.size();
            triangles += p.mesh.triangles.size() / 3;
          }
        for (int side = 0; side < (compare ? 2 : 1); ++side) {
          View v = region;
          if (compare) {
            v.w = region.w / 2;
            v.x += side * v.w;
          }
          float sx = float(fw) / ww, sy = float(fh) / wh;
          glViewport(int(v.x * sx), int((wh - v.y - v.h) * sy), int(v.w * sx),
                     int(v.h * sy));
          auto vp = camera.matrix(float(v.w) / v.h);
          if (background)
            renderer.draw(waterMesh, water, model, vp, 1);
          if (grid)
            renderer.draw(gridMesh, environment, model, vp, 1);
          if (renderer.hidden && (compare ? side == 0 : mode == 0)) {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            for (size_t i = 0; i < meshes.size(); ++i)
              if (model.parts[i].visible)
                renderer.draw(*meshes[i], model.parts[i], model, vp, 1);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
          }
          for (size_t i = 0; i < meshes.size(); ++i)
            if (model.parts[i].visible)
              renderer.draw(*meshes[i], model.parts[i], model, vp,
                            compare ? (side == 0 ? 0 : 1) : mode);
          if (ghost && renderer.amplitude > 0)
            for (size_t i = 0; i < meshes.size(); ++i)
              if (model.parts[i].visible)
                renderer.draw(*meshes[i], model.parts[i], model, vp, 0, true);
          if (truckOn) {
            float y = model.p.deckHeight + .6f +
                      displacement(renderer.loadX, renderer.loadX,
                                   renderer.amplitude, model.p, model.hieu) +
                      renderer.explode;
            renderer.draw(
                truckMesh, truck, model,
                vp * glm::translate(glm::mat4(1), {renderer.loadX, y, 0}), 1);
          }
          auto *overlay = ImGui::GetForegroundDrawList();
          overlay->AddText({float(v.x + 15), float(v.y + 15)},
                           IM_COL32(225, 230, 235, 255),
                           model.hieu ? "SONG HIEU / cable-stayed"
                                      : "GOLDEN GATE / suspension");
          overlay->AddText({float(v.x + 15), float(v.y + 35)},
                           IM_COL32(170, 190, 205, 255),
                           compare     ? (side == 0 ? "WIREFRAME" : "SURFACE")
                           : mode == 0 ? "WIREFRAME"
                           : mode == 1 ? "SURFACE"
                                       : "SURFACE + EDGES");
          // Project world axes using the view orientation at a small screen
          // origin.
          auto origin = ImVec2(float(v.x + 45), float(v.y + v.h - 45));
          auto rot = glm::mat3(
              glm::lookAt(camera.eye(), camera.target, glm::vec3(0, 1, 0)));
          glm::vec3 axes[] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
          ImU32 colors[] = {IM_COL32(240, 100, 80, 255),
                            IM_COL32(90, 210, 130, 255),
                            IM_COL32(90, 155, 255, 255)};
          const char *labels[] = {"X", "Y", "Z"};
          for (int a = 0; a < 3; ++a) {
            auto d = rot * axes[a] * 25.f;
            ImVec2 end{origin.x + d.x, origin.y - d.y};
            overlay->AddLine(origin, end, colors[a], 2);
            overlay->AddText(end, colors[a], labels[a]);
          }
        }
        panel("Status", {0, float(wh) - bottom}, {float(ww), bottom});
        ImGui::Text("%.1f FPS | %zu vertices / %zu triangles | LMB orbit / "
                    "click select | MMB pan | Wheel zoom | %s | x%.1f",
                    io.Framerate, vertices, triangles,
                    running ? "Running" : "Paused", amplification);
        ImGui::End();
        ImGui::Render();
        glViewport(0, 0, fw, fh);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if (capture || (smoke && frame % 12 == 11)) {
          const char *names[] = {"golden-wire", "golden-surface",
                                 "hieu-wire",   "hieu-surface",
                                 "comparison",  "deformation"};
          std::string name =
              smoke
                  ? names[frame / 12]
                  : "capture-" + std::to_string(std::chrono::system_clock::now()
                                                    .time_since_epoch()
                                                    .count());
          std::string path =
              std::string(BRIDGE_ROOT) + "/captures/" + name + ".png";
          View pixels{
              int(region.x * float(fw) / ww), int(region.y * float(fh) / wh),
              int(region.w * float(fw) / ww), int(region.h * float(fh) / wh)};
          bool saved = screenshot(path, pixels, fh);
          feedback = saved ? "PNG saved to captures/" : "PNG save failed";
          if (!saved)
            throw std::runtime_error(feedback);
          std::cout << path << std::endl;
        }
        if (smoke && frame == 10) {
          auto projected = camera.matrix(float(region.w) / region.h) *
                           glm::vec4(0, model.p.deckHeight, 0, 1);
          auto ndc = glm::vec3(projected) / projected.w;
          glm::vec2 mouse{region.x + (ndc.x + 1) * region.w / 2,
                          region.y + (1 - ndc.y) * region.h / 2};
          if (pick(mouse, region, camera, model, renderer) < 0)
            throw std::runtime_error("Visible geometry picking failed");
          for (auto &part : model.parts)
            part.visible = false;
          if (pick(mouse, region, camera, model, renderer) != -1)
            throw std::runtime_error("Hidden geometry picking failed");
          for (auto &part : model.parts)
            part.visible = true;
          std::cout << "Picking visible/hidden checks passed" << std::endl;
        }
        if (glGetError() != GL_NO_ERROR)
          throw std::runtime_error("OpenGL error during rendering");
        glfwSwapBuffers(window);
        ++frame;
        if (frame > 5) {
          measured += elapsed;
          ++measuredFrames;
        }
      }
      std::cout << "Frames " << frame << ", measured mean FPS "
                << (measured > 0 ? measuredFrames / measured : 0) << std::endl;
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    glfwTerminate();
    return 1;
  }
}
