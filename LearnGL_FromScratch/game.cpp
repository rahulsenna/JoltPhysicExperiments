#include "game_api.h"
#include "graphics_api.h"
#include "arena2.h"
#include "linmath.h"
#include "mesh.h"
#include "shader.h"
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
    mat4x4_rotate_Z(temp.arena, model, model, accumulated_time);

    temp_end(temp);
  }

  void game_render(GameMemory *memory, RenderContext *ctx)
  {
    GraphicsAPI *gfx = memory->gfx;
    Shader *shader = ctx->shader;

    shader->use(gfx);

    vec3 camera = {0.040f, 2.5f, -10.0f};
    vec3 light_pos = {8.0f, 5.0f, 8.0f};
    vec3 center = {0.0f, 0.0f, 0.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};
    float target_x = 0.0f, target_y = 0.0f, target_z = 0.0f;

    mat4x4 perspective = {};
    mat4x4_perspective(perspective, 1.047f, (float)ctx->width / ctx->height, 0.1f, 100.0f);
    mat4x4 view = {};
    mat4x4_look_at(view, camera, center, up);
    
    shader->set_mat4(gfx, "view", (const float *)view);
    shader->set_mat4(gfx, "projection", (const float *)perspective);
    shader->set_vec3(gfx, "light_pos", light_pos);
    shader->set_vec3(gfx, "view_pos", camera);

    for (int i = 0; i < memory->objects_count; ++i)
    {
      shader->set_mat4(gfx, "model", (const float *)memory->objects[i].model);
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
