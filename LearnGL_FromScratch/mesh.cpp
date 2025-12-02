#include "mesh.h"
#include <cmath>

void Mesh::create(Arena *arena, Vertex *verts, s32 vert_count, u32 *inds, s32 ind_count, GraphicsAPI *gfx)
{
  vertices = verts;
  vertex_count = vert_count;
  indices = inds;
  index_count = ind_count;
  model = push_struct_no_zero(arena, mat4x4);
  mat4x4_identity(*model);

  // Create vertex buffer
  vbo = gfx->create_buffer(arena, vertices, vertex_count * sizeof(Vertex));

  // Create index buffer
  ebo = gfx->create_index_buffer(arena, indices, index_count * sizeof(u32));

  // Create and setup VAO
  vao = gfx->create_vertex_array(arena);
  gfx->bind_vertex_array(vao);
  gfx->bind_buffer(vbo);
  gfx->bind_index_buffer(ebo);

  // Position attribute (location 0)
  gfx->enable_vertex_attrib(0);
  gfx->vertex_attrib_pointer(0, 3, sizeof(Vertex), offsetof(Vertex, position));

  // Normal attribute (location 1)
  gfx->enable_vertex_attrib(1);
  gfx->vertex_attrib_pointer(1, 3, sizeof(Vertex), offsetof(Vertex, normal));

  // Color attribute (location 2)
  gfx->enable_vertex_attrib(2);
  gfx->vertex_attrib_pointer(2, 3, sizeof(Vertex), offsetof(Vertex, color));

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
  if (vbo)
    gfx->destroy_buffer(vbo);
  if (ebo)
    gfx->destroy_buffer(ebo);

  vao = nullptr;
  vbo = nullptr;
  ebo = nullptr;

  vertices = nullptr;
  indices = nullptr;
}

void Mesh::translate(r32 x, r32 y, r32 z)
{
  mat4x4_translate(*model, x, y, z);
}

// ========== Ground ==========
Mesh *Mesh::create_ground(Arena *arena, GraphicsAPI *gfx, r32 size, r32 r, r32 g, r32 b)
{
  Vertex *vertices = push_array(arena, Vertex, 4);
  u32 *indices = push_array(arena, u32, 6);

  vertices[0] = {{-size, 0, -size}, {0, 1, 0}, {r, g, b}};
  vertices[1] = {{size, 0, -size}, {0, 1, 0}, {r, g, b}};
  vertices[2] = {{size, 0, size}, {0, 1, 0}, {r, g, b}};
  vertices[3] = {{-size, 0, size}, {0, 1, 0}, {r, g, b}};

  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  indices[3] = 0;
  indices[4] = 2;
  indices[5] = 3;

  Mesh *mesh = push_struct(arena, Mesh);
  mesh->create(arena, vertices, 4, indices, 6, gfx);
  return mesh;
}

// ========== Box ==========
Mesh *Mesh::create_box(Arena *arena, GraphicsAPI *gfx, r32 w, r32 h, r32 d, r32 r, r32 g, r32 b)
{
  Vertex *vertices = push_array(arena, Vertex, 24);
  u32 *indices = push_array(arena, u32, 36);

  s32 v = 0;

  // Front face
  vertices[v++] = {{-w, -h, d}, {0, 0, 1}, {r, g, b}};
  vertices[v++] = {{w, -h, d}, {0, 0, 1}, {r, g, b}};
  vertices[v++] = {{w, h, d}, {0, 0, 1}, {r, g, b}};
  vertices[v++] = {{-w, h, d}, {0, 0, 1}, {r, g, b}};

  // Back face
  vertices[v++] = {{w, -h, -d}, {0, 0, -1}, {r, g, b}};
  vertices[v++] = {{-w, -h, -d}, {0, 0, -1}, {r, g, b}};
  vertices[v++] = {{-w, h, -d}, {0, 0, -1}, {r, g, b}};
  vertices[v++] = {{w, h, -d}, {0, 0, -1}, {r, g, b}};

  // Top face
  vertices[v++] = {{-w, h, d}, {0, 1, 0}, {r, g, b}};
  vertices[v++] = {{w, h, d}, {0, 1, 0}, {r, g, b}};
  vertices[v++] = {{w, h, -d}, {0, 1, 0}, {r, g, b}};
  vertices[v++] = {{-w, h, -d}, {0, 1, 0}, {r, g, b}};

  // Bottom face
  vertices[v++] = {{-w, -h, -d}, {0, -1, 0}, {r, g, b}};
  vertices[v++] = {{w, -h, -d}, {0, -1, 0}, {r, g, b}};
  vertices[v++] = {{w, -h, d}, {0, -1, 0}, {r, g, b}};
  vertices[v++] = {{-w, -h, d}, {0, -1, 0}, {r, g, b}};

  // Right face
  vertices[v++] = {{w, -h, d}, {1, 0, 0}, {r, g, b}};
  vertices[v++] = {{w, -h, -d}, {1, 0, 0}, {r, g, b}};
  vertices[v++] = {{w, h, -d}, {1, 0, 0}, {r, g, b}};
  vertices[v++] = {{w, h, d}, {1, 0, 0}, {r, g, b}};

  // Left face
  vertices[v++] = {{-w, -h, -d}, {-1, 0, 0}, {r, g, b}};
  vertices[v++] = {{-w, -h, d}, {-1, 0, 0}, {r, g, b}};
  vertices[v++] = {{-w, h, d}, {-1, 0, 0}, {r, g, b}};
  vertices[v++] = {{-w, h, -d}, {-1, 0, 0}, {r, g, b}};

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

  Mesh *mesh = push_struct(arena, Mesh);
  mesh->create(arena, vertices, 24, indices, 36, gfx);
  return mesh;
}

// ========== Sphere ==========
Mesh *Mesh::create_sphere(Arena *arena, GraphicsAPI *gfx, r32 radius, s32 sectors, s32 stacks,
                          r32 r, r32 g, r32 b)
{
  // Calculate exact sizes [web:54]
  s32 vert_count = (stacks + 1) * (sectors + 1);
  s32 ind_count = stacks * sectors * 6;

  Vertex *vertices = push_array(arena, Vertex, vert_count);
  u32 *indices = push_array(arena, u32, ind_count);

  const r32 PI = 3.14159265359f;
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

      vertices[v++] = {{x, y, z}, {nx, ny, nz}, {r, g, b}};
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

  Mesh *mesh = push_struct(arena, Mesh);
  mesh->create(arena, vertices, vert_count, indices, ind_count, gfx);
  return mesh;
}

// ========== Cylinder ==========
Mesh *Mesh::create_cylinder(Arena *arena, GraphicsAPI *gfx, r32 radius, r32 height, s32 sectors,
                           r32 r, r32 g, r32 b)
{
  // Calculate sizes: side (2*(sectors+1)) + top cap (1 + sectors+1) + bottom cap (1 + sectors+1)
  s32 vert_count = 2 * (sectors + 1) + 2 * (sectors + 2);
  s32 ind_count = sectors * 6 + sectors * 3 + sectors * 3; // side + top + bottom

  Vertex *vertices = push_array(arena, Vertex, vert_count);
  u32 *indices = push_array(arena, u32, ind_count);

  const r32 PI = 3.14159265359f;
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

    vertices[v++] = {{x, half_height, z}, {nx, 0, nz}, {r, g, b}};
    vertices[v++] = {{x, -half_height, z}, {nx, 0, nz}, {r, g, b}};
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
  vertices[v++] = {{0, half_height, 0}, {0, 1, 0}, {r, g, b}};

  for (s32 i = 0; i <= sectors; i++)
  {
    r32 theta = 2.0f * PI * i / sectors;
    r32 x = radius * cosf(theta);
    r32 z = radius * sinf(theta);
    vertices[v++] = {{x, half_height, z}, {0, 1, 0}, {r, g, b}};
  }

  for (s32 i = 0; i < sectors; i++)
  {
    indices[idx++] = top_center;
    indices[idx++] = top_center + i + 1;
    indices[idx++] = top_center + i + 2;
  }

  // Bottom cap
  u32 bottom_center = v;
  vertices[v++] = {{0, -half_height, 0}, {0, -1, 0}, {r, g, b}};

  for (s32 i = 0; i <= sectors; i++)
  {
    r32 theta = 2.0f * PI * i / sectors;
    r32 x = radius * cosf(theta);
    r32 z = radius * sinf(theta);
    vertices[v++] = {{x, -half_height, z}, {0, -1, 0}, {r, g, b}};
  }

  for (s32 i = 0; i < sectors; i++)
  {
    indices[idx++] = bottom_center;
    indices[idx++] = bottom_center + i + 2;
    indices[idx++] = bottom_center + i + 1;
  }

  Mesh *mesh = push_struct(arena, Mesh);
  mesh->create(arena, vertices, v, indices, idx, gfx);
  return mesh;
}

// ========== Cone ==========
Mesh *Mesh::create_cone(Arena *arena, GraphicsAPI *gfx, r32 radius, r32 height, s32 sectors,
                       r32 r, r32 g, r32 b)
{
  // Apex (1) + base circle (sectors+1) + bottom cap center (1) + bottom circle (sectors+1)
  s32 vert_count = 1 + (sectors + 1) + 1 + (sectors + 1);
  s32 ind_count = sectors * 3 + sectors * 3; // side + bottom

  Vertex *vertices = push_array(arena, Vertex, vert_count);
  u32 *indices = push_array(arena, u32, ind_count);

  const r32 PI = 3.14159265359f;
  r32 half_height = height / 2.0f;
  s32 v = 0;
  s32 idx = 0;

  // Apex
  u32 apex = v;
  vertices[v++] = {{0, half_height, 0}, {0, 1, 0}, {r, g, b}};

  // Base circle
  for (s32 i = 0; i <= sectors; i++)
  {
    r32 theta = 2.0f * PI * i / sectors;
    r32 x = radius * cosf(theta);
    r32 z = radius * sinf(theta);

    r32 slant = sqrtf(radius * radius + height * height);
    r32 nx = (height * x) / (radius * slant);
    r32 ny = radius / slant;
    r32 nz = (height * z) / (radius * slant);

    vertices[v++] = {{x, -half_height, z}, {nx, ny, nz}, {r, g, b}};
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
  vertices[v++] = {{0, -half_height, 0}, {0, -1, 0}, {r, g, b}};

  for (s32 i = 0; i <= sectors; i++)
  {
    r32 theta = 2.0f * PI * i / sectors;
    r32 x = radius * cosf(theta);
    r32 z = radius * sinf(theta);
    vertices[v++] = {{x, -half_height, z}, {0, -1, 0}, {r, g, b}};
  }

  for (s32 i = 0; i < sectors; i++)
  {
    indices[idx++] = bottom_center;
    indices[idx++] = bottom_center + i + 2;
    indices[idx++] = bottom_center + i + 1;
  }

  Mesh *mesh = push_struct(arena, Mesh);
  mesh->create(arena, vertices, v, indices, idx, gfx);
  return mesh;
}
