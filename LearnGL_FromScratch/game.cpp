#include "game_api.h"
#include "graphics_api.h"
#include "arena2.h"
#include "linmath.h"
#include "mesh.h"
#include "shader.h"
#include <stdio.h>
#include <assert.h>

extern "C"
{
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

  void game_init(GameMemory *memory)
  {
    GraphicsAPI *gfx = memory->gfx;
    Arena *arena = memory->arena;

    memory->camera[0] = 0.f;
    memory->camera[1] = 2.5f;
    memory->camera[2] = -10.0f;

    memory->yaw = 90.0f;
    memory->pitch = 0.0f;

    // -------------[ Jolt ]----------------
    memory->physics = push_struct(arena, PhysicsState);
    memory->physics->factory_instance = new (push_struct(arena, JPH::Factory)) JPH::Factory();

    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = memory->physics->factory_instance;
    JPH::RegisterTypes();

    memory->physics->temp_allocator = new (push_struct(arena, JoltTempArenaAllocator)) JoltTempArenaAllocator(arena, 10 * 1024 * 1024);
    memory->physics->job_system = new (push_struct(arena, JPH::JobSystemThreadPool))  JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);
    

    memory->physics->broad_phase_layer_interface = new (push_struct(arena, BPLayerInterfaceImpl)) BPLayerInterfaceImpl();
    memory->physics->object_vs_broadphase_filter = new (push_struct(arena, ObjectVsBroadPhaseLayerFilterImpl)) ObjectVsBroadPhaseLayerFilterImpl();
    memory->physics->object_vs_object_filter = new (push_struct(arena, ObjectLayerPairFilterImpl)) ObjectLayerPairFilterImpl();

    // Create physics system
    const u32 max_bodies = 1024;
    const u32 num_body_mutexes = 0; // Auto-detect
    const u32 max_body_pairs = 1024;
    const u32 max_contact_constraints = 1024;

    memory->physics->physics_system = new (push_struct(arena, JPH::PhysicsSystem)) JPH::PhysicsSystem();
    memory->physics->physics_system->Init(max_bodies, num_body_mutexes, max_body_pairs,
                                 max_contact_constraints,
                                 *memory->physics->broad_phase_layer_interface,
                                 *memory->physics->object_vs_broadphase_filter,
                                 *memory->physics->object_vs_object_filter);

    memory->physics->physics_system->SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
    // -------------[ Jolt ]----------------


    memory->render_context_count = 1;
    s32 r_idx = 0;
    memory->render_contexts = push_array(arena, RenderContext, memory->render_context_count);
    RenderContext *ctx = (RenderContext *)&memory->render_contexts[r_idx++];

    ctx->shader = push_struct(arena, Shader);
    *ctx->shader = Shader::create_basic(arena, gfx);

    ctx->objects_count = 5;
    s32 o_idx = 0;
    ctx->objects = push_array(arena, Object, ctx->objects_count);

    JPH::BodyInterface &body_interface = memory->physics->physics_system->GetBodyInterface();

    // Ground (static)
    ctx->objects[o_idx].mesh = Mesh::create_ground(arena, gfx, 50.0f);
    {
      JPH::BoxShapeSettings ground_shape(JPH::Vec3(25.0f, 0.1f, 25.0f));
      JPH::ShapeSettings::ShapeResult shape_result = ground_shape.Create();
      JPH::BodyCreationSettings ground_settings(shape_result.Get(), JPH::Vec3(0, -0.1f, 0),
                                                JPH::Quat::sIdentity(),
                                                JPH::EMotionType::Static,
                                                Layers::NON_MOVING);
      ctx->objects[o_idx].body_id = body_interface.CreateAndAddBody(ground_settings, JPH::EActivation::DontActivate);
    }
    o_idx++;

    // Box (dynamic)
    ctx->objects[o_idx].mesh = Mesh::create_box(arena, gfx, 1.0f, 1.0f, 1.0f);
    ctx->objects[o_idx].mesh->translate(-3.0f, 0.f, 0.0f);
    {
      JPH::BoxShapeSettings box_shape(JPH::Vec3(1.0f, 1.0f, 1.0f));
      JPH::ShapeSettings::ShapeResult shape_result = box_shape.Create();
      JPH::BodyCreationSettings box_settings(shape_result.Get(), JPH::Vec3(-3.0f, 10.1f, 0.0f),
                                             JPH::Quat::sIdentity(),
                                             JPH::EMotionType::Dynamic,
                                             Layers::MOVING);
      ctx->objects[o_idx].body_id = body_interface.CreateAndAddBody(box_settings, JPH::EActivation::Activate);
    }
    o_idx++;

    // Sphere (dynamic)
    ctx->objects[o_idx].mesh = Mesh::create_sphere(arena, gfx, 1.0f, 36, 18);
    ctx->objects[o_idx].mesh->translate(0.0f, 1.5f, 0.0f);
    {
      JPH::SphereShapeSettings sphere_shape(1.0f);
      JPH::ShapeSettings::ShapeResult shape_result = sphere_shape.Create();
      JPH::BodyCreationSettings sphere_settings(shape_result.Get(), JPH::Vec3(0.0f, 10.f, 0.0f),
                                                JPH::Quat::sIdentity(),
                                                JPH::EMotionType::Dynamic,
                                                Layers::MOVING);
      ctx->objects[o_idx].body_id = body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);
    }
    o_idx++;

    // Cone (dynamic, approximate as cylinder)
    ctx->objects[o_idx].mesh = Mesh::create_cone(arena, gfx, 1.0f, 2);
    ctx->objects[o_idx].mesh->translate(3.0f, 0.5f, 0.0f);
    {
      JPH::CylinderShapeSettings cone_shape(1.0f, 0.5f);
      JPH::ShapeSettings::ShapeResult shape_result = cone_shape.Create();
      JPH::BodyCreationSettings cone_settings(shape_result.Get(), JPH::Vec3(3.0f, 10.0f, 0.0f),
                                              JPH::Quat::sIdentity(),
                                              JPH::EMotionType::Dynamic,
                                              Layers::MOVING);
      ctx->objects[o_idx].body_id = body_interface.CreateAndAddBody(cone_settings, JPH::EActivation::Activate);
    }
    o_idx++;

    // Cylinder (dynamic)
    ctx->objects[o_idx].mesh = Mesh::create_cylinder(arena, gfx, 1.0f, 2);
    ctx->objects[o_idx].mesh->translate(-6.0f, 0.5f, 0.0f);
    {
      JPH::CylinderShapeSettings cylinder_shape(1.0f, 0.5f);
      JPH::ShapeSettings::ShapeResult shape_result = cylinder_shape.Create();
      JPH::BodyCreationSettings cylinder_settings(shape_result.Get(), JPH::Vec3(-6.0f, 10.0f, 0.0f),
                                                  JPH::Quat::sIdentity(),
                                                  JPH::EMotionType::Dynamic,
                                                  Layers::MOVING);
      ctx->objects[o_idx].body_id = body_interface.CreateAndAddBody(cylinder_settings, JPH::EActivation::Activate);
    }
    o_idx++;

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
        JPH::Vec3 position = body_interface.GetPosition(ctx->objects[j].body_id);
        JPH::Quat rotation = body_interface.GetRotation(ctx->objects[j].body_id);

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
