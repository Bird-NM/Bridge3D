#pragma once
#include "model.hpp"
#include <glad/glad.h>
struct GpuMesh {
  unsigned vao = 0, vbo = 0, tri = 0, edge = 0, triEdges = 0;
  int triangles = 0, edges = 0, allEdges = 0, vertices = 0;
  GpuMesh(const Mesh &m);
  ~GpuMesh();
  GpuMesh(const GpuMesh &) = delete;
  GpuMesh &operator=(const GpuMesh &) = delete;
};
struct Renderer {
  unsigned program = 0, depthProgram = 0, skyProgram = 0, skyVao = 0,
           shadowFbo = 0, shadowTexture = 0;
  glm::mat4 lightVP{1}, modelMatrix{1};
  glm::vec3 eye{0}, sunDirection = glm::normalize(glm::vec3(-.35f, .82f, .45f));
  bool presentation = true, shadows = true, shadowPass = false,
       depthOnly = false;
  int shadowResolution = 2048;
  unsigned sceneFbo = 0, sceneColor = 0, sceneDepth = 0, resolveFbo = 0,
           resolveTexture = 0, toneProgram = 0;
  int sceneWidth = 0, sceneHeight = 0, sceneSamples = 0;
  void beginFrame(int width, int height);
  void resolveFrame(int width, int height);
  int selected = -1;
  float explode = 0, amplitude = 0, loadX = 0;
  float environmentFade = 0, time = 0;
  bool waterRipples = true, clearanceCutaway = false;
  bool fullTriangles = false, points = false, hidden = true;
  Renderer();
  void sky(glm::mat4 vp);
  void beginShadow(glm::vec3 minimum, glm::vec3 maximum);
  void endShadow();
  ~Renderer();
  void draw(const GpuMesh &mesh, const Component &part, const Model &model,
            glm::mat4 vp, int mode, bool ghost = false);
};
