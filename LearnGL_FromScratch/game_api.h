#ifndef GAME_API_H
#define GAME_API_H

#include "arena2.h"
#include "linmath.h"
#include "mesh.h"
#include "shader.h"
#include <time.h>


struct GraphicsAPI;
struct PhysicsState;
namespace JPH { class BodyID; }

enum class ObjectType
{
  GROUND = 0,
  BOX,
  SPHERE,
  CYLINDER,
  CONE,
};

struct CreateObjectParams
{
  vec3 size;
  vec3 loc = {0, 0, 0};
  vec3 color = {0.8f, 0.8f, 0.8f};
};
typedef struct
{
  Mesh *mesh;
  JPH::BodyID *body_id;
  ObjectType type;
} Object;

typedef struct RenderContext
{
  Shader *shader;
  Object *objects;
  u32 objects_count = 0;
} RenderContext;

typedef struct GameMemory
{
  Arena *arena;
  GraphicsAPI *gfx;
  RenderContext *render_contexts;
  u32 render_context_count;
  s32 width, height;
  vec3 camera;
  r32 yaw, pitch;
  PhysicsState *physics;
} GameMemory;

typedef struct GameButtonState
{
    bool down;
} GameButtonState;

typedef struct GameControllerInput
{
  union
  {
    GameButtonState Buttons[12];
    struct
    {
      GameButtonState move_up;
      GameButtonState move_down;
      GameButtonState move_left;
      GameButtonState move_right;

      GameButtonState action_down;
      GameButtonState action_right;
      GameButtonState action_left;
      GameButtonState action_up;

      GameButtonState left_shoulder;
      GameButtonState right_shoulder;

      GameButtonState start;
      GameButtonState back;
    };
  };

} GameControllerInput;

typedef struct GameInput
{
  GameButtonState mouse_buttons[3];
  GameControllerInput controllers;
  r64 mouse_x, mouse_y, mouse_z;
  r32 deltat_for_frame;
} GameInput;

typedef struct GameAPI
{
  void *dll_handle;
  time_t dll_timestamp;

  void (*init)(GameMemory *);
  void (*update)(GameMemory *, GameInput *);
  void (*render)(GameMemory *);
  void (*hot_reloaded)(GameMemory *);
  void (*shutdown)(GameMemory *);
} GameAPI;

#endif
