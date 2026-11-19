#include "renderer.hpp"
#include <glm/gtc/type_ptr.hpp>
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
Renderer::Renderer() {
  const char *vs = R"(#version 330 core
layout(location=2) in float mobility;
layout(location=0) in vec3 pos; layout(location=1) in vec3 normal;
uniform mat4 vp; uniform float amplitude,loadX,length,towerX,towerHeight,deckHeight,explode;
uniform int hieu,part;
out vec3 N;
float delta(float x){
 float l=-length/2.,r=length/2.;
 if(hieu==1){if(x<0.)r=0.;else l=0.;}
 else {if(x < -towerX)r=-towerX;else if(x<towerX){l=-towerX;r=towerX;}else l=towerX;}
 float t=clamp((x-l)/(r-l),0.,1.);
 return -amplitude*pow(sin(3.14159265*t),2.)*exp(-pow((x-loadX)/45.,2.));
}
void main(){vec3 p=pos;N=normal;
 if(part>0 && part<6){float w=clamp((towerHeight-pos.y)/(towerHeight-deckHeight-2.),0.,1.);w*=mobility;p.y+=delta(pos.x)*w;
 float dx=(delta(pos.x+.02)-delta(pos.x-.02))/.04*w;
 float dy=pos.y>deckHeight+2. && pos.y<towerHeight ? -delta(pos.x)/(towerHeight-deckHeight-2.):0.;
 N=normalize(vec3(normal.x-dx*normal.y/(1.+dy),normal.y/(1.+dy),normal.z));}
 p.y+=explode*(part>=8?0.:part==0||part==7?2.:part==2||part==3?4.:part==6?0.:1.);
 gl_Position=vp*vec4(p,1.);gl_PointSize=3.;})";
  const char *fs = R"(#version 330 core
in vec3 N;out vec4 outputColor;uniform vec3 color;uniform int lit;
void main(){float light=lit==1?.5+.5*abs(dot(normalize(N),normalize(vec3(.3,.8,.5)))):1.;outputColor=vec4(color*light,1.);})";
  unsigned v = shader(GL_VERTEX_SHADER, vs), f = shader(GL_FRAGMENT_SHADER, fs);
  program = glCreateProgram();
  glAttachShader(program, v);
  glAttachShader(program, f);
  glLinkProgram(program);
  int ok;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  glDeleteShader(v);
  glDeleteShader(f);
  if (!ok)
    throw std::runtime_error("Shader link failed");
}
Renderer::~Renderer() { glDeleteProgram(program); }
void Renderer::draw(const GpuMesh &mesh, const Component &part, const Model &m,
                    glm::mat4 vp, int mode, bool ghost) {
  glUseProgram(program);
  auto f = [&](const char *key, float x) {
    glUniform1f(glGetUniformLocation(program, key), x);
  };
  auto i = [&](const char *key, int x) {
    glUniform1i(glGetUniformLocation(program, key), x);
  };
  glUniformMatrix4fv(glGetUniformLocation(program, "vp"), 1, GL_FALSE,
                     glm::value_ptr(vp));
  f("amplitude", ghost ? 0 : amplitude);
  f("loadX", loadX);
  f("length", m.p.length);
  f("towerX", m.p.towerX);
  f("towerHeight", m.p.towerHeight);
  f("deckHeight", m.p.deckHeight);
  f("explode", explode);
  i("hieu", m.hieu);
  i("part", part.id);
  glBindVertexArray(mesh.vao);
  auto color = part.id == selected ? glm::vec3(1, .72, .16) : part.color;
  if (ghost)
    color = glm::vec3(.26, .32, .38);
  auto setColor = [&](glm::vec3 c) {
    glUniform3fv(glGetUniformLocation(program, "color"), 1, glm::value_ptr(c));
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
  if (mode == 0 && hidden) {
    glColorMask(0, 0, 0, 0);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1, 1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.tri);
    glDrawElements(GL_TRIANGLES, mesh.triangles, GL_UNSIGNED_INT, nullptr);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glColorMask(1, 1, 1, 1);
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
