#ifndef MESH_H
#define MESH_H

#include "graphics_api.h"
#include "arena2.h"
#include "linmath.h"
#include "physics.h"

struct Vertex
{
  r32 *positions;
  r32 *normals;
  r32 *colors;
};

struct Mesh
{
  // Opaque GPU handles (backend-agnostic)
  GraphicsVertexArray vao;
  GraphicsBuffer position_vbo;
  GraphicsBuffer normal_vbo;
  GraphicsBuffer color_vbo;
  GraphicsBuffer ebo;
  s32 index_count;

  // CPU-side data (optional - can free after upload)
  Vertex *vertices;
  u32 *indices;
  s32 vertex_count;
  mat4x4 *model;

  Mesh() : vao(nullptr), position_vbo(nullptr), normal_vbo(nullptr), color_vbo(nullptr), ebo(nullptr),
           index_count(0), vertices(nullptr), indices(nullptr), vertex_count(0) {}

  void create(Arena *arena, Vertex *verts, s32 vert_count, u32 *inds, s32 ind_count, GraphicsAPI *gfx);
  void draw(GraphicsAPI *gfx) const;
  void destroy(GraphicsAPI *gfx);
  void translate(r32 x, r32 y, r32 z);
  JPH::ConvexHullShapeSettings create_convex_hull();

  static Mesh *create_ground(Arena *arena, GraphicsAPI *gfx, r32 size, r32 r = 0.2f, r32 g = 0.3f, r32 b = 0.2f);
  static Mesh *create_box(Arena *arena, GraphicsAPI *gfx, r32 w, r32 h, r32 d, r32 r = 0.8f, r32 g = 0.2f, r32 b = 0.2f);
  static Mesh *create_sphere(Arena *arena, GraphicsAPI *gfx, r32 radius, s32 sectors = 36, s32 stacks = 18, r32 r = 0.2f, r32 g = 0.2f, r32 b = 0.8f);
  static Mesh *create_cylinder(Arena *arena, GraphicsAPI *gfx, r32 radius, r32 height, s32 sectors = 36, r32 r = 0.8f, r32 g = 0.8f, r32 b = 0.2f);
  static Mesh *create_cone(Arena *arena, GraphicsAPI *gfx, r32 radius, r32 height, s32 sectors = 36, r32 r = 0.8f, r32 g = 0.4f, r32 b = 0.2f);
};

#endif // MESH_H
