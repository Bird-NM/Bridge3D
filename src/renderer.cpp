#include "renderer.hpp"
#include <algorithm>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
GpuMesh::GpuMesh(const Mesh &m) {
  vertices = int(m.vertices.size());
  triangles = int(m.triangles.size());
  edges = int(m.edges.size());
  std::vector<unsigned> all;
  for (size_t i = 0; i < m.triangles.size(); i += 3)
    all.insert(all.end(),
               {m.triangles[i], m.triangles[i + 1], m.triangles[i + 1],
                m.triangles[i + 2], m.triangles[i + 2], m.triangles[i]});
  allEdges = int(all.size());
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &tri);
  glGenBuffers(1, &edge);
  glGenBuffers(1, &triEdges);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, m.vertices.size() * sizeof(Vertex),
               m.vertices.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)sizeof(glm::vec3));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, mobility));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, tint));
  glEnableVertexAttribArray(3);
  for (auto pair :
       std::vector<std::pair<unsigned, const std::vector<unsigned> *>>{
           {tri, &m.triangles}, {edge, &m.edges}, {triEdges, &all}}) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pair.first);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 pair.second->size() * sizeof(unsigned), pair.second->data(),
                 GL_STATIC_DRAW);
  }
}
GpuMesh::~GpuMesh() {
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &tri);
  glDeleteBuffers(1, &edge);
  glDeleteBuffers(1, &triEdges);
  glDeleteVertexArrays(1, &vao);
}
static unsigned shader(unsigned type, const char *source) {
  unsigned s = glCreateShader(type);
  glShaderSource(s, 1, &source, nullptr);
  glCompileShader(s);
  int ok;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[4096];
    glGetShaderInfoLog(s, 4096, nullptr, log);
    throw std::runtime_error(log);
  }
  return s;
}
static unsigned programFromFiles(const char *vertex, const char *fragment) {
  auto read = [](const char *name) {
    std::ifstream file(std::string(BRIDGE_ROOT) + "/shaders/" + name);
    if (!file)
      throw std::runtime_error("Shader file missing");
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
  };
  auto vs = read(vertex), fs = read(fragment);
  unsigned v = shader(GL_VERTEX_SHADER, vs.c_str()),
           f = shader(GL_FRAGMENT_SHADER, fs.c_str());
  unsigned program = glCreateProgram();
  glAttachShader(program, v);
  glAttachShader(program, f);
  glLinkProgram(program);
  int ok;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  glDeleteShader(v);
  glDeleteShader(f);
  if (!ok) {
    char log[4096];
    glGetProgramInfoLog(program, 4096, nullptr, log);
    throw std::runtime_error(log);
  }
  return program;
}
Renderer::Renderer() {
  program = programFromFiles("bridge.vert", "bridge.frag");
  depthProgram = programFromFiles("bridge.vert", "depth.frag");
  skyProgram = programFromFiles("sky.vert", "sky.frag");
  toneProgram = programFromFiles("sky.vert", "tonemap.frag");
  glGenVertexArrays(1, &skyVao);
  glGenFramebuffers(1, &shadowFbo);
  glGenTextures(1, &shadowTexture);
  glBindTexture(GL_TEXTURE_2D, shadowTexture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, shadowResolution,
               shadowResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  float border[] = {1, 1, 1, 1};
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
  glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                         shadowTexture, 0);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    throw std::runtime_error("Shadow framebuffer incomplete");
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
Renderer::~Renderer() {
  glDeleteFramebuffers(1, &sceneFbo);
  glDeleteFramebuffers(1, &resolveFbo);
  glDeleteTextures(1, &resolveTexture);
  glDeleteProgram(toneProgram);
  glDeleteRenderbuffers(1, &sceneColor);
  glDeleteRenderbuffers(1, &sceneDepth);
  glDeleteTextures(1, &shadowTexture);
  glDeleteFramebuffers(1, &shadowFbo);
  glDeleteVertexArrays(1, &skyVao);
  glDeleteProgram(program);
  glDeleteProgram(depthProgram);
  glDeleteProgram(skyProgram);
}
void Renderer::sky(glm::mat4 vp) {
  glUseProgram(skyProgram);
  auto inv = glm::inverse(vp);
  glUniformMatrix4fv(glGetUniformLocation(skyProgram, "inverseVP"), 1, GL_FALSE,
                     glm::value_ptr(inv));
  glUniform3fv(glGetUniformLocation(skyProgram, "eye"), 1, glm::value_ptr(eye));
  glUniform3fv(glGetUniformLocation(skyProgram, "sunDirection"), 1,
               glm::value_ptr(sunDirection));
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glBindVertexArray(skyVao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
}
void Renderer::beginShadow(glm::vec3 minimum, glm::vec3 maximum) {
  auto center = (minimum + maximum) / 2.f;
  auto view =
      glm::lookAt(center + sunDirection * 700.f, center, glm::vec3(0, 1, 0));
  glm::vec3 lo(1e9f), hi(-1e9f);
  for (int x = 0; x < 2; ++x)
    for (int y = 0; y < 2; ++y)
      for (int z = 0; z < 2; ++z) {
        auto point = glm::vec3(view * glm::vec4(x ? maximum.x : minimum.x,
                                                y ? maximum.y : minimum.y,
                                                z ? maximum.z : minimum.z, 1));
        lo = glm::min(lo, point);
        hi = glm::max(hi, point);
      }
  lightVP = glm::ortho(lo.x - 4, hi.x + 4, lo.y - 4, hi.y + 4, -hi.z - 20,
                       -lo.z + 20) *
            view;
  glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
  glViewport(0, 0, shadowResolution, shadowResolution);
  glClear(GL_DEPTH_BUFFER_BIT);
  shadowPass = true;
}
void Renderer::endShadow() {
  shadowPass = false;
  glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
}
void Renderer::draw(const GpuMesh &mesh, const Component &part, const Model &m,
                    glm::mat4 vp, int mode, bool ghost) {
  unsigned activeProgram = (shadowPass || depthOnly) ? depthProgram : program;
  glUseProgram(activeProgram);
  auto f = [&](const char *key, float x) {
    glUniform1f(glGetUniformLocation(activeProgram, key), x);
  };
  auto i = [&](const char *key, int x) {
    glUniform1i(glGetUniformLocation(activeProgram, key), x);
  };
  glUniformMatrix4fv(glGetUniformLocation(activeProgram, "vp"), 1, GL_FALSE,
                     glm::value_ptr(vp));
  f("amplitude", ghost ? 0 : amplitude);
  f("loadX", loadX);
  f("length", m.p.length);
  f("towerX", m.p.towerX);
  f("hieuTowerX", m.p.hieuTowerX);
  f("towerHeight", m.p.towerHeight);
  f("deckHeight", m.p.deckHeight);
  f("explode", explode);
  f("environmentFade", environmentFade);
  f("time", time);
  i("waterRipples", waterRipples);
  i("hieu", m.hieu);
  i("part", part.id);
  i("clearanceCutaway", m.hieu && clearanceCutaway);
  f("cutawayY", m.p.deckHeight + .6f + m.p.vehicleClearanceHeight + .5f);
  glUniformMatrix4fv(glGetUniformLocation(activeProgram, "modelMatrix"), 1,
                     GL_FALSE, glm::value_ptr(modelMatrix));
  glUniformMatrix4fv(glGetUniformLocation(activeProgram, "lightVP"), 1,
                     GL_FALSE, glm::value_ptr(lightVP));
  auto normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
  glUniformMatrix3fv(glGetUniformLocation(activeProgram, "normalMatrix"), 1,
                     GL_FALSE, glm::value_ptr(normalMatrix));
  glUniform3fv(glGetUniformLocation(activeProgram, "eye"), 1,
               glm::value_ptr(eye));
  glUniform3fv(glGetUniformLocation(activeProgram, "sunDirection"), 1,
               glm::value_ptr(sunDirection));
  i("presentation", presentation);
  i("shadows", shadows && presentation);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, shadowTexture);
  i("shadowMap", 0);
  glBindVertexArray(mesh.vao);
  if (shadowPass || depthOnly) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 2);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.tri);
    glDrawElements(GL_TRIANGLES, mesh.triangles, GL_UNSIGNED_INT, nullptr);
    glDisable(GL_POLYGON_OFFSET_FILL);
    return;
  }

  auto color = part.id == selected ? glm::vec3(1, .72, .16) : part.color;
  if (ghost)
    color = glm::vec3(.26, .32, .38);
  auto setColor = [&](glm::vec3 c) {
    glUniform3fv(glGetUniformLocation(activeProgram, "color"), 1,
                 glm::value_ptr(c));
  };
  if (mode != 0) {
    i("lit", 1);
    setColor(color);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1, 1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.tri);
    glDrawElements(GL_TRIANGLES, mesh.triangles, GL_UNSIGNED_INT, nullptr);
    glDisable(GL_POLYGON_OFFSET_FILL);
  }
  if (mode != 1) {
    i("lit", 0);
    setColor(mode == 2 ? color * .45f : color);
    if (mode == 0 && !hidden)
      glDisable(GL_DEPTH_TEST);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                 fullTriangles ? mesh.triEdges : mesh.edge);
    glDrawElements(GL_LINES, fullTriangles ? mesh.allEdges : mesh.edges,
                   GL_UNSIGNED_INT, nullptr);
    glEnable(GL_DEPTH_TEST);
  }
  if (points) {
    i("lit", 0);
    setColor(color);
    glDrawArrays(GL_POINTS, 0, mesh.vertices);
  }
}

void Renderer::beginFrame(int width, int height) {
  if (!sceneFbo) {
    glGenFramebuffers(1, &sceneFbo);
    glGenRenderbuffers(1, &sceneColor);
    glGenRenderbuffers(1, &sceneDepth);
    glGenFramebuffers(1, &resolveFbo);
    glGenTextures(1, &resolveTexture);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
  if (width != sceneWidth || height != sceneHeight) {
    sceneWidth = width;
    sceneHeight = height;
    int maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    sceneSamples = std::min(4, maxSamples);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneColor);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, sceneSamples, GL_RGBA16F,
                                     width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER, sceneColor);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneDepth);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, sceneSamples,
                                     GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, sceneDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      throw std::runtime_error("Scene MSAA framebuffer incomplete");
    glBindFramebuffer(GL_FRAMEBUFFER, resolveFbo);
    glBindTexture(GL_TEXTURE_2D, resolveTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA,
                 GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           resolveTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      throw std::runtime_error("HDR resolve framebuffer incomplete");
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFbo);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES,
                                 &sceneSamples);
    std::cout << "Scene MSAA " << sceneSamples << " samples at " << width << "x"
              << height << std::endl;
  }
}
void Renderer::resolveFrame(int width, int height) {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFbo);
  glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                    GL_COLOR_BUFFER_BIT, GL_NEAREST);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, width, height);
  glUseProgram(toneProgram);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, resolveTexture);
  glUniform1i(glGetUniformLocation(toneProgram, "scene"), 0);
  glUniform1i(glGetUniformLocation(toneProgram, "presentation"), presentation);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glBindVertexArray(skyVao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);
}
