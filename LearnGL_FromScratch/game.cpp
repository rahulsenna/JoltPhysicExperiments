#include "game_api.h"
#include "graphics_api.h"
#include "arena2.h"
#include "linmath.h"
#include "mesh.h"
#include "shader.h"
#include <stdio.h>
#include <assert.h>
#include "physics.cpp"

extern "C"
{
  void *arena_push(Arena *arena, u64 size, u64 align, b32 zero) __attribute__((weak));
  Temp temp_begin(Arena *arena) __attribute__((weak));
  void temp_end(Temp temp) __attribute__((weak));


  void *arena_push(Arena *arena, u64 size, u64 align, b32 zero) {}
  Temp temp_begin(Arena *arena) {};
  void temp_end(Temp temp) {};
}

static void compute_camera_basis(r32 yaw, r32 pitch, vec3 forward, vec3 right)
{
  r32 ryaw = yaw * DEG2RAD;
  r32 rpitch = pitch * DEG2RAD;

  forward[0] = cosf(ryaw) * cosf(rpitch);
  forward[1] = sinf(rpitch);
  forward[2] = sinf(ryaw) * cosf(rpitch);

  vec3_norm(forward, forward);

  vec3 up = {0.0f, 1.0f, 0.0f};

  vec3_mul_cross(right, forward, up);
  vec3_norm(right, right);
}

void create_object(GameMemory *memory, JPH::BodyInterface &body_interface, Object *object, ObjectType type, CreateObjectParams params)
{
  GraphicsAPI *gfx = memory->gfx;
  Arena *arena = memory->arena;

  JPH::Vec3 jolt_pos(params.loc[0], params.loc[1], params.loc[2]);
  JPH::EMotionType motion = (type == ObjectType::GROUND) ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic;
  JPH::ObjectLayer layer = (type == ObjectType::GROUND) ? Layers::NON_MOVING : Layers::MOVING;
  JPH::EActivation activation = (type == ObjectType::GROUND) ? JPH::EActivation::DontActivate : JPH::EActivation::Activate;

  JPH::ShapeSettings::ShapeResult shape_result;

  switch (type)
  {
  case ObjectType::GROUND:
  {
    r32 half_size = params.size[0] * 0.5f;
    object->mesh = Mesh::create_ground(arena, gfx, params.size[0], params.color[0], params.color[1], params.color[2]);
    // object->mesh = Mesh::create_box(arena, gfx, half_size, 0.1f, half_size, params.color[0], params.color[1], params.color[2]);

    JPH::BoxShapeSettings shape(JPH::Vec3(half_size, 0.1f, half_size));
    shape_result = shape.Create();
    jolt_pos = JPH::Vec3(0, -0.1f, 0);
    break;
  }
  case ObjectType::BOX:
  {
    object->mesh = Mesh::create_box(arena, gfx, params.size[0], params.size[1], params.size[2], params.color[0], params.color[1], params.color[2]);
    object->mesh->translate(params.loc[0], params.loc[1], params.loc[2]);

    JPH::BoxShapeSettings shape(JPH::Vec3(params.size[0], params.size[1], params.size[2]));
    shape_result = shape.Create();
    break;
  }
  case ObjectType::SPHERE:
  {
    object->mesh = Mesh::create_sphere(arena, gfx, params.size[0], 36, 18,
                                       params.color[0], params.color[1], params.color[2]);
    object->mesh->translate(params.loc[0], params.loc[1], params.loc[2]);

    JPH::SphereShapeSettings shape(params.size[0]);
    shape_result = shape.Create();
    break;
  }
  case ObjectType::CYLINDER:
  {
    object->mesh = Mesh::create_cylinder(arena, gfx, params.size[0], params.size[1], 36,
                                         params.color[0], params.color[1], params.color[2]);
    object->mesh->translate(params.loc[0], params.loc[1], params.loc[2]);

    JPH::CylinderShapeSettings shape(params.size[1] * 0.5f, params.size[0]);
    shape_result = shape.Create();
    break;
  }
  case ObjectType::CONE:
  {
    object->mesh = Mesh::create_cone(arena, gfx, params.size[0], params.size[1], 36,
                                     params.color[0], params.color[1], params.color[2]);
    object->mesh->translate(params.loc[0], params.loc[1], params.loc[2]);

    // Approximate cone as cylinder (Jolt doesn't have native cone)
    JPH::CylinderShapeSettings shape(params.size[1] * 0.5f, params.size[0]);
    shape_result = shape.Create();
    break;
  }
  }

  JPH::BodyCreationSettings body_settings(shape_result.Get(), jolt_pos, JPH::Quat::sIdentity(), motion, layer);
  object->body_id = push_struct(arena, JPH::BodyID);
  *object->body_id = body_interface.CreateAndAddBody(body_settings, activation);
  object->type = type;
}

extern "C"
{
  void game_init(GameMemory *memory)
  {
    GraphicsAPI *gfx = memory->gfx;
    Arena *arena = memory->arena;

    memory->camera[0] = 0.f;
    memory->camera[1] = 2.5f;
    memory->camera[2] = -10.0f;

    memory->yaw = 90.0f;
    memory->pitch = 0.0f;

    init_physics(memory);

    memory->render_context_count = 1;
    s32 r_idx = 0;
    memory->render_contexts = push_array(arena, RenderContext, memory->render_context_count);
    RenderContext *ctx = (RenderContext *)&memory->render_contexts[r_idx++];

    ctx->shader = Shader::create_basic(arena, gfx);

    ctx->objects_count = 5;
    s32 o_idx = 0;
    ctx->objects = push_array(arena, Object, ctx->objects_count);

    JPH::BodyInterface &body_interface = memory->physics->physics_system->GetBodyInterface();

    create_object(memory, body_interface, &ctx->objects[o_idx++], ObjectType::GROUND, {{50.0f}, {}, {.2, .3, .2}});
    create_object(memory, body_interface, &ctx->objects[o_idx++], ObjectType::BOX, {{1.0f, 1.0f, 1.0f}, {.0f, 10, 0.0f}, {.8, .2, .2}});
    create_object(memory, body_interface, &ctx->objects[o_idx++], ObjectType::SPHERE, {{1.0f}, {0.0f, 10, 0.0f}, {.2, .2, .8}});
    create_object(memory, body_interface, &ctx->objects[o_idx++], ObjectType::CONE, {{1.0f, 2}, {0, 10.f, 0.0f}, {.8, .4, .2}});
    create_object(memory, body_interface, &ctx->objects[o_idx++], ObjectType::CYLINDER, {{1.0f, 2}, {0, 10.f, 0.0f}, {.8, .8, .2}});

    memory->physics->physics_system->OptimizeBroadPhase();

    assert(o_idx <= ctx->objects_count && "objects_count MISMATCH");
    assert(r_idx == memory->render_context_count && "render_context_count MISMATCH");
    printf("Game initialized\n");
  }

  void game_update(GameMemory *memory, GameInput *input)
  {
    r32 dt = input->deltat_for_frame;
    r32 speed = 5.0f;

    const r32 sensitivity = 0.12f;

    r32 xoffset = (r32)input->mouse_x * sensitivity;
    r32 yoffset = (r32)input->mouse_y * sensitivity;

    memory->yaw += xoffset;
    memory->pitch -= yoffset;

    // clamp pitch
    if (memory->pitch > 89.0f)
      memory->pitch = 89.0f;
    if (memory->pitch < -89.0f)
      memory->pitch = -89.0f;

    vec3 forward;
    vec3 right;
    compute_camera_basis(memory->yaw, memory->pitch, forward, right);

    if (input->controllers.move_up.down)
    {
      memory->camera[0] += forward[0] * speed * dt;
      memory->camera[1] += forward[1] * speed * dt;
      memory->camera[2] += forward[2] * speed * dt;
    }
    if (input->controllers.move_down.down)
    {
      memory->camera[0] -= forward[0] * speed * dt;
      memory->camera[1] -= forward[1] * speed * dt;
      memory->camera[2] -= forward[2] * speed * dt;
    }
    if (input->controllers.move_right.down)
    {
      memory->camera[0] += right[0] * speed * dt;
      memory->camera[1] += right[1] * speed * dt;
      memory->camera[2] += right[2] * speed * dt;
    }
    if (input->controllers.move_left.down)
    {
      memory->camera[0] -= right[0] * speed * dt;
      memory->camera[1] -= right[1] * speed * dt;
      memory->camera[2] -= right[2] * speed * dt;
    }

    memory->physics->temp_allocator->Clear();
    // Update physics (1 collision step)
    memory->physics->physics_system->Update(dt, 1, memory->physics->temp_allocator, memory->physics->job_system);

    // Sync physics bodies to render meshes
    JPH::BodyInterface &body_interface = memory->physics->physics_system->GetBodyInterface();

    for (int i = 0; i < memory->render_context_count; ++i)
    {
      RenderContext *ctx = &memory->render_contexts[i];
      for (int j = 0; j < ctx->objects_count; ++j)
      {
        JPH::Vec3 position = body_interface.GetPosition(*ctx->objects[j].body_id);
        JPH::Quat rotation = body_interface.GetRotation(*ctx->objects[j].body_id);

        quat q = {rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW()};

        // Update mesh transform - dereference the pointer
        mat4x4_from_quat(*ctx->objects[j].mesh->model, q);
        (*ctx->objects[j].mesh->model)[3][0] = position.GetX();
        (*ctx->objects[j].mesh->model)[3][1] = position.GetY();
        (*ctx->objects[j].mesh->model)[3][2] = position.GetZ();
      }
    }

  }

  void game_render(GameMemory *memory)
  {
    GraphicsAPI *gfx = memory->gfx;

    vec3 forward, right;
    compute_camera_basis(memory->yaw, memory->pitch, forward, right);

    vec3 target = {
        memory->camera[0] + forward[0],
        memory->camera[1] + forward[1],
        memory->camera[2] + forward[2],
    };

    vec3 light_pos = {8.0f, 5.0f, 8.0f};
    vec3 up = {0.0f, 1.0f, 0.0f};

    mat4x4 view = {};
    mat4x4_look_at(view, memory->camera, target, up);

    mat4x4 perspective = {};
    mat4x4_perspective(perspective, 1.047f, (r32)memory->width / memory->height, 0.1f, 100.0f);

    for (int i = 0; i < memory->render_context_count; ++i)
    {
      RenderContext *ctx = &memory->render_contexts[i];
      Shader *shader = ctx->shader;

      shader->use(gfx);

      shader->set_mat4(gfx, "view", (const r32 *)view);
      shader->set_mat4(gfx, "projection", (const r32 *)perspective);
      shader->set_vec3(gfx, "light_pos", light_pos);
      shader->set_vec3(gfx, "view_pos", memory->camera);

      for (int j = 0; j < ctx->objects_count; ++j)
      {
        shader->set_mat4(gfx, "model", (const r32 *)ctx->objects[j].mesh->model);
        ctx->objects[j].mesh->draw(gfx);
      }
    }
    if (memory->physics->debug_draw_enabled)
      draw_physics(memory, view, perspective);
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
