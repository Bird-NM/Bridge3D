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
  unsigned program;
  int selected = -1;
  float explode = 0, amplitude = 0, loadX = 0;
  bool fullTriangles = false, points = false, hidden = true;
  Renderer();
  ~Renderer();
  void draw(const GpuMesh &mesh, const Component &part, const Model &model,
            glm::mat4 vp, int mode, bool ghost = false);
};
