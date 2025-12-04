#include "physics.h"
#include "game_api.h"

void draw_physics(GameMemory *memory, mat4x4 view, mat4x4 projection)
{
  JoltDebugRenderer *debug_renderer = memory->physics->debug_renderer;
  debug_renderer->Clear();
  memory->physics->physics_system->DrawBodies({.mDrawShapeWireframe = true}, debug_renderer);

  if (debug_renderer->vertex_count == 0)
    return;
  DebugLineResources *resources = memory->physics->debug_line_resources;
  GraphicsAPI *gfx = memory->gfx;

  gfx->use_program(resources->shader);
  gfx->bind_vertex_array(resources->vao);
  gfx->update_buffer_data(resources->vbo, debug_renderer->vertices, debug_renderer->vertex_count * sizeof(JoltDebugRenderer::Vertex));

  gfx->set_mat4(resources->shader, "view", (const r32 *)view);
  gfx->set_mat4(resources->shader, "projection", (const r32 *)projection);

  gfx->disable_depth_test();
  gfx->set_line_width(2.0f);

  gfx->draw_line_arrays(0, debug_renderer->vertex_count);
  gfx->enable_depth_test();
}


void init_physics(GameMemory *memory)
{
  GraphicsAPI *gfx = memory->gfx;
  Arena *arena = memory->arena;

  memory->physics = push_struct(arena, PhysicsState);
  memory->physics->factory_instance = new (push_struct(arena, JPH::Factory)) JPH::Factory();

  JPH::RegisterDefaultAllocator();
  JPH::Factory::sInstance = memory->physics->factory_instance;
  JPH::RegisterTypes();

  memory->physics->temp_allocator = new (push_struct(arena, JoltTempArenaAllocator)) JoltTempArenaAllocator(arena, 10 * 1024 * 1024);
  memory->physics->job_system = new (push_struct(arena, JPH::JobSystemThreadPool)) JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

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

  // --------------[ Jolt Debug Render ]-----------------
  memory->physics->debug_renderer = new (push_struct(arena, JoltDebugRenderer)) JoltDebugRenderer();
  memory->physics->debug_renderer->InitializeLines(arena);
  memory->physics->debug_draw_enabled = true;

  memory->physics->debug_line_resources = push_struct(arena, DebugLineResources);
  Shader shader;
  shader.create(arena, "shaders/line.vert", "shaders/line.frag", gfx);
  memory->physics->debug_line_resources->shader = shader.program;

  memory->physics->debug_line_resources->vao = gfx->create_vertex_array(arena);
  memory->physics->debug_line_resources->vbo = gfx->create_buffer(arena, nullptr, 1024 * 1024); // 1MB buffer
  gfx->bind_vertex_array(memory->physics->debug_line_resources->vao);
  gfx->bind_buffer(memory->physics->debug_line_resources->vbo);

  gfx->enable_vertex_attrib(0);
  gfx->vertex_attrib_pointer(0, 3, sizeof(float) * 7, 0);

  gfx->enable_vertex_attrib(1);
  gfx->vertex_attrib_pointer(1, 4, sizeof(float) * 7, sizeof(float) * 3);
  // --------------[ Jolt Debug Render ]-----------------
}