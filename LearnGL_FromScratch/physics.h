#ifndef PHYSICS_H
#define PHYSICS_H

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include "jolt_arena_allocator.h"

namespace Layers
{
  static constexpr JPH::ObjectLayer NON_MOVING = 0;
  static constexpr JPH::ObjectLayer MOVING = 1;
  static constexpr JPH::uint8 NUM_LAYERS = 2;
};

namespace BroadPhaseLayers
{
  static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
  static constexpr JPH::BroadPhaseLayer MOVING(1);
  static constexpr JPH::uint NUM_LAYERS(2);
};

class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface
{
public:
  virtual JPH::uint GetNumBroadPhaseLayers() const override
  {
    return BroadPhaseLayers::NUM_LAYERS;
  }

  virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
  {
    if (layer == Layers::NON_MOVING)
      return BroadPhaseLayers::NON_MOVING;
    return BroadPhaseLayers::MOVING;
  }
};

class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
  virtual bool ShouldCollide(JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2) const override
  {
    switch (layer1)
    {
    case Layers::NON_MOVING:
      return layer2 == BroadPhaseLayers::MOVING;
    case Layers::MOVING:
      return true;
    default:
      return false;
    }
  }
};

class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
public:
  virtual bool ShouldCollide(JPH::ObjectLayer obj1, JPH::ObjectLayer obj2) const override
  {
    switch (obj1)
    {
    case Layers::NON_MOVING:
      return obj2 == Layers::MOVING;
    case Layers::MOVING:
      return true;
    default:
      return false;
    }
  }
};

typedef struct
{
  JPH::Factory *factory_instance;
  JoltTempArenaAllocator *temp_allocator;
  JPH::JobSystemThreadPool *job_system;
  BPLayerInterfaceImpl *broad_phase_layer_interface;
  ObjectVsBroadPhaseLayerFilterImpl *object_vs_broadphase_filter;
  ObjectLayerPairFilterImpl *object_vs_object_filter;
  JPH::PhysicsSystem *physics_system;
} PhysicsState;

#endif