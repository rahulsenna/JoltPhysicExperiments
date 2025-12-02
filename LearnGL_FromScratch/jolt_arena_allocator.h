#ifndef JOLT_ARENA_ALLOCATOR_H
#define JOLT_ARENA_ALLOCATOR_H

#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include "arena2.h"

class JoltTempArenaAllocator : public JPH::TempAllocator
{
public:
  JoltTempArenaAllocator(Arena *arena, size_t temp_size)
      : arena(arena)
  {
    temp_buffer = (u8 *)arena_push(arena, temp_size, 16, 0); // 16-byte alignment
    temp_buffer_size = temp_size;
    temp_buffer_used = 0;
  }

  virtual void *Allocate(uint inSize) override
  {
    // Align to 16 bytes
    inSize = (inSize + 15) & ~15;
    if (temp_buffer_used + inSize > temp_buffer_size)
    {
      JPH_ASSERT(false, "TempAllocator out of memory!");
      return nullptr;
    }

    void *result = temp_buffer + temp_buffer_used;
    temp_buffer_used += inSize;
    return result;
  }

  virtual void Free(void *inAddress, uint inSize) override {}

  void Clear()
  {
    temp_buffer_used = 0;
  }

private:
  Arena *arena;
  u8 *temp_buffer;
  size_t temp_buffer_size;
  size_t temp_buffer_used;
};

#endif // JOLT_ARENA_ALLOCATOR_H
