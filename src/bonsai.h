#ifndef PLAYGROUNDH
#define PLAYGROUNDH

#include <perlin.h>
#include <stdio.h>
#include <types.h>
#include <math.h>

#include <render.h>

#include <constants.hpp>

#define ArrayCount(a) (sizeof(a)/sizeof(a[0]))

#define InvalidDefaultCase default: {assert(false);} break

#define Print(Pos) \
  Print_P( Pos, #Pos )

inline void
Print_P( canonical_position P, const char* name)
{
  printf(" -- %s\n", name);
  printf(" Offset: %f %f %f \n", P.Offset.x, P.Offset.y, P.Offset.z );
  printf(" WorldP: %d %d %d \n", P.WorldP.x, P.WorldP.y, P.WorldP.z );
}

inline void
Print_P( voxel_position P, const char* name)
{
  printf(" %s %d %d %d \n", name, P.x, P.y, P.z );
}

inline void
Print_P( v3 P, const char* name)
{
  printf(" %s %f %f %f \n", name, P.x, P.y, P.z );
}

inline void
Print_P( glm::vec3 P, const char* name)
{
  printf(" %s %f %f %f \n", name, P.x, P.y, P.z );
}

inline int
UnSetFlag( int Flags, int Flag )
{
  int Result = Flags &= ~Flag;
  return Result;
}

inline int
SetFlag( int Flags, int Flag )
{
  int Result = Flags |= Flag;
  return Result;
}

inline bool
IsSet( int Flags, int Flag )
{
  bool Result = ( (Flags & Flag) != 0 );
  return Result;
}

inline bool
NotSet( int Flags, int Flag )
{
  bool Result = !(IsSet(Flags, Flag));
  return Result;
}

enum ChunkFlags {
  Chunk_Uninitialized           = 1 << 0,

  Chunk_Entity                  = 1 << 1,
  Chunk_World                   = 1 << 2,
  Chunk_RebuildExteriorBoundary = 1 << 3,
  Chunk_RebuildInteriorBoundary = 1 << 4
};


enum VoxelFlags {

  Voxel_Filled    = 1 << ((N_VOXEL_STORAGE_BITS*3) + 1),

  Voxel_Yellow    = 1 << ((N_VOXEL_STORAGE_BITS*3) + 2),
  Voxel_Red       = 1 << ((N_VOXEL_STORAGE_BITS*3) + 3),
  Voxel_Green     = 1 << ((N_VOXEL_STORAGE_BITS*3) + 4),
  Voxel_Teal      = 1 << ((N_VOXEL_STORAGE_BITS*3) + 5),
  Voxel_White     = 1 << ((N_VOXEL_STORAGE_BITS*3) + 6),
  Voxel_Purple    = 1 << ((N_VOXEL_STORAGE_BITS*3) + 7)
};


struct Voxel
{
  int flags;
};

inline Voxel
SetVoxelP(Voxel V, voxel_position P)
{
  assert( P.x < Pow2(N_VOXEL_STORAGE_BITS) );
  assert( P.y < Pow2(N_VOXEL_STORAGE_BITS) );
  assert( P.z < Pow2(N_VOXEL_STORAGE_BITS) );

  Voxel Result = V;

  Result.flags &= 0xFFFF0000 >> (8 - N_VOXEL_STORAGE_BITS);

  Result.flags |= P.x << (N_VOXEL_STORAGE_BITS * 0);
  Result.flags |= P.y << (N_VOXEL_STORAGE_BITS * 1);
  Result.flags |= P.z << (N_VOXEL_STORAGE_BITS * 2);

  return Result;

}

inline voxel_position
GetVoxelP(chunk_dimension Dim, int i)
{
  int x = i % Dim.x;
  int y = (i/Dim.x) % Dim.y ;
  int z = i / (Dim.x*Dim.y);

  assert(x <= Dim.x);
  assert(y <= Dim.y);
  assert(z <= Dim.z);

  voxel_position Result = Voxel_Position(x,y,z);
  return Result;
}

inline voxel_position
GetVoxelP(Voxel V)
{
  voxel_position P = Voxel_Position(
    V.flags >> (N_VOXEL_STORAGE_BITS * 0) & 0x000000FF >> (8 - N_VOXEL_STORAGE_BITS),
    V.flags >> (N_VOXEL_STORAGE_BITS * 1) & 0x000000FF >> (8 - N_VOXEL_STORAGE_BITS),
    V.flags >> (N_VOXEL_STORAGE_BITS * 2) & 0x000000FF >> (8 - N_VOXEL_STORAGE_BITS)
  );

  return P;
}

struct Chunk
{
  Voxel *Voxels;

  Voxel *BoundaryVoxels;
  int BoundaryVoxelCount;

  chunk_dimension Dim;

  // Position in absolute world coordinates.  A chunk is one world coordinate
  voxel_position WorldP;

  // Position within the chunk this is contained in, if applicable
  v3 Offset;

  int flags;
};

void
ZeroChunk( Chunk * chunk )
{
  for ( int i = 0; i < Volume(chunk->Dim); ++ i)
  {
    chunk->Voxels[i].flags = 0;
  }

  chunk->BoundaryVoxelCount = 0;

  chunk->flags = 0;
  chunk->flags = SetFlag( chunk->flags, Chunk_Uninitialized );
  chunk->flags = SetFlag( chunk->flags, Chunk_RebuildInteriorBoundary );
  chunk->flags = SetFlag( chunk->flags, Chunk_RebuildExteriorBoundary );
}

Chunk
AllocateChunk(chunk_dimension Dim, voxel_position WorldP)
{
  Chunk Result;

  Result.Dim = Dim;

  Result.WorldP = WorldP;
  Result.Offset = V3(0,0,0);

  Result.Voxels = (Voxel*)calloc(Volume(Dim), sizeof(Voxel));
  Result.BoundaryVoxels = (Voxel*)calloc(Volume(Dim), sizeof(Voxel));

  ZeroChunk(&Result);

  for (int i = 0; i < Volume(Result.Dim); ++i)
  {
    Result.Voxels[i] = SetVoxelP( Result.Voxels[i], GetVoxelP(Dim, i) );
  }

  return Result;
}

struct ChunkStack
{
  Chunk *chunks; // This should be Volume(VisibleRegion) chunks
  int count = 0;
};

Chunk
PopChunkStack(ChunkStack *stack)
{
  Chunk Result = stack->chunks[--stack->count];

  assert(stack->count >= 0);
  return Result;
};

void
PushChunkStack(ChunkStack *stack, Chunk chunk)
{
  assert(stack->count + 1 < CHUNK_STACK_SIZE);

  stack->chunks[stack->count++] = chunk;
  return;
};

struct Frustum
{
  float farClip;
  float nearClip;
  float FOV;
};

struct Camera_Object
{
  Frustum Frust;

  canonical_position P;
  canonical_position Target; // TODO : Can this just be a v3?

  v3 Front;
};

struct World
{
  Chunk *Chunks;

  ChunkStack FreeChunks;

  // This is the number of chunks in xyz we're going to update and render
  chunk_dimension VisibleRegion;

  chunk_dimension ChunkDim;

  world_position VisibleRegionOrigin;

  PerlinNoise Noise;

  v3 Gravity;

  VertexBlock VertexData;
  VertexBlock ColorData;
  VertexBlock NormalData;

  int VertexCount; // How many verticies are we drawing
};

struct Entity
{
  Chunk Model;
  v3 Velocity;
  v3 Acceleration;
};

struct collision_event
{
  canonical_position CP;
  bool didCollide;
};



Chunk*
GetWorldChunk( World *world, world_position WorldP )
{
  Chunk *Result;

  if (
    WorldP.x < 0 ||
    WorldP.x >= world->VisibleRegion.x ||

    WorldP.y < 0 ||
    WorldP.y >= world->VisibleRegion.y ||

    WorldP.z < 0 ||
    WorldP.z >= world->VisibleRegion.z )
  {
    /* assert(false); // Requesting outside the initialized world; no bueno? */
    return nullptr;
  }

  int i =
    WorldP.x +
    (WorldP.y * world->VisibleRegion.x) +
    (WorldP.z * world->VisibleRegion.x * world->VisibleRegion.y);

  Result = &world->Chunks[i];

  assert( Result->WorldP == WorldP );

  return Result;
}

inline bool
IsFacingPoint( v3 FaceToPoint, v3 FaceNormal )
{
  bool Result = false;

#if DEBUG_OPTIMIZE_TRI_COUNT
  if ( Dot(FaceToPoint, FaceNormal) > 0 )
  {
    Result = true;
  }
#else
  Result=true;
#endif

  return Result;
}

inline bool
IsFacingPoint( glm::vec3 FaceToPoint, v3 FaceNormal )
{
  bool Result = IsFacingPoint(GLV3(FaceToPoint), FaceNormal);
  return Result;
}

inline int
GetIndex(voxel_position P, Chunk *chunk)
{
  int i =
    (P.x) +
    (P.y*chunk->Dim.x) +
    (P.z*chunk->Dim.x*chunk->Dim.y);

  assert(i < Volume(chunk->Dim));

  return i;
}

inline bool
IsFilledInWorld( Chunk *chunk, voxel_position VoxelP )
{
  bool isFilled = true;

  if (chunk)
  {
    int i = GetIndex(VoxelP, chunk);

    assert(i > -1);
    assert(i < Volume(chunk->Dim));
    assert(VoxelP == GetVoxelP(chunk->Voxels[i]));

    isFilled = IsSet(chunk->Voxels[i].flags, Voxel_Filled);
  }

  return isFilled;
}

inline bool
IsFilledInWorld( World *world, Chunk *chunk, canonical_position VoxelP )
{
  bool isFilled = true;

  if ( chunk )
  {
    Chunk *localChunk = chunk;

    if ( chunk->WorldP != VoxelP.WorldP )
    {
      localChunk = GetWorldChunk(world, VoxelP.WorldP);
    }

    isFilled = IsFilledInWorld( localChunk, Voxel_Position(VoxelP.Offset) );
  }

  return isFilled;
}

inline bool
NotFilledInWorld( World *world, Chunk *chunk, canonical_position VoxelP )
{
  bool Result = !(IsFilledInWorld(world,chunk,VoxelP));
  return Result;
}

bool
IsFilled( Chunk *chunk, voxel_position VoxelP )
{
  int i = GetIndex( VoxelP, chunk);

  assert(i > -1);
  assert(i < Volume(chunk->Dim));

  bool isFilled = IsSet(chunk->Voxels[i].flags, Voxel_Filled);
  return isFilled;
}

inline voxel_position
ClampPositive( voxel_position V )
{
  voxel_position Result = V;

  if ( V.x < 0 )
    Result.x = 0;

  if ( V.y < 0 )
    Result.y = 0;

  if ( V.z < 0 )
    Result.z = 0;

  return Result;
}


// NOTE : The maximum bound is non-inclusive; 0 is part of the chunk
// while the furthest point in x,y or z is the next chunk
inline canonical_position
Canonicalize( World *world, v3 Offset, world_position WorldP )
{
  canonical_position Result;

  Result.Offset = Offset;
  Result.WorldP = WorldP;

  if ( Result.Offset.x >= world->ChunkDim.x )
  {
    int ChunkWidths = Result.Offset.x / world->ChunkDim.x;
    Result.Offset.x -= world->ChunkDim.x*ChunkWidths;
    Result.WorldP.x += ChunkWidths;
  }
  if ( Result.Offset.y >= world->ChunkDim.y )
  {
    int ChunkWidths = Result.Offset.y / world->ChunkDim.y;
    Result.Offset.y -= world->ChunkDim.y*ChunkWidths;
    Result.WorldP.y += ChunkWidths;
  }
  if ( Result.Offset.z >= world->ChunkDim.z )
  {
    int ChunkWidths = Result.Offset.z / world->ChunkDim.z;
    Result.Offset.z -= world->ChunkDim.z*ChunkWidths;
    Result.WorldP.z += ChunkWidths;
  }

  if ( Result.Offset.x < 0 )
  {
    int ChunkWidths = (Result.Offset.x-world->ChunkDim.x) / -world->ChunkDim.x;
    Result.Offset.x += world->ChunkDim.x*ChunkWidths;
    Result.WorldP.x -= ChunkWidths;
  }
  if ( Result.Offset.y < 0 )
  {
    int ChunkWidths = (Result.Offset.y-world->ChunkDim.y) / -world->ChunkDim.y;
    Result.Offset.y += world->ChunkDim.y*ChunkWidths;
    Result.WorldP.y -= ChunkWidths;
  }
  if ( Result.Offset.z < 0 )
  {
    int ChunkWidths = (Result.Offset.z-world->ChunkDim.z) / -world->ChunkDim.z;
    Result.Offset.z += world->ChunkDim.z*ChunkWidths;
    Result.WorldP.z -= ChunkWidths;
  }

  return Result;
}

inline canonical_position
Canonicalize( World *world, canonical_position CP )
{
  canonical_position Result = Canonicalize( world, CP.Offset, CP.WorldP );
  return Result;
}

#endif