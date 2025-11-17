#pragma once
struct Vertex
{
  float position[3];
  float normal[3];
  float color[3];
};

class Mesh
{
public:
  GLuint vao = 0;
  GLuint vbo = 0;
  GLuint ebo = 0;
  int index_count = 0;

  Mesh() = default;

  ~Mesh()
  {
    if (vao)
      glDeleteVertexArrays(1, &vao);
    if (vbo)
      glDeleteBuffers(1, &vbo);
    if (ebo)
      glDeleteBuffers(1, &ebo);
  }

  void create(const std::vector<Vertex> &vertices, const std::vector<unsigned int> &indices)
  {
    index_count = indices.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // Color
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
  }

  void draw() const
  {
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }

  // Factory methods for different shapes

  static Mesh create_ground(float size, float r = 0.2f, float g = 0.3f, float b = 0.2f)
  {
    std::vector<Vertex> vertices = {
        {{-size, 0, -size}, {0, 1, 0}, {r, g, b}},
        {{size, 0, -size}, {0, 1, 0}, {r, g, b}},
        {{size, 0, size}, {0, 1, 0}, {r, g, b}},
        {{-size, 0, size}, {0, 1, 0}, {r, g, b}},
    };

    std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};

    Mesh mesh;
    mesh.create(vertices, indices);
    return mesh;
  }

  static Mesh create_box(float w, float h, float d, float r = 0.8f, float g = 0.2f, float b = 0.2f)
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
    mesh.create(vertices, indices);
    return mesh;
  }

  static Mesh create_sphere(float radius, int sectors = 36, int stacks = 18,
                            float r = 0.2f, float g = 0.2f, float b = 0.8f)
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
    mesh.create(vertices, indices);
    return mesh;
  }

  static Mesh create_cylinder(float radius, float height, int sectors = 36,
                              float r = 0.8f, float g = 0.8f, float b = 0.2f)
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

      // Top
      vertices.push_back({{x, half_height, z}, {nx, 0, nz}, {r, g, b}});
      // Bottom
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
    mesh.create(vertices, indices);
    return mesh;
  }

  static Mesh create_cone(float radius, float height, int sectors = 36,
                          float r = 0.8f, float g = 0.4f, float b = 0.2f)
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

      // Calculate side normal
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
    mesh.create(vertices, indices);
    return mesh;
  }
};