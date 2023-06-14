#define PLATFORM_GL_IMPLEMENTATIONS 1
#define DEBUG_SYSTEM_API 1

#include <bonsai_types.h>

#include <game_constants.h>
#include <game_types.h>

const s32 SQUARE_SIZE = 8;

void
SimpleCamera(camera* Camera, float FarClip, float DistanceFromTarget, canonical_position InitialTarget)
{
  Clear(Camera);

  Camera->Frust.farClip = FarClip;
  Camera->Frust.nearClip = 1.0f;
  Camera->Frust.width = 30.0f;
  Camera->Frust.FOV = 45.0f;

  Camera->Up = WORLD_Z;
  Camera->Right = WORLD_X;

  Camera->Pitch = PI32 * 0.645f;
  Camera->Yaw = 0;

  Camera->DistanceFromTarget = DistanceFromTarget;

  /* UpdateCameraP( */
  /*     Canonical_Position(Voxel_Position(1,1,1), World_Position(0,0,0)), */
  /*     Camera, */
  /*     WorldChunkDim); */

  /* input *Input = 0; */
  /* v2 MouseDelta = {}; */
  /* UpdateGameCamera(World, MouseDelta, Input, InitialTarget, Camera); */

  return;
}

inline
v3
Input_Move_Square(hotkeys *Hotkeys) {
  v3 UpdateDir = V3(0,0,0);

  if (Hotkeys->Forward) {
    UpdateDir.y += 1;
  } else if (Hotkeys->Backward) {
    UpdateDir.y -= 1;
  } else if (Hotkeys->Left) {
    UpdateDir.x -= 1;
  } else if (Hotkeys->Right) {
    UpdateDir.x += 1;
  }

  return UpdateDir; 
}


link_internal u32
Noise_Checkered( perlin_noise *Noise,
            world_chunk *Chunk,
            chunk_dimension Dim,
            chunk_dimension SrcToDest,
            u8 ColorIndex,
            s32 Frequency,
            s32 Amplitude,
            s64 zMin,
            chunk_dimension WorldChunkDim,
            void *Ignored )
{
  Assert(SQUARE_SIZE % 2 == 0);

  u32 Result = 0;

  for ( s32 z = 0; z < Dim.z; ++ z)
  {
    s64 WorldZ = z - zMin - SrcToDest.z + (WorldChunkDim.z*Chunk->WorldP.z);
    for ( s32 y = 0; y < Dim.y; ++ y)
    {
      for ( s32 x = 0; x < Dim.x; ++ x)
      {
        if (WorldZ < zMin)
        {
          s32 Index = GetIndex(Voxel_Position(x,y,z), Dim);
          Chunk->Voxels[Index].Flags = Voxel_Filled;

          if (y / SQUARE_SIZE % 2 == 0) {
            ColorIndex = x / SQUARE_SIZE % 2 == 1 ? GREY_7 : GREY_0;
          } else {
            ColorIndex = x / SQUARE_SIZE % 2 == 1 ? GREY_0 : GREY_7;
          }

          Chunk->Voxels[Index].Color = ColorIndex;
          ++Result;
        }
      }
    }
  }

  return Result;
}


model *
AllocateGameModels(game_state *GameState, memory_arena *Memory, heap_allocator *Heap)
{
  model *Result = Allocate(model, Memory, ModelIndex_Count);
  Result[ModelIndex_FirstPlayerModel] = LoadVoxModel(Memory, Heap, "models/players/chr_flatguy.vox", GetTranArena());

  return Result;
}


// NOTE(Jesse): This function gets called for each worker thread at engine
// startup, but not the main thread!
BONSAI_API_WORKER_THREAD_INIT_CALLBACK()
{
  Global_ThreadStates = AllThreads;
  SetThreadLocal_ThreadIndex(ThreadIndex);
}

// NOTE(Jesse): This is the worker thread loop.  These are a few default
// implementations of functions for copying data around in the engine.
//
// TODO(Jesse): Make these jobs opt-in, such that game code doesn't have to
// care about these unless it wants to.  Most of these jobs will never have a
// custom implementation in games until way into development, if ever.
//
BONSAI_API_WORKER_THREAD_CALLBACK()
{
  switch (Entry->Type)
  {
    // NOTE(Jesse): A noop entry is a bug; InvalidCodePath() crashes the process
    // in debug mode, and does nothing in release mode (in the hopes we handle
    // whatever else happens gracefully).
    case type_work_queue_entry_noop: { InvalidCodePath(); } break;

    case type_work_queue_entry_update_world_region:
    case type_work_queue_entry_rebuild_mesh:
    case type_work_queue_entry_init_asset:
    {
      NotImplemented;
    } break;

    case type_work_queue_entry_init_world_chunk:
     {
       volatile work_queue_entry_init_world_chunk *Job = SafeAccess(work_queue_entry_init_world_chunk, Entry);
       world_chunk *Chunk = Job->Chunk;

       if (ChunkIsGarbage(Chunk))
       {
       }
       else
       {
        s32 Frequency = 50;
        s32 Amplititude = 15;
        InitializeChunkWithNoise( Noise_Checkered,
                                  Thread,
                                  Chunk,
                                  Chunk->Dim,
                                  0,
                                  Frequency,
                                  Amplititude,
                                  0,
                                  ChunkInitFlag_Noop,                              
                                  0 );
       }

       FinalizeChunkInitialization(Chunk);
     } break;

    case type_work_queue_entry_copy_buffer_ref:
    {
      work_queue_entry_copy_buffer_ref *CopyJob = SafeAccess(work_queue_entry_copy_buffer_ref, Entry);
      DoCopyJob(CopyJob, &Thread->EngineResources->MeshFreelist, Thread->PermMemory);
    } break;

    case type_work_queue_entry_copy_buffer:
    {
      volatile work_queue_entry_copy_buffer *CopyJob = SafeAccess(work_queue_entry_copy_buffer, Entry);
      DoCopyJob(CopyJob, &Thread->EngineResources->MeshFreelist, Thread->PermMemory);
    } break;

    case type_work_queue_entry_copy_buffer_set:
    {
      volatile work_queue_entry_copy_buffer_set *CopySet = SafeAccess(work_queue_entry_copy_buffer_set, Entry);
      RangeIterator(CopyIndex, (s32)CopySet->Count)
      {
        volatile work_queue_entry_copy_buffer *CopyJob = &CopySet->CopyTargets[CopyIndex];
        DoCopyJob(CopyJob, &Thread->EngineResources->MeshFreelist, Thread->PermMemory);
      }
    } break;

    case type_work_queue_entry_sim_particle_system:
    {
      work_queue_entry_sim_particle_system *Job = SafeAccess(work_queue_entry_sim_particle_system, Entry);
      SimulateParticleSystem(Job);
    } break;
  }
}

// NOTE(Jesse): This gets called once on the main thread at engine startup.
// This is a bare-bones example of the code you'll need to spawn a camera,
// and spawn an entity for it to follow.
//
// The movement code for the entity is left as an excercise for the reader and
// should be implemented in the main thread callback.
//
// This function must return a pointer to a GameState struct, as defined by the
// game.  In this example, the definition is in `examples/blank_project/game_types.h`
BONSAI_API_MAIN_THREAD_INIT_CALLBACK()
{
  // NOTE(Jesse): This is a convenience macro for unpacking all the information
  // the engine passes around.  It nominally reduces the amount of typing you have to do.
  //
  UNPACK_ENGINE_RESOURCES(Resources);

  // NOTE(Jesse): Update this path if you copy this project to your new project path
  //
  Global_AssetPrefixPath = CSz("examples/blank_project/assets");

  world_position WorldCenter = {};
  canonical_position CameraTargetP = {};

  GameState = Allocate(game_state, Resources->Memory, 1);
  GameState->Models = AllocateGameModels(GameState, Memory, Heap);

  SimpleCamera(Graphics->Camera, 10000.0f, 480.0f, CameraTargetP);

  AllocateWorld(World, WorldCenter, WORLD_CHUNK_DIM, g_VisibleRegion);

  // World->Flags = WorldFlag_WorldCenterFollowsCameraTarget;

  entity *CameraTarget = GetFreeEntity(EntityTable);
  SpawnEntity( CameraTarget, EntityType_Player, GameState->Models, ModelIndex_FirstPlayerModel );

  Resources->CameraTarget = CameraTarget;

  return GameState;
}

// NOTE(Jesse): This is the main game loop.  Put your game update logic here!
//
BONSAI_API_MAIN_THREAD_CALLBACK()
{
  Assert(ThreadLocal_ThreadIndex == 0);

  TIMED_FUNCTION();
  UNPACK_ENGINE_RESOURCES(Resources);

  f32 dt = Plat->dt;

  Resources->CameraTarget->P.Offset += Input_Move_Square(Hotkeys) * SQUARE_SIZE * 0.1f;
}
