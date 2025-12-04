// jolt_debug_renderer.h
#ifndef JOLT_DEBUG_RENDERER_H
#define JOLT_DEBUG_RENDERER_H

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include "graphics_api.h"
#include "linmath.h"
#include "arena2.h"

class JoltDebugRenderer : public JPH::DebugRendererSimple
{
public:
  struct Vertex
  {
    vec3 pos;
    vec4 color;
  };

  JoltDebugRenderer() : vertices(nullptr), vertex_count(0), vertex_capacity(0) { Initialize(); }

  void InitializeLines(Arena *arena, s32 capacity = 1000000)
  {
    vertices = push_array(arena, Vertex, capacity);
    vertex_capacity = capacity;
    vertex_count = 0;
  }

  virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override
  {
    if (vertex_count >= vertex_capacity)
      return;

    Vertex *point = &vertices[vertex_count++];
    point->pos[0] = (r32)inFrom.GetX();
    point->pos[1] = (r32)inFrom.GetY();
    point->pos[2] = (r32)inFrom.GetZ();
    point->color[0] = inColor.r / 255.0f;
    point->color[1] = inColor.g / 255.0f;
    point->color[2] = inColor.b / 255.0f;
    point->color[3] = inColor.a / 255.0f;

    point = &vertices[vertex_count++];
    point->pos[0] = (r32)inTo.GetX();
    point->pos[1] = (r32)inTo.GetY();
    point->pos[2] = (r32)inTo.GetZ();
    point->color[0] = inColor.r / 255.0f;
    point->color[1] = inColor.g / 255.0f;
    point->color[2] = inColor.b / 255.0f;
    point->color[3] = inColor.a / 255.0f;
  }

  virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor, r32 inHeight) override {}

  void Clear()
  {
    vertex_count = 0;
  }

  Vertex *vertices;
  s32 vertex_count;
  s32 vertex_capacity;
};

#endif // JOLT_DEBUG_RENDERER_H
