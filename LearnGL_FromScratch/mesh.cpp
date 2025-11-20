#include "mesh.h"
#include <cmath>

void Mesh::create(const std::vector<Vertex> &verts, const std::vector<unsigned int> &inds, GraphicsAPI *gfx)
{
  vertices = verts;
  indices = inds;
  index_count = indices.size();

  // Create vertex buffer
  vbo = gfx->create_buffer(vertices.data(), vertices.size() * sizeof(Vertex));

  // Create index buffer
  ebo = gfx->create_index_buffer(indices.data(), indices.size() * sizeof(unsigned int));

  // Create and setup VAO
  vao = gfx->create_vertex_array();
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

  vertices.clear();
  indices.clear();
}

// ========== Ground ==========
Mesh Mesh::create_ground(GraphicsAPI *gfx, float size, float r, float g, float b)
{
  std::vector<Vertex> vertices = {
      {{-size, 0, -size}, {0, 1, 0}, {r, g, b}},
      {{size, 0, -size}, {0, 1, 0}, {r, g, b}},
      {{size, 0, size}, {0, 1, 0}, {r, g, b}},
      {{-size, 0, size}, {0, 1, 0}, {r, g, b}},
  };

  std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};

  Mesh mesh;
  mesh.create(vertices, indices, gfx);
  return mesh;
}

// ========== Box ==========
Mesh Mesh::create_box(GraphicsAPI *gfx, float w, float h, float d, float r, float g, float b)
{
  std::vector<Vertex> vertices;

  // Front face
  vertices.push_back({{-w, -h, d}, {0, 0, 1}, {r, g, b}});
  vertices.push_back({{w, -h, d}, {0, 0, 1}, {r, g, b}});
  vertices.push_back({{w, h, d}, {0, 0, 1}, {r, g, b}});
  vertices.push_back({{-w, h, d}, {0, 0, 1}, {r, g, b}});

  // Back face
  vertices.push_back({{w, -h, -d}, {0, 0, -1}, {r, g, b}});
  vertices.push_back({{-w, -h, -d}, {0, 0, -1}, {r, g, b}});
  vertices.push_back({{-w, h, -d}, {0, 0, -1}, {r, g, b}});
  vertices.push_back({{w, h, -d}, {0, 0, -1}, {r, g, b}});

  // Top face
  vertices.push_back({{-w, h, d}, {0, 1, 0}, {r, g, b}});
  vertices.push_back({{w, h, d}, {0, 1, 0}, {r, g, b}});
  vertices.push_back({{w, h, -d}, {0, 1, 0}, {r, g, b}});
  vertices.push_back({{-w, h, -d}, {0, 1, 0}, {r, g, b}});

  // Bottom face
  vertices.push_back({{-w, -h, -d}, {0, -1, 0}, {r, g, b}});
  vertices.push_back({{w, -h, -d}, {0, -1, 0}, {r, g, b}});
  vertices.push_back({{w, -h, d}, {0, -1, 0}, {r, g, b}});
  vertices.push_back({{-w, -h, d}, {0, -1, 0}, {r, g, b}});

  // Right face
  vertices.push_back({{w, -h, d}, {1, 0, 0}, {r, g, b}});
  vertices.push_back({{w, -h, -d}, {1, 0, 0}, {r, g, b}});
  vertices.push_back({{w, h, -d}, {1, 0, 0}, {r, g, b}});
  vertices.push_back({{w, h, d}, {1, 0, 0}, {r, g, b}});

  // Left face
  vertices.push_back({{-w, -h, -d}, {-1, 0, 0}, {r, g, b}});
  vertices.push_back({{-w, -h, d}, {-1, 0, 0}, {r, g, b}});
  vertices.push_back({{-w, h, d}, {-1, 0, 0}, {r, g, b}});
  vertices.push_back({{-w, h, -d}, {-1, 0, 0}, {r, g, b}});

  std::vector<unsigned int> indices;
  for (int i = 0; i < 6; i++)
  {
    unsigned int base = i * 4;
    indices.insert(indices.end(), {base + 0, base + 1, base + 2,
                                   base + 0, base + 2, base + 3});
  }

  Mesh mesh;
  mesh.create(vertices, indices, gfx);
  return mesh;
}

// ========== Sphere ==========
Mesh Mesh::create_sphere(GraphicsAPI *gfx, float radius, int sectors, int stacks,
                         float r, float g, float b)
{
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  const float PI = 3.14159265359f;

  for (int i = 0; i <= stacks; i++)
  {
    float phi = PI * i / stacks;

    for (int j = 0; j <= sectors; j++)
    {
      float theta = 2.0f * PI * j / sectors;

      float x = radius * sinf(phi) * cosf(theta);
      float y = radius * cosf(phi);
      float z = radius * sinf(phi) * sinf(theta);

      float nx = x / radius;
      float ny = y / radius;
      float nz = z / radius;

      vertices.push_back({{x, y, z}, {nx, ny, nz}, {r, g, b}});
    }
  }

  for (int i = 0; i < stacks; i++)
  {
    for (int j = 0; j < sectors; j++)
    {
      unsigned int first = i * (sectors + 1) + j;
      unsigned int second = first + sectors + 1;

      indices.insert(indices.end(), {first, second, first + 1});
      indices.insert(indices.end(), {second, second + 1, first + 1});
    }
  }

  Mesh mesh;
  mesh.create(vertices, indices, gfx);
  return mesh;
}

// ========== Cylinder ==========
Mesh Mesh::create_cylinder(GraphicsAPI *gfx, float radius, float height, int sectors,
                           float r, float g, float b)
{
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  const float PI = 3.14159265359f;
  float half_height = height / 2.0f;

  // Side vertices
  for (int i = 0; i <= sectors; i++)
  {
    float theta = 2.0f * PI * i / sectors;
    float x = radius * cosf(theta);
    float z = radius * sinf(theta);
    float nx = x / radius;
    float nz = z / radius;

    vertices.push_back({{x, half_height, z}, {nx, 0, nz}, {r, g, b}});
    vertices.push_back({{x, -half_height, z}, {nx, 0, nz}, {r, g, b}});
  }

  // Side faces
  for (int i = 0; i < sectors; i++)
  {
    unsigned int base = i * 2;
    indices.insert(indices.end(), {base, base + 2, base + 1,
                                   base + 1, base + 2, base + 3});
  }

  // Top cap
  unsigned int top_center = vertices.size();
  vertices.push_back({{0, half_height, 0}, {0, 1, 0}, {r, g, b}});

  for (int i = 0; i <= sectors; i++)
  {
    float theta = 2.0f * PI * i / sectors;
    float x = radius * cosf(theta);
    float z = radius * sinf(theta);
    vertices.push_back({{x, half_height, z}, {0, 1, 0}, {r, g, b}});
  }

  for (int i = 0; i < sectors; i++)
  {
    indices.insert(indices.end(), {top_center, top_center + i + 1, top_center + i + 2});
  }

  // Bottom cap
  unsigned int bottom_center = vertices.size();
  vertices.push_back({{0, -half_height, 0}, {0, -1, 0}, {r, g, b}});

  for (int i = 0; i <= sectors; i++)
  {
    float theta = 2.0f * PI * i / sectors;
    float x = radius * cosf(theta);
    float z = radius * sinf(theta);
    vertices.push_back({{x, -half_height, z}, {0, -1, 0}, {r, g, b}});
  }

  for (int i = 0; i < sectors; i++)
  {
    indices.insert(indices.end(), {bottom_center, bottom_center + i + 2, bottom_center + i + 1});
  }

  Mesh mesh;
  mesh.create(vertices, indices, gfx);
  return mesh;
}

// ========== Cone ==========
Mesh Mesh::create_cone(GraphicsAPI *gfx, float radius, float height, int sectors,
                       float r, float g, float b)
{
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  const float PI = 3.14159265359f;
  float half_height = height / 2.0f;

  // Apex
  unsigned int apex = vertices.size();
  vertices.push_back({{0, half_height, 0}, {0, 1, 0}, {r, g, b}});

  // Base circle
  for (int i = 0; i <= sectors; i++)
  {
    float theta = 2.0f * PI * i / sectors;
    float x = radius * cosf(theta);
    float z = radius * sinf(theta);

    float slant = sqrtf(radius * radius + height * height);
    float nx = (height * x) / (radius * slant);
    float ny = radius / slant;
    float nz = (height * z) / (radius * slant);

    vertices.push_back({{x, -half_height, z}, {nx, ny, nz}, {r, g, b}});
  }

  // Side faces
  for (int i = 0; i < sectors; i++)
  {
    indices.insert(indices.end(), {apex, apex + i + 1, apex + i + 2});
  }

  // Bottom cap
  unsigned int bottom_center = vertices.size();
  vertices.push_back({{0, -half_height, 0}, {0, -1, 0}, {r, g, b}});

  for (int i = 0; i <= sectors; i++)
  {
    float theta = 2.0f * PI * i / sectors;
    float x = radius * cosf(theta);
    float z = radius * sinf(theta);
    vertices.push_back({{x, -half_height, z}, {0, -1, 0}, {r, g, b}});
  }

  for (int i = 0; i < sectors; i++)
  {
    indices.insert(indices.end(), {bottom_center, bottom_center + i + 2, bottom_center + i + 1});
  }

  Mesh mesh;
  mesh.create(vertices, indices, gfx);
  return mesh;
}
