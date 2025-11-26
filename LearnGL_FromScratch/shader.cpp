#include "shader.h"
#include <stdio.h>
#include <stdlib.h>

static char *read_file_to_arena(Arena *arena, const char *filepath)
{
  FILE *file = fopen(filepath, "rb");
  if (!file)
  {
    fprintf(stderr, "Failed to open shader file: %s\n", filepath);
    return nullptr;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = (char *)push_array(arena, char, size + 1);
  fread(buffer, 1, size, file);
  buffer[size] = '\0';
  fclose(file);

  return buffer;
}

void Shader::create(Arena *arena, const char *vertex_path, const char *fragment_path, GraphicsAPI *gfx)
{
  Temp temp = temp_begin(arena);
  vertex_source = read_file_to_arena(temp.arena, vertex_path);
  fragment_source = read_file_to_arena(temp.arena, fragment_path);
  

  if (!vertex_source || !fragment_source)
  {
    is_loaded = false;
    temp_end(temp);
    return;
  }
  GraphicsShader vertex_shader = gfx->create_shader(temp.arena, SHADER_TYPE_VERTEX, vertex_source);
  GraphicsShader fragment_shader = gfx->create_shader(temp.arena, SHADER_TYPE_FRAGMENT, fragment_source);
  temp_end(temp);

  program = gfx->create_program(arena, vertex_shader, fragment_shader);
  is_loaded = (program != nullptr);
}

void Shader::use(GraphicsAPI *gfx) const
{
  if (is_loaded)
  {
    gfx->use_program(program);
  }
}

void Shader::destroy(GraphicsAPI *gfx)
{
  if (is_loaded)
  {
    gfx->destroy_shader(program);
    program = nullptr;
    is_loaded = false;
  }
}

void Shader::set_int(GraphicsAPI *gfx, const char *name, s32 value) const
{
  gfx->set_int(program, name, value);
}

void Shader::set_float(GraphicsAPI *gfx, const char *name, r32 value) const
{
  gfx->set_float(program, name, value);
}

void Shader::set_vec3(GraphicsAPI *gfx, const char *name, r32 *data) const
{
  gfx->set_vec3(program, name, data);
}
void Shader::set_vec3(GraphicsAPI *gfx, const char *name, r32 x, r32 y, r32 z) const
{
  vec3 data{x, y, z};
  set_vec3(gfx, name, data);
}

void Shader::set_vec4(GraphicsAPI *gfx, const char *name, r32 *data) const
{
  gfx->set_vec4(program, name, data);
}

void Shader::set_vec4(GraphicsAPI *gfx, const char *name, r32 x, r32 y, r32 z, r32 w) const
{
  vec4 data{x, y, z, w};
  gfx->set_vec4(gfx, name, data);
}

void Shader::set_mat4(GraphicsAPI *gfx, const char *name, const r32 *mat) const
{
  gfx->set_mat4(program, name, mat);
}

Shader Shader::create_basic(Arena *arena, GraphicsAPI *gfx)
{
  Shader shader;
  shader.create(arena, "shaders/basic.vert", "shaders/basic.frag", gfx);
  return shader;
}

Shader Shader::create_lit(Arena *arena, GraphicsAPI *gfx)
{
  Shader shader;
  shader.create(arena, "shaders/lit.vert", "shaders/lit.frag", gfx);
  return shader;
}
