#include "mesh.h"
#include <cmath>

JPH::ConvexHullShapeSettings Mesh::create_convex_hull()
{
  JPH::Array<JPH::Vec3> verts;
  verts.reserve(vertex_count);
  JPH::Float3 *vertices_ptr = reinterpret_cast<JPH::Float3 *>(vertices->positions);
  verts.assign(vertices_ptr, vertices_ptr + vertex_count);

  JPH::IndexedTriangleList triangles;
  triangles.reserve(index_count / 3);
  for (s32 i = 0; i < index_count; i += 3)
  {
    triangles.emplace_back(
        indices[i + 0],
        indices[i + 1],
        indices[i + 2]);
  }
  return JPH::ConvexHullShapeSettings(verts, JPH::cDefaultConvexRadius);
}

void Mesh::create(Arena *arena, Vertex *verts, s32 vert_count, u32 *inds, s32 ind_count, GraphicsAPI *gfx)
{
  vertices = verts;
  vertex_count = vert_count;
  indices = inds;
  index_count = ind_count;
  model = push_struct_no_zero(arena, mat4x4);
  mat4x4_identity(*model);

  // Create vertex buffer
  position_vbo = gfx->create_buffer(arena, verts->positions, vertex_count * 3 * sizeof(r32));
  normal_vbo = gfx->create_buffer(arena, verts->normals, vertex_count * 3 * sizeof(r32));
  color_vbo = gfx->create_buffer(arena, verts->colors, vertex_count * 3 * sizeof(r32));

  // Create index buffer
  ebo = gfx->create_index_buffer(arena, indices, index_count * sizeof(u32));

  // Create and setup VAO
  vao = gfx->create_vertex_array(arena);
  gfx->bind_vertex_array(vao);
  
  gfx->bind_buffer(position_vbo);
  gfx->enable_vertex_attrib(0);
  gfx->vertex_attrib_pointer(0, 3, 3 * sizeof(r32), 0);

  gfx->bind_buffer(normal_vbo);
  gfx->enable_vertex_attrib(1);
  gfx->vertex_attrib_pointer(1, 3, 3 * sizeof(r32), 0);


  gfx->bind_buffer(color_vbo);
  gfx->enable_vertex_attrib(2);
  gfx->vertex_attrib_pointer(2, 3, 3 * sizeof(r32), 0);
  
  gfx->bind_index_buffer(ebo);
  gfx->bind_vertex_array(nullptr); // Unbind
}

void Mesh::draw(GraphicsAPI *gfx) const
{
  gfx->bind_vertex_array(vao);
  gfx->draw_elements(index_count);
  gfx->bind_vertex_array(nullptr);
}

void Mesh::destroy(GraphicsAPI *gfx)
{
  if (vao)
    gfx->destroy_vertex_array(vao);
  if (position_vbo)
    gfx->destroy_buffer(position_vbo);
  if (normal_vbo)
    gfx->destroy_buffer(normal_vbo);
  if (color_vbo)
    gfx->destroy_buffer(color_vbo);    
  if (ebo)
    gfx->destroy_buffer(ebo);

  vao = nullptr;
  position_vbo = nullptr;
  normal_vbo = nullptr;
  color_vbo = nullptr;
  ebo = nullptr;

  vertices = nullptr;
  indices = nullptr;
}

void Mesh::translate(r32 x, r32 y, r32 z)
{
  mat4x4_translate(*model, x, y, z);
}

#define SET_VERTEX(idx, px, py, pz, nx, ny, nz, cr, cg, cb) \
  positions[(idx)*3+0] = px; positions[(idx)*3+1] = py; positions[(idx)*3+2] = pz; \
  normals[(idx)*3+0] = nx; normals[(idx)*3+1] = ny; normals[(idx)*3+2] = nz; \
  colors[(idx)*3+0] = cr; colors[(idx)*3+1] = cg; colors[(idx)*3+2] = cb;

Mesh *Mesh::create_ground(Arena *arena, GraphicsAPI *gfx, r32 size, r32 r, r32 g, r32 b)
{
  r32 *positions = push_array(arena, r32, 4 * 3);
  r32 *normals = push_array(arena, r32, 4 * 3);
  r32 *colors = push_array(arena, r32, 4 * 3);
  u32 *indices = push_array(arena, u32, 6);

  SET_VERTEX(0, -size, 0, -size, 0, 1, 0, r, g, b);
  SET_VERTEX(1,  size, 0, -size, 0, 1, 0, r, g, b);
  SET_VERTEX(2,  size, 0,  size, 0, 1, 0, r, g, b);
  SET_VERTEX(3, -size, 0,  size, 0, 1, 0, r, g, b);

  indices[0] = 0; indices[1] = 2; indices[2] = 1;
  indices[3] = 0; indices[4] = 3; indices[5] = 2;

  
  Vertex *vertices = push_struct(arena, Vertex);
  *vertices = {positions, normals, colors};

  Mesh *mesh = push_struct(arena, Mesh);
  mesh->create(arena, vertices, 4, indices, 6, gfx);
  return mesh;
}

// ========== Box ==========
Mesh *Mesh::create_box(Arena *arena, GraphicsAPI *gfx, r32 w, r32 h, r32 d, r32 r, r32 g, r32 b)
{
  r32 *positions = push_array(arena, r32, 24 * 3);
  r32 *normals = push_array(arena, r32, 24 * 3);
  r32 *colors = push_array(arena, r32, 24 * 3);
  u32 *indices = push_array(arena, u32, 36);

  s32 v = 0;
  // Front face (normal: 0, 0, 1)
  SET_VERTEX(v, -w, -h,  d,  0, 0, 1, r, g, b); v++;
  SET_VERTEX(v,  w, -h,  d,  0, 0, 1, r, g, b); v++;
  SET_VERTEX(v,  w,  h,  d,  0, 0, 1, r, g, b); v++;
  SET_VERTEX(v, -w,  h,  d,  0, 0, 1, r, g, b); v++;

  // Back face (normal: 0, 0, -1)
  SET_VERTEX(v,  w, -h, -d,  0, 0, -1, r, g, b); v++;
  SET_VERTEX(v, -w, -h, -d,  0, 0, -1, r, g, b); v++;
  SET_VERTEX(v, -w,  h, -d,  0, 0, -1, r, g, b); v++;
  SET_VERTEX(v,  w,  h, -d,  0, 0, -1, r, g, b); v++;

  // Top face (normal: 0, 1, 0)
  SET_VERTEX(v, -w,  h,  d,  0, 1, 0, r, g, b); v++;
  SET_VERTEX(v,  w,  h,  d,  0, 1, 0, r, g, b); v++;
  SET_VERTEX(v,  w,  h, -d,  0, 1, 0, r, g, b); v++;
  SET_VERTEX(v, -w,  h, -d,  0, 1, 0, r, g, b); v++;

  // Bottom face (normal: 0, -1, 0)
  SET_VERTEX(v, -w, -h, -d,  0, -1, 0, r, g, b); v++;
  SET_VERTEX(v,  w, -h, -d,  0, -1, 0, r, g, b); v++;
  SET_VERTEX(v,  w, -h,  d,  0, -1, 0, r, g, b); v++;
  SET_VERTEX(v, -w, -h,  d,  0, -1, 0, r, g, b); v++;

  // Right face (normal: 1, 0, 0)
  SET_VERTEX(v,  w, -h,  d,  1, 0, 0, r, g, b); v++;
  SET_VERTEX(v,  w, -h, -d,  1, 0, 0, r, g, b); v++;
  SET_VERTEX(v,  w,  h, -d,  1, 0, 0, r, g, b); v++;
  SET_VERTEX(v,  w,  h,  d,  1, 0, 0, r, g, b); v++;

  // Left face (normal: -1, 0, 0)
  SET_VERTEX(v, -w, -h, -d, -1, 0, 0, r, g, b); v++;
  SET_VERTEX(v, -w, -h,  d, -1, 0, 0, r, g, b); v++;
  SET_VERTEX(v, -w,  h,  d, -1, 0, 0, r, g, b); v++;
  SET_VERTEX(v, -w,  h, -d, -1, 0, 0, r, g, b); v++;

  s32 idx = 0;
  for (s32 i = 0; i < 6; i++)
  {
    u32 base = i * 4;
    indices[idx++] = base + 0;
    indices[idx++] = base + 1;
    indices[idx++] = base + 2;
    indices[idx++] = base + 0;
    indices[idx++] = base + 2;
    indices[idx++] = base + 3;
  }

  Vertex *vertex_data = push_struct(arena, Vertex);
  *vertex_data = {positions, normals, colors};

  Mesh *mesh = push_struct(arena, Mesh);
  mesh->create(arena, vertex_data, 24, indices, 36, gfx);
  return mesh;
}


// ========== Sphere ==========
Mesh *Mesh::create_sphere(Arena *arena, GraphicsAPI *gfx, r32 radius, s32 sectors, s32 stacks,
                          r32 r, r32 g, r32 b)
{
  s32 vert_count = (stacks + 1) * (sectors + 1);
  s32 ind_count = stacks * sectors * 6;

  r32 *positions = push_array(arena, r32, vert_count * 3);
  r32 *normals = push_array(arena, r32, vert_count * 3);
  r32 *colors = push_array(arena, r32, vert_count * 3);
  u32 *indices = push_array(arena, u32, ind_count);

  s32 v = 0;

  for (s32 i = 0; i <= stacks; i++)
  {
    r32 phi = PI * i / stacks;

    for (s32 j = 0; j <= sectors; j++)
    {
      r32 theta = 2.0f * PI * j / sectors;

      r32 x = radius * sinf(phi) * cosf(theta);
      r32 y = radius * cosf(phi);
      r32 z = radius * sinf(phi) * sinf(theta);

      r32 nx = x / radius;
      r32 ny = y / radius;
      r32 nz = z / radius;

      positions[v*3 + 0] = x;
      positions[v*3 + 1] = y;
      positions[v*3 + 2] = z;

      normals[v*3 + 0] = nx;
      normals[v*3 + 1] = ny;
      normals[v*3 + 2] = nz;

      colors[v*3 + 0] = r;
      colors[v*3 + 1] = g;
      colors[v*3 + 2] = b;

      v++;
    }
  }

  s32 idx = 0;
  for (s32 i = 0; i < stacks; i++)
  {
    for (s32 j = 0; j < sectors; j++)
    {
      u32 first = i * (sectors + 1) + j;
      u32 second = first + sectors + 1;

      indices[idx++] = first;
      indices[idx++] = second;
      indices[idx++] = first + 1;

      indices[idx++] = second;
      indices[idx++] = second + 1;
      indices[idx++] = first + 1;
    }
  }
  Vertex *vertex_data = push_struct(arena, Vertex);
  *vertex_data = {positions, normals, colors};

  Mesh *mesh = push_struct(arena, Mesh);
  mesh->create(arena, vertex_data, vert_count,  indices, ind_count, gfx);
  return mesh;
}

// ========== Cylinder ==========
Mesh *Mesh::create_cylinder(Arena *arena, GraphicsAPI *gfx, r32 radius, r32 height, s32 sectors,
                           r32 r, r32 g, r32 b)
{
  // Calculate sizes: side (2*(sectors+1)) + top cap (1 + sectors+1) + bottom cap (1 + sectors+1)
  s32 vert_count = 2 * (sectors + 1) + 2 * (sectors + 2);
  s32 ind_count = sectors * 6 + sectors * 3 + sectors * 3; // side + top + bottom

  r32 *positions = push_array(arena, r32, vert_count * 3);
  r32 *normals = push_array(arena, r32, vert_count * 3);
  r32 *colors = push_array(arena, r32, vert_count * 3);
  u32 *indices = push_array(arena, u32, ind_count);

  r32 half_height = height / 2.0f;
  s32 v = 0;
  s32 idx = 0;

  // Side vertices
  for (s32 i = 0; i <= sectors; i++)
  {
    r32 theta = 2.0f * PI * i / sectors;
    r32 x = radius * cosf(theta);
    r32 z = radius * sinf(theta);
    r32 nx = x / radius;
    r32 nz = z / radius;

    // Top vertex
    positions[v*3+0] = x; positions[v*3+1] = half_height; positions[v*3+2] = z;
    normals[v*3+0] = nx; normals[v*3+1] = 0; normals[v*3+2] = nz;
    colors[v*3+0] = r; colors[v*3+1] = g; colors[v*3+2] = b;
    v++;

    // Bottom vertex
    positions[v*3+0] = x; positions[v*3+1] = -half_height; positions[v*3+2] = z;
    normals[v*3+0] = nx; normals[v*3+1] = 0; normals[v*3+2] = nz;
    colors[v*3+0] = r; colors[v*3+1] = g; colors[v*3+2] = b;
    v++;
  }

  // Side faces
  for (s32 i = 0; i < sectors; i++)
  {
    u32 base = i * 2;
    indices[idx++] = base;
    indices[idx++] = base + 2;
    indices[idx++] = base + 1;
    indices[idx++] = base + 1;
    indices[idx++] = base + 2;
    indices[idx++] = base + 3;
  }

  // Top cap
  u32 top_center = v;
  positions[v*3+0] = 0; positions[v*3+1] = half_height; positions[v*3+2] = 0;
  normals[v*3+0] = 0; normals[v*3+1] = 1; normals[v*3+2] = 0;
  colors[v*3+0] = r; colors[v*3+1] = g; colors[v*3+2] = b;
  v++;

  for (s32 i = 0; i <= sectors; i++)
  {
    r32 theta = 2.0f * PI * i / sectors;
    r32 x = radius * cosf(theta);
    r32 z = radius * sinf(theta);
    
    positions[v*3+0] = x; positions[v*3+1] = half_height; positions[v*3+2] = z;
    normals[v*3+0] = 0; normals[v*3+1] = 1; normals[v*3+2] = 0;
    colors[v*3+0] = r; colors[v*3+1] = g; colors[v*3+2] = b;
    v++;
  }

  for (s32 i = 0; i < sectors; i++)
  {
    indices[idx++] = top_center;
    indices[idx++] = top_center + i + 1;
    indices[idx++] = top_center + i + 2;
  }

  // Bottom cap
  u32 bottom_center = v;
  positions[v*3+0] = 0; positions[v*3+1] = -half_height; positions[v*3+2] = 0;
  normals[v*3+0] = 0; normals[v*3+1] = -1; normals[v*3+2] = 0;
  colors[v*3+0] = r; colors[v*3+1] = g; colors[v*3+2] = b;
  v++;

  for (s32 i = 0; i <= sectors; i++)
  {
    r32 theta = 2.0f * PI * i / sectors;
    r32 x = radius * cosf(theta);
    r32 z = radius * sinf(theta);
    
    positions[v*3+0] = x; positions[v*3+1] = -half_height; positions[v*3+2] = z;
    normals[v*3+0] = 0; normals[v*3+1] = -1; normals[v*3+2] = 0;
    colors[v*3+0] = r; colors[v*3+1] = g; colors[v*3+2] = b;
    v++;
  }

  for (s32 i = 0; i < sectors; i++)
  {
    indices[idx++] = bottom_center;
    indices[idx++] = bottom_center + i + 2;
    indices[idx++] = bottom_center + i + 1;
  }

  Vertex* vertex_data = push_struct(arena, Vertex);
  *vertex_data = {positions, normals, colors};

  Mesh *mesh = push_struct(arena, Mesh);
  mesh->create(arena, vertex_data, v, indices, idx, gfx);
  return mesh;
}

// ========== Cone ==========
Mesh *Mesh::create_cone(Arena *arena, GraphicsAPI *gfx, r32 radius, r32 height, s32 sectors,
                       r32 r, r32 g, r32 b)
{
  // Apex (1) + base circle (sectors+1) + bottom cap center (1) + bottom circle (sectors+1)
  s32 vert_count = 1 + (sectors + 1) + 1 + (sectors + 1);
  s32 ind_count = sectors * 3 + sectors * 3; // side + bottom

  r32 *positions = push_array(arena, r32, vert_count * 3);
  r32 *normals = push_array(arena, r32, vert_count * 3);
  r32 *colors = push_array(arena, r32, vert_count * 3);
  u32 *indices = push_array(arena, u32, ind_count);

  r32 half_height = height / 2.0f;
  s32 v = 0;
  s32 idx = 0;

  // Apex
  u32 apex = v;
  positions[v*3+0] = 0; positions[v*3+1] = half_height; positions[v*3+2] = 0;
  normals[v*3+0] = 0; normals[v*3+1] = 1; normals[v*3+2] = 0;
  colors[v*3+0] = r; colors[v*3+1] = g; colors[v*3+2] = b;
  v++;

  // Base circle
  r32 slant = sqrtf(radius * radius + height * height);
  for (s32 i = 0; i <= sectors; i++)
  {
    r32 theta = 2.0f * PI * i / sectors;
    r32 x = radius * cosf(theta);
    r32 z = radius * sinf(theta);

    r32 nx = (height * x) / (radius * slant);
    r32 ny = radius / slant;
    r32 nz = (height * z) / (radius * slant);

    positions[v*3+0] = x; positions[v*3+1] = -half_height; positions[v*3+2] = z;
    normals[v*3+0] = nx; normals[v*3+1] = ny; normals[v*3+2] = nz;
    colors[v*3+0] = r; colors[v*3+1] = g; colors[v*3+2] = b;
    v++;
  }

  // Side faces
  for (s32 i = 0; i < sectors; i++)
  {
    indices[idx++] = apex;
    indices[idx++] = apex + i + 1;
    indices[idx++] = apex + i + 2;
  }

  // Bottom cap
  u32 bottom_center = v;
  positions[v*3+0] = 0; positions[v*3+1] = -half_height; positions[v*3+2] = 0;
  normals[v*3+0] = 0; normals[v*3+1] = -1; normals[v*3+2] = 0;
  colors[v*3+0] = r; colors[v*3+1] = g; colors[v*3+2] = b;
  v++;

  for (s32 i = 0; i <= sectors; i++)
  {
    r32 theta = 2.0f * PI * i / sectors;
    r32 x = radius * cosf(theta);
    r32 z = radius * sinf(theta);
    
    positions[v*3+0] = x; positions[v*3+1] = -half_height; positions[v*3+2] = z;
    normals[v*3+0] = 0; normals[v*3+1] = -1; normals[v*3+2] = 0;
    colors[v*3+0] = r; colors[v*3+1] = g; colors[v*3+2] = b;
    v++;
  }

  for (s32 i = 0; i < sectors; i++)
  {
    indices[idx++] = bottom_center;
    indices[idx++] = bottom_center + i + 2;
    indices[idx++] = bottom_center + i + 1;
  }

  Vertex *vertex_data = push_struct(arena, Vertex);
  *vertex_data = {positions, normals, colors};

  Mesh *mesh = push_struct(arena, Mesh);
  mesh->create(arena, vertex_data, v, indices, idx, gfx);
  return mesh;
}
