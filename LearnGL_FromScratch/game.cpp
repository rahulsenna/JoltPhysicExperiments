#include "game_api.h"
#include "graphics_api.h"
#include "arena2.h"
#include "linmath.h"
#include "mesh.h"
#include <stdio.h>

extern "C"
{
  void game_init(GameMemory *memory)
  {
    GraphicsAPI *gfx = memory->gfx;
    Arena *arena = memory->arena;

    memory->objects = push_array(arena, Mesh, 5);
    memory->objects_count = 0;
    memory->objects[memory->objects_count++] = Mesh::create_ground(arena, gfx, 50.0f);
    memory->objects[memory->objects_count++] = Mesh::create_box(arena, gfx, 1.0f, 1.0f, 1.0f);
    memory->objects[memory->objects_count++] = Mesh::create_sphere(arena, gfx, 1.0f, 36, 18);
    memory->objects[memory->objects_count++] = Mesh::create_cone(arena, gfx, 1.0f, 2);
    memory->objects[memory->objects_count++] = Mesh::create_cylinder(arena, gfx, 1.0f, 2);

    memory->objects[1].translate(-3.0f, 0.5f, 0.0f);
    memory->objects[2].translate(.0f, 1.5f, 0.0f);
    memory->objects[3].translate(3.0f, 0.5f, 0.0f);
    memory->objects[4].translate(-6.0f, 0.5f, 0.0f);

    printf("Game initialized\n");
  }

  void game_update(GameMemory *memory, float delta_time)
  {
    static double accumulated_time = 0.0;
    accumulated_time += delta_time;

    Temp temp = temp_begin(memory->arena);
    float *temp_calculations = push_array(temp.arena, float, 100);

    mat4x4 model;
    mat4x4_identity(model);
    mat4x4_rotate_Z(memory->arena, model, model, accumulated_time);

    temp_end(temp);
  }

  void game_render(GameMemory *memory, RenderContext *ctx)
  {
    GraphicsAPI *gfx = ctx->gfx;
    GraphicsProgram program = ctx->program;

    gfx->use_program(program);
    int model_loc = gfx->get_uniform_location(program, "model");
    int view_loc = gfx->get_uniform_location(program, "view");
    int proj_loc = gfx->get_uniform_location(program, "projection");
    int light_loc = gfx->get_uniform_location(program, "light_pos");
    int view_pos_loc = gfx->get_uniform_location(program, "view_pos");

    vec3 camera = {0.040f, 2.5f, -10.0f};
    vec3 light_pos = {8.0f, 5.0f, 8.0f};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};
    float target_x = 0.0f, target_y = 0.0f, target_z = 0.0f;

    mat4x4 perspective = {};
    mat4x4_perspective(perspective, 1.047f, (float)ctx->width / ctx->height, 0.1f, 100.0f);
    mat4x4 view = {};
    mat4x4_look_at(view, camera, center, up);

    gfx->set_uniform_mat4(program, view_loc, (const float *)view);
    gfx->set_uniform_mat4(program, proj_loc, (const float *)perspective);

    gfx->set_uniform_vec3(program, light_loc, light_pos);
    gfx->set_uniform_vec3(program, view_pos_loc, camera);

    //  mat4x4_translate(model, x, y, z);             // Position
    //  mat4x4_rotate_Y(model, model, angle);         // Rotation
    //  mat4x4_scale_aniso(model, model, sx, sy, sz); // Scale

    for (int i = 0; i < memory->objects_count; ++i)
    {
      gfx->set_uniform_mat4(ctx->program, model_loc, (const float *)memory->objects[i].model);
      memory->objects[i].draw(gfx);
    }
  }

  void game_hot_reloaded(GameMemory *memory)
  {
    printf("===== GAME CODE HOT RELOADED =====\n");
  }

  void game_shutdown(GameMemory *memory)
  {
    printf("Game shutdown\n");
  }

} // extern "C"
