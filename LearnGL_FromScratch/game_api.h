#ifndef GAME_API_H
#define GAME_API_H

#include "arena2.h"
#include "linmath.h"
#include "mesh.h"
#include <time.h>

struct GraphicsAPI;

typedef struct GameMemory
{
  Arena *arena;
  GraphicsAPI *gfx;
  Mesh *objects;
  u32 objects_count = 0;
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

  void (*init)(GameMemory *);
  void (*update)(GameMemory *, float);
  void (*render)(GameMemory *, RenderContext *);
  void (*hot_reloaded)(GameMemory *);
  void (*shutdown)(GameMemory *);
} GameAPI;

#endif
