#ifndef SHADER_H
#define SHADER_H

#include "graphics_api.h"
#include "arena2.h"
#include "linmath.h"

struct Shader
{
  // Opaque GPU handle (backend-agnostic)
  GraphicsProgram program;
  
  b32 is_loaded;

  Shader() : program(nullptr), is_loaded(false) {}

  void create(Arena *arena, const char *vertex_path, const char *fragment_path, GraphicsAPI *gfx);
  void use(GraphicsAPI *gfx) const;
  void destroy(GraphicsAPI *gfx);

  // Uniform setters
  void set_int(GraphicsAPI *gfx, const char *name, s32 value) const;
  void set_float(GraphicsAPI *gfx, const char *name, r32 value) const;
  void set_vec3(GraphicsAPI *gfx, const char *name, r32 x, r32 y, r32 z) const;
  void set_vec4(GraphicsAPI *gfx, const char *name, r32 x, r32 y, r32 z, r32 w) const;
  void set_vec3(GraphicsAPI *gfx, const char *name, r32 *data) const;
  void set_vec4(GraphicsAPI *gfx, const char *name, r32 *data) const;
  void set_mat4(GraphicsAPI *gfx, const char *name, const r32 *mat) const;

  // Static helper to load common shaders
  static Shader *create_basic(Arena *arena, GraphicsAPI *gfx);
  static Shader *create_lit(Arena *arena, GraphicsAPI *gfx);
};

#endif // SHADER_H
