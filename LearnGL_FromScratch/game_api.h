#ifndef GAME_API_H
#define GAME_API_H

#include "arena2.h"
#include "linmath.h"
#include "mesh.h"
#include "shader.h"
#include <time.h>

struct GraphicsAPI;

typedef struct RenderContext
{
  Shader *shader;
  Mesh *objects;
  u32 objects_count = 0;
} RenderContext;

typedef struct GameMemory
{
  Arena *arena;
  GraphicsAPI *gfx;
  RenderContext *render_contexts;
  u32 render_context_count = 0;
  s32 width, height;
} GameMemory;


typedef struct GameAPI
{
  void *dll_handle;
  time_t dll_timestamp;

  void (*init)(GameMemory *);
  void (*update)(GameMemory *, float);
  void (*render)(GameMemory *);
  void (*hot_reloaded)(GameMemory *);
  void (*shutdown)(GameMemory *);
} GameAPI;

#endif
