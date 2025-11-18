#include "game_api.h"
#include "graphics_api.h"
#include "arena.h"
#include "linmath.h"
#include <stdio.h>

extern "C"
{

  void game_init(GameMemory *memory, arena::MemoryArena *arena)
  {
    printf("Game initialized\n");

    memory->arena = arena;
    memory->rotation_speed = 1.0f;
    memory->last_change_time = 0.0;
    mat4x4_identity(memory->model_matrix);
  }

  void game_update(GameMemory *memory, float delta_time)
  {
    static double accumulated_time = 0.0;
    accumulated_time += delta_time;

    temp_arena temp = begin_temp_memory(1024 * 1024);
    float *temp_calculations = push_temp_array(temp, 100, float);

    memory->rotation_speed = (float)accumulated_time;

    mat4x4_identity(memory->model_matrix);
    mat4x4_rotate_Z(memory->model_matrix, memory->model_matrix, memory->rotation_speed);

    end_temp_memory(temp);
  }

  void game_render(GameMemory *memory, RenderContext *ctx)
  {
    GraphicsAPI *gfx = ctx->gfx;

    gfx->use_program((GraphicsProgram)ctx->program);
    gfx->set_uniform_mat4((GraphicsProgram)ctx->program, ctx->mvp_location,
                          (const float *)&memory->model_matrix);
    gfx->bind_vertex_array((GraphicsVertexArray)ctx->vertex_array);
    gfx->draw_arrays(0, 3);
  }

  void game_hot_reloaded(GameMemory *memory)
  {
    printf("===== GAME CODE HOT RELOADED =====\n");
    printf("Arena pointer: %p\n", memory->arena);
    printf("Rotation speed preserved: %f\n", memory->rotation_speed);
  }

  void game_shutdown(GameMemory *memory)
  {
    printf("Game shutdown\n");
  }

} // extern "C"
