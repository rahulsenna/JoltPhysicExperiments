#ifndef GAME_API_H
#define GAME_API_H

#include "arena.h"
#include "linmath.h"

struct GraphicsAPI;

typedef struct GameMemory
{
  arena::MemoryArena *arena;
  float rotation_speed;
  double last_change_time;
  mat4x4 model_matrix;
} GameMemory;

typedef struct RenderContext
{
  void *program;
  void *vertex_array;
  int mvp_location;
  int width;
  int height;
  GraphicsAPI *gfx;
} RenderContext;

typedef struct GameAPI
{
  void *dll_handle;
  time_t dll_timestamp;

  void (*init)(GameMemory *, arena::MemoryArena *);
  void (*update)(GameMemory *, float);
  void (*render)(GameMemory *, RenderContext *);
  void (*hot_reloaded)(GameMemory *);
  void (*shutdown)(GameMemory *);
} GameAPI;

#endif
