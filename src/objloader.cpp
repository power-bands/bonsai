#include <vector>
#include <stdio.h>
#include <string>
#include <cstring>

#include "objloader.hpp"

// Very, VERY simple OBJ loader.
// Here is a short list of features a real function would provide :
// - Binary files. Reading a model should be just a few memcpy's away, not parsing a file at runtime. In short : OBJ is not very great.
// - Animations & bones (includes bones weights)
// - Multiple UVs
// - All attributes should be optional, not "forced"
// - More stable. Change a line in the OBJ file and it crashes.
// - More secure. Change another line and you can inject code.
// - Loading from memory, stream, etc

struct u32_static_array
{
  u32 *Elements;

  umm At;
  umm End;
};

struct v3_static_array
{
  v3 *Elements;

  umm At;
  umm End;
};

struct stream_cursor
{
  const char *Start;
  const char *At;
  const char *End;
};

u32_static_array
U32_Static_Array(u32 Count, memory_arena *Memory)
{
  u32 *Elements = PUSH_STRUCT_CHECKED(u32, Memory, Count );
  u32_static_array Result= {Elements, 0, Count};

  return Result;
}

v3_static_array
V3_Static_Array(u32 Count, memory_arena *Memory)
{
  v3 *Elements = PUSH_STRUCT_CHECKED(v3, Memory, Count );
  v3_static_array Result= {Elements, 0, Count};

  return Result;
}

template <typename T, typename T_a>inline void
Push(T Vec, T_a *Array)
{
  Assert( Array->At < Array->End );
  Array->Elements[Array->At++] = Vec;
  return;
}

b32
Contains(const char* Haystack, char Needle)
{
  b32 Result = False;
  while (*Haystack)
  {
    Result |= (Needle == *Haystack++);
  }
  return Result;
}

const char *
EatAllCharacters(const char *At, const char *Characters)
{
  while ( Contains(Characters, *At) )
  {
    ++At;
  }

  return At;
}

char *
ReadUntilTerminatorList(stream_cursor *Cursor, const char *TerminatorList, memory_arena *Arena)
{
  const char *Start = Cursor->At;
  const char *At = Cursor->At;
  const char *Head = Cursor->At;

  umm ResultLength = 0;
  b32 FoundTerminator = False;

  while (Remaining(Cursor) && *At++)
  {
    ResultLength = (umm)(At - Head);
    const char *TerminatorAt = TerminatorList;
    while (*TerminatorAt)
    {
      if(*At == *TerminatorAt)
      {
        FoundTerminator = True;
        ResultLength = (umm)(At - Head);
        At = EatAllCharacters(At, TerminatorList);
        goto done;
      }
      ++TerminatorAt;
    }
  }
done:

  Cursor->At = At;

  char *Result = PUSH_STRUCT_CHECKED(char, Arena, ResultLength + 1);
  MemCopy((u8*)Start, (u8*)Result, ResultLength);

  return Result;
}

void
EatWhitespace(stream_cursor *Cursor)
{
  Cursor->At = EatAllCharacters(Cursor->At, "\n ");
  return;
}

char *
PopWord(stream_cursor *Cursor, memory_arena *Arena, const char *Delimeters = 0)
{
  if (!Delimeters)
    Delimeters = " \n";

  EatWhitespace(Cursor);
  char *Result = ReadUntilTerminatorList(Cursor, Delimeters, Arena);
  return Result;
}

char *
PopLine(stream_cursor *Cursor, memory_arena *Arena)
{
  char *Result = ReadUntilTerminatorList(Cursor, "\n", Arena);
  return Result;
}

r32
PopFloat(stream_cursor *Cursor, memory_arena *Arena)
{
  char *Float = PopWord(Cursor, Arena);
  r32 Result = (r32)atof(Float);
  return Result;
}

u32
PopU32(stream_cursor *Cursor, memory_arena *Arena, const char* Delim = 0)
{
  char *Str = PopWord(Cursor, Arena, Delim);
  u32 Result = (u32)atoi(Str);
  return Result;
}


stream_cursor
StreamCursor(const char *Data, umm Length)
{
  stream_cursor Result = {};
  Result.Start = Data;
  Result.At = Data;
  Result.End = Data + Length + 1;

  return Result;
}

umm
Length(const char *Str)
{
  const char *Start = Str;
  const char *End = Str;

  while (*End++);

  umm Result = (umm)(End - Start);
  return Result;
}

stream_cursor
StreamCursor(const char *Data)
{
  umm DataLen = Length(Data);
  stream_cursor Result = StreamCursor(Data, DataLen);
  return Result;
}

struct obj_stats
{
  u32 VertCount;
  u32 NormalCount;
  u32 UVCount;
  u32 FaceCount;
};

obj_stats
GetObjStats(stream_cursor Cursor, memory_arena *Memory)
{
  obj_stats Result = {};

  while (Cursor.At < Cursor.End)
  {
    char *LineType = PopWord(&Cursor, Memory);
    if (LineType == 0) { break; }

    if ( StringsMatch(LineType, "v"))
    {
      ++Result.VertCount;
    }
    else if ( StringsMatch(LineType, "vt") )
    {
      ++Result.UVCount;
    }
    else if ( StringsMatch(LineType, "vn") )
    {
      ++Result.NormalCount;
    }
    else if ( StringsMatch(LineType, "f") )
    {
      ++Result.FaceCount;
    }
    else
    {
      // Irrelevant.
    }
  }

  return Result;
}

model
LoadObj(memory_arena *Memory, const char * FilePath)
{
  Info("Loading .obj file : %s \n", FilePath);


  // FIXME(Jesse): Use TranArena


  umm Length = 0;
  char *Start = ReadEntireFileIntoString(FilePath, Memory, &Length);
  if (!Start) { model M = {}; return M; }

  stream_cursor Stream = StreamCursor(Start, Length);
  obj_stats Stats = GetObjStats(Stream, Memory);

  //
  // FIXME(Jesse): Use TranArena for these
  v3_static_array TempVerts       = V3_Static_Array(Stats.VertCount, Memory);
  v3_static_array TempNormals     = V3_Static_Array(Stats.NormalCount, Memory);

  u32_static_array VertIndicies   = U32_Static_Array(Stats.FaceCount*3, Memory);
  u32_static_array NormalIndicies = U32_Static_Array(Stats.FaceCount*3, Memory);

  while (Stream.At < Stream.End)
  {
    char *LineType = PopWord(&Stream, Memory);
    if (LineType == 0) { break; }

    if ( StringsMatch(LineType, "v") )
    {
      v3 Vert = {{ PopFloat(&Stream, Memory), PopFloat(&Stream, Memory), PopFloat(&Stream, Memory) }};
      Push(Vert*3.0f, &TempVerts);
    }
    else if ( StringsMatch(LineType, "vn") )
    {
      v3 Normal = {{ PopFloat(&Stream, Memory), PopFloat(&Stream, Memory), PopFloat(&Stream, Memory) }};
      Push(Normal, &TempNormals);
    }
#if 0
    else if ( StringsMatch(LineType, "vt") )
    {
      v2 UV = PopV2(&Stream, "%f %f %f\n");
      Push(UV, &TempUVs);
    }
#endif
    else if ( StringsMatch(LineType, "f") )
    {
      s32 vIndex[3] = {};
      s32 nIndex[3] = {};

      if (Stats.UVCount)
      {
        vIndex[0] = PopU32(&Stream, Memory, "/");
        /* Discard UV*/ PopU32(&Stream, Memory, "/");
        nIndex[0] = PopU32(&Stream, Memory);

        vIndex[1] = PopU32(&Stream, Memory, "/");
        /* Discard UV*/ PopU32(&Stream, Memory, "/");
        nIndex[1] = PopU32(&Stream, Memory);

        vIndex[2] = PopU32(&Stream, Memory, "/");
        /* Discard UV*/ PopU32(&Stream, Memory, "/");
        nIndex[2] = PopU32(&Stream, Memory);
      }
      else
      {
        vIndex[0] = PopU32(&Stream, Memory, "//");
        nIndex[0] = PopU32(&Stream, Memory);

        vIndex[1] = PopU32(&Stream, Memory, "//");
        nIndex[1] = PopU32(&Stream, Memory);

        vIndex[2] = PopU32(&Stream, Memory, "//");
        nIndex[2] = PopU32(&Stream, Memory);
      }

      Push(vIndex[0]-1, &VertIndicies );
      Push(vIndex[1]-1, &VertIndicies );
      Push(vIndex[2]-1, &VertIndicies );

      Push(nIndex[0]-1, &NormalIndicies );
      Push(nIndex[1]-1, &NormalIndicies );
      Push(nIndex[2]-1, &NormalIndicies );

    }
    else
    {
      // Irrelevant.
    }

    continue;
  }

  Assert(Remaining(&TempVerts) == 0 );
  Assert(Remaining(&TempNormals) == 0 );
  Assert(Remaining(&VertIndicies) == 0 );
  Assert(Remaining(&NormalIndicies) == 0 );

  untextured_3d_geometry_buffer Mesh = {};
  AllocateMesh(&Mesh, Stats.FaceCount*3, Memory);

  for( u32 Index = 0;
       Index < VertIndicies.At;
       ++Index )
  {
    u32 vIndex = VertIndicies.Elements[Index];
    u32 nIndex = NormalIndicies.Elements[Index];

    v3 Vertex = TempVerts.Elements[vIndex];
    v3 Normal = TempNormals.Elements[nIndex];

    Mesh.Verts[Mesh.At] = Vertex;
    Mesh.Normals[Mesh.At] = Normal;
    Mesh.At++;

    Assert(Mesh.At < Mesh.End);
  }

  Assert(Mesh.At+1 == Mesh.End);

  model Result = {};
  Result.Chunk = PUSH_STRUCT_CHECKED(chunk_data, Memory, 1);;
  Result.Chunk->Mesh = Mesh;
  SetFlag(&Result, Chunk_Initialized);

  return Result;
}
