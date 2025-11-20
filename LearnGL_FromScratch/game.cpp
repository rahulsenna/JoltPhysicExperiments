#include "game_api.h"
#include "graphics_api.h"
#include "arena.h"
#include "linmath.h"
#include "mesh.h"
#include <stdio.h>

extern "C"
{
  Mesh ground, box, sphere, cone, cylinder;
  
  void game_init(GameMemory *memory, arena::MemoryArena *arena, GraphicsAPI *gfx)
  {
    printf("Game initialized\n");

    memory->arena = arena;
    memory->rotation_speed = 1.0f;
    memory->last_change_time = 0.0;
    mat4x4_identity(memory->model_matrix);

    ground = Mesh::create_ground(gfx, 50.0f);
    box = Mesh::create_box(gfx, 1.0f, 1.0f, 1.0f);
    sphere = Mesh::create_sphere(gfx, 1.0f, 36, 18);
    cone = Mesh::create_cone(gfx, 1.0f, 2);
    cylinder = Mesh::create_cylinder(gfx, 1.0f, 2);
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

    gfx->set_uniform_mat4(program, view_loc, (const float*)view);
    gfx->set_uniform_mat4(program, proj_loc, (const float*)perspective);
    
    gfx->set_uniform_vec3(program, light_loc, light_pos);
    gfx->set_uniform_vec3(program, view_pos_loc, camera);

    //  mat4x4_translate(model, x, y, z);             // Position
    //  mat4x4_rotate_Y(model, model, angle);         // Rotation
    //  mat4x4_scale_aniso(model, model, sx, sy, sz); // Scale

    mat4x4 ground_model;
    mat4x4_identity(ground_model);
    gfx->set_uniform_mat4(ctx->program, model_loc, (const float *)ground_model);
    ground.draw(gfx);

    mat4x4 box_model;
    mat4x4_identity(box_model);
    mat4x4_translate(box_model, -3.0f, 0.5f, 0.0f);
    gfx->set_uniform_mat4(ctx->program, model_loc, (const float *)box_model);
    box.draw(gfx);

    mat4x4 sphere_model;
    mat4x4_identity(sphere_model);
    mat4x4_translate(sphere_model, .0f, 1.5f, 0.0f);
    gfx->set_uniform_mat4(ctx->program, model_loc, (const float *)sphere_model);
    sphere.draw(gfx);

    mat4x4 cone_model;
    mat4x4_identity(cone_model);
    mat4x4_translate(cone_model, 3.0f, 0.5f, 0.0f);
    gfx->set_uniform_mat4(ctx->program, model_loc, (const float *)cone_model);
    cone.draw(gfx);

    mat4x4 cylinder_model;
    mat4x4_identity(cylinder_model);
    mat4x4_translate(cylinder_model, -6.0f, 0.5f, 0.0f);
    gfx->set_uniform_mat4(ctx->program, model_loc, (const float *)cylinder_model);
    cylinder.draw(gfx);
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
