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
  struct DebugLine
  {
    vec3 pos;
    vec4 color;
  };

  JoltDebugRenderer() : lines(nullptr), lines_count(0), lines_capacity(0) { Initialize(); }

  void InitializeLines(Arena *arena, s32 capacity = 1000000)
  {
    lines = push_array(arena, DebugLine, capacity);
    lines_capacity = capacity;
    lines_count = 0;
  }

  virtual void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override
  {
    if (lines_count >= lines_capacity)
      return;

    DebugLine *line = &lines[lines_count++];
    line->pos[0] = (r32)inFrom.GetX();
    line->pos[1] = (r32)inFrom.GetY();
    line->pos[2] = (r32)inFrom.GetZ();
    line->color[0] = inColor.r / 255.0f;
    line->color[1] = inColor.g / 255.0f;
    line->color[2] = inColor.b / 255.0f;
    line->color[3] = inColor.a / 255.0f;

    line = &lines[lines_count++];
    line->pos[0] = (r32)inTo.GetX();
    line->pos[1] = (r32)inTo.GetY();
    line->pos[2] = (r32)inTo.GetZ();
    line->color[0] = inColor.r / 255.0f;
    line->color[1] = inColor.g / 255.0f;
    line->color[2] = inColor.b / 255.0f;
    line->color[3] = inColor.a / 255.0f;
  }

  virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view &inString, JPH::ColorArg inColor, r32 inHeight) override {}

  void Clear()
  {
    lines_count = 0;
  }

  DebugLine *lines;
  s32 lines_count;
  s32 lines_capacity;
};

#endif // JOLT_DEBUG_RENDERER_H
