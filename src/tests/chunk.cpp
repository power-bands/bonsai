
#define VR_X 8
#define VR_Y 8
#define VR_Z 8

#include <bonsai_types.h>

global_variable chunk_dimension WORLD_CHUNK_DIM = Chunk_Dimension(8,8,8);

#include <engine/cpp/render_position.cpp>
#include <engine/cpp/chunk.cpp>
#include <engine/cpp/mesh.cpp>
#include <engine/cpp/threadsafe.cpp>
#include <engine/cpp/triangle.cpp>
#include <engine/cpp/debug_draw.cpp>
#include <engine/cpp/lod.cpp>
#include <engine/cpp/world_chunk.cpp>

#include <tests/test_utils.cpp>


s32
main()
{
  TestSuiteBegin("Chunk");

  memory_arena *Memory = AllocateArena(Megabytes(32));

  chunk_dimension DestChunkDim = WORLD_CHUNK_DIM;
  world_position  DestChunkP = World_Position(0);

  chunk_dimension SynChunkDim = DestChunkDim+2;
  world_position  SynChunkP = World_Position(0);

  { // Ensure copy works as intended
    world_chunk *SyntheticChunk = AllocateWorldChunk(Memory, SynChunkP, SynChunkDim);
    world_chunk *DestChunk = AllocateWorldChunk(Memory, DestChunkP, DestChunkDim);

    InitChunkPlane(0, SyntheticChunk, SynChunkDim, GRASS_GREEN);
    CopyChunkOffset(SyntheticChunk, SynChunkDim, DestChunk, DestChunkDim, Voxel_Position(1));
    SetFlag(DestChunk, Chunk_Initialized);
    SetFlag(SyntheticChunk, Chunk_Initialized);

    for ( int z = 0; z < DestChunkDim.z; ++ z)
    {
      for ( int y = 0; y < DestChunkDim.y; ++ y)
      {
        for ( int x = 0; x < DestChunkDim.x; ++ x)
        {
          TestThat( NotFilledInChunk(SyntheticChunk->Data, Voxel_Position(x+1,y+1,z+1), SynChunkDim) );
          TestThat( NotFilledInChunk(DestChunk->Data, Voxel_Position(x,y,z), DestChunkDim) );
        }
      }
    }

  }

  { // Ensure copy works as intended
    world_chunk *SyntheticChunk = AllocateWorldChunk(Memory, SynChunkP, SynChunkDim);
    world_chunk *DestChunk = AllocateWorldChunk(Memory, DestChunkP, DestChunkDim);

    InitChunkPlane(1, SyntheticChunk, SynChunkDim, GRASS_GREEN);
    CopyChunkOffset(SyntheticChunk, SynChunkDim, DestChunk, DestChunkDim, Voxel_Position(1));
    SetFlag(DestChunk, Chunk_Initialized);
    SetFlag(SyntheticChunk, Chunk_Initialized);

    for ( int z = 0; z < DestChunkDim.z; ++ z)
    {
      for ( int y = 0; y < DestChunkDim.y; ++ y)
      {
        for ( int x = 0; x < DestChunkDim.x; ++ x)
        {
          if ( z == 0 )
          {
            TestThat( IsFilledInChunk(DestChunk->Data, Voxel_Position(x,y,z), DestChunkDim) );
            TestThat( IsFilledInChunk(SyntheticChunk->Data, Voxel_Position(x+1,y+1,z+1), SynChunkDim) );
          }
          else
          {
            TestThat( NotFilledInChunk(DestChunk->Data, Voxel_Position(x,y,z), DestChunkDim) );
            TestThat( NotFilledInChunk(SyntheticChunk->Data, Voxel_Position(x+1,y+1,z+1), SynChunkDim) );
          }
        }
      }
    }

  }


  TestSuiteEnd();
}
