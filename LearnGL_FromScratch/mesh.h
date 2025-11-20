#ifndef MESH_H
#define MESH_H

#include "graphics_api.h"
#include <vector>
#include "arena.h"

struct Vertex
{
  float position[3];
  float normal[3];
  float color[3];
};

struct Mesh
{
  // Opaque GPU handles (backend-agnostic)
  GraphicsVertexArray vao;
  GraphicsBuffer vbo;
  GraphicsBuffer ebo;
  int index_count;
  
  // CPU-side data (optional - can free after upload)
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;
  
  Mesh() : vao(nullptr), vbo(nullptr), ebo(nullptr), index_count(0) {}
  
  void create(const std::vector<Vertex> &verts, const std::vector<unsigned int> &inds, GraphicsAPI* gfx);
  void draw(GraphicsAPI* gfx) const;
  void destroy(GraphicsAPI* gfx);
  
  // Factory methods
  static Mesh create_ground(GraphicsAPI* gfx, float size, float r = 0.2f, float g = 0.3f, float b = 0.2f);
  static Mesh create_box(GraphicsAPI* gfx, float w, float h, float d, float r = 0.8f, float g = 0.2f, float b = 0.2f);
  static Mesh create_sphere(GraphicsAPI* gfx, float radius, int sectors = 36, int stacks = 18,
                           float r = 0.2f, float g = 0.2f, float b = 0.8f);
  static Mesh create_cylinder(GraphicsAPI* gfx, float radius, float height, int sectors = 36,
                             float r = 0.8f, float g = 0.8f, float b = 0.2f);
  static Mesh create_cone(GraphicsAPI* gfx, float radius, float height, int sectors = 36,
                         float r = 0.8f, float g = 0.4f, float b = 0.2f);
};

#endif // MESH_H
