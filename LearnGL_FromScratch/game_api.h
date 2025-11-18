#pragma once

namespace arena
{
  class MemoryArena;
}

#include "linmath.h"
#include <GLFW/glfw3.h>

struct GameMemory
{
  float rotation_speed;
  mat4x4 model_matrix;
  double last_change_time;

  // Store arena pointer but DON'T own it
  // The host owns the arena, we just use it
  arena::MemoryArena *arena;
};

struct GameAPI
{
  void (*init)(GameMemory *memory, arena::MemoryArena *arena);
  void (*update)(GameMemory *memory, float delta_time);
  void (*render)(GameMemory *memory, GLuint program, GLint mvp_location, GLuint vertex_array);
  void (*hot_reloaded)(GameMemory *memory);
  void (*shutdown)(GameMemory *memory);

  void *dll_handle;
  time_t dll_timestamp;
};
