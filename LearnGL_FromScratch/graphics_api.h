#ifndef GRAPHICS_API_H
#define GRAPHICS_API_H

#include <stdint.h>
#include <stddef.h> 

struct GLFWwindow;

typedef void *GraphicsBuffer;
typedef void *GraphicsShader;
typedef void *GraphicsProgram;
typedef void *GraphicsVertexArray;

enum ShaderType
{
  SHADER_TYPE_VERTEX,
  SHADER_TYPE_FRAGMENT
};

struct GraphicsAPI
{
  void (*set_window_hints)();

  bool (*init)(GLFWwindow *window);
  void (*shutdown)();

  GraphicsBuffer (*create_buffer)(const void *data, size_t size);
  GraphicsShader (*create_shader)(ShaderType type, const char *source);
  GraphicsProgram (*create_program)(GraphicsShader vertex, GraphicsShader fragment);
  GraphicsVertexArray (*create_vertex_array)();

  GraphicsBuffer (*create_index_buffer)(const void *data, size_t size);
  void (*bind_index_buffer)(GraphicsBuffer buffer);
  void (*draw_elements)(int count);

  void (*set_uniform_mat4)(GraphicsProgram program, int location, const float *data);
  void (*set_uniform_vec3)(GraphicsProgram program, int location, const float *data);

  void (*bind_buffer)(GraphicsBuffer buffer);
  void (*bind_vertex_array)(GraphicsVertexArray vao);
  void (*use_program)(GraphicsProgram program);

  int (*get_attrib_location)(GraphicsProgram program, const char *name);
  int (*get_uniform_location)(GraphicsProgram program, const char *name);
  void (*enable_vertex_attrib)(int location);
  void (*vertex_attrib_pointer)(int location, int size, int stride, size_t offset);

  void (*clear)(float r, float g, float b, float a);
  void (*viewport)(int x, int y, int width, int height);
  void (*draw_arrays)(int first, int count);

  void (*swap_buffers)(GLFWwindow *window);

  void (*destroy_buffer)(GraphicsBuffer buffer);
  void (*destroy_shader)(GraphicsShader shader);
  void (*destroy_program)(GraphicsProgram program);
  void (*destroy_vertex_array)(GraphicsVertexArray vao);
};

GraphicsAPI *create_graphics_api_opengl();
GraphicsAPI *create_graphics_api_vulkan();
GraphicsAPI *create_graphics_api_metal();
GraphicsAPI *create_graphics_api_dx12();

#endif
