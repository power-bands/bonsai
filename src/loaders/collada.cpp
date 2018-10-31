#include <xml.cpp>

r32
StringToFloat(counted_string* String)
{
  const char* Temp = FormatString(TranArena, "%s", String->Start);

  r32 Result = atof(Temp);
  return Result;
}

s32
StringToInt(counted_string String)
{
  const char* Temp = FormatString(TranArena, "%s", String.Start);

  s32 Result = atoi(Temp);
  return Result;
}

s32
StringToInt(counted_string* String)
{
  s32 Result = StringToInt(*String);
  return Result;
}

v3_stream
ParseV3Array(umm ElementCount, ansi_stream FloatStream, memory_arena* Memory)
{
  v3_stream Result = V3_Stream(ElementCount, Memory);

  for (umm DestIndex = 0;
      DestIndex < ElementCount;
      ++DestIndex)
  {
    for (u32 Inner = 0;
         Inner < 3;
        ++Inner)
    {
      counted_string Float = PopWordCounted(&FloatStream);
      Result.Start[DestIndex].E[Inner] = StringToFloat(&Float);
    }
    ++Result.At;
  }

  return Result;
}

r32_stream
ParseFloatArray(umm TotalFloatCount, ansi_stream FloatStream, memory_arena* Memory)
{
  r32_stream Result = R32_Stream(TotalFloatCount, Memory);

  for (umm DestIndex = 0;
      DestIndex < TotalFloatCount;
      ++DestIndex)
  {
    counted_string Float = PopWordCounted(&FloatStream);
    Result.Start[DestIndex] = StringToFloat(&Float);
  }

  return Result;
}

void
Dump(v3_stream* Array)
{
  umm ElementCount = AtElements(Array);

  for (umm ElementIndex = 0;
      ElementIndex < ElementCount;
      ++ElementIndex)
  {
    Print(Array->Start[ElementIndex]);
  }

  return;
}

void
Dump(hashtable<xml_tag*> * Hash)
{
  for (u32 ElementIndex = 0;
      ElementIndex < Hash->Size;
      ++ElementIndex)
  {
    xml_tag* Element = Hash->Table[ElementIndex];
    if (Element)
    {
      Log("");
      Print(ElementIndex);
      Print(Element);
      while (Element = Element->NextInHash)
      {
        Print(Element);
      }
    }
  }

  return;
}

void
Dump(xml_token_stream *Stream, umm TokenCount)
{
  for (u32 TokenIndex = 0;
      TokenIndex < TokenCount;
      ++TokenIndex)
  {
    Print(Stream->Start + TokenIndex);
  }
}

animation
AllocateAnimation(v3i KeyframeCount, memory_arena* Memory)
{
  animation Result = {};

  Result.xKeyframeCount = KeyframeCount.x;
  Result.xKeyframes = Allocate(keyframe, Memory, KeyframeCount.x);

  Result.yKeyframeCount = KeyframeCount.y;
  Result.yKeyframes = Allocate(keyframe, Memory, KeyframeCount.y);

  Result.zKeyframeCount = KeyframeCount.z;
  Result.zKeyframes = Allocate(keyframe, Memory, KeyframeCount.z);

  return Result;
}

loaded_collada_mesh
LoadMeshData(xml_token_stream* XmlTokens, counted_string* GeometryId, memory_arena* PermMemory, memory_arena* TempMemory)
{
  counted_string PositionsSelector   = FormatCountedString(TempMemory, "geometry%.*s float_array%.*s-positions-array", GeometryId->Count, GeometryId->Start, GeometryId->Count, GeometryId->Start);
  counted_string NormalsSelector     = FormatCountedString(TempMemory, "geometry%.*s float_array%.*s-normals-array", GeometryId->Count, GeometryId->Start, GeometryId->Count, GeometryId->Start);
  counted_string VertexCountSelector = FormatCountedString(TempMemory, "geometry%.*s polylist vcount", GeometryId->Count, GeometryId->Start);
  counted_string VertIndicesSelector = FormatCountedString(TempMemory, "geometry%.*s polylist p", GeometryId->Count, GeometryId->Start);
  counted_string PolylistSelector    = FormatCountedString(TempMemory, "geometry%.*s polylist", GeometryId->Count, GeometryId->Start);

  ansi_stream Triangles         = AnsiStream(GetFirstMatchingTag(XmlTokens, &VertexCountSelector)->Value);
  ansi_stream VertIndices       = AnsiStream(GetFirstMatchingTag(XmlTokens, &VertIndicesSelector)->Value);

  xml_tag* Polylist             = GetFirstMatchingTag(XmlTokens, &PolylistSelector);
  u32 TotalTriangleCount        = StringToInt(GetPropertyValue(Polylist, CS("count")));

  xml_tag* PositionTag          = GetFirstMatchingTag(XmlTokens, &PositionsSelector);
  xml_tag* NormalTag            = GetFirstMatchingTag(XmlTokens, &NormalsSelector);
  counted_string PositionString = PositionTag->Value;
  counted_string NormalString   = NormalTag->Value;
  u32 PositionCount             = StringToInt(GetPropertyValue(PositionTag, CS("count")));
  u32 NormalCount               = StringToInt(GetPropertyValue(NormalTag, CS("count")));

  v3_stream Positions           = ParseV3Array(PositionCount, AnsiStream(PositionString), TempMemory);
  v3_stream Normals             = ParseV3Array(NormalCount, AnsiStream(NormalString), TempMemory);


  chunk_data* ChunkData = AllocateChunk(PermMemory, Chunk_Dimension(0,0,0));

  v3 MaxP = V3(0);
  v3 MinP = V3(FLT_MAX);

  for (u32 TriangleIndex = 0;
      TriangleIndex < TotalTriangleCount;
      ++TriangleIndex)
  {
    counted_string CurrentTriangle = PopWordCounted(&Triangles);
    Assert(StringsMatch(CurrentTriangle, CS("3")));

    for (u32 VertIndex = 0;
        VertIndex < 3;
        ++VertIndex)
    {
      Assert(ChunkData->Mesh.At < ChunkData->Mesh.End);

      u32 PositionIndex = (u32)StringToInt(PopWordCounted(&VertIndices));
      u32 NormalIndex = (u32)StringToInt(PopWordCounted(&VertIndices));
      Assert(PositionIndex < TotalElements(&Positions));
      Assert(NormalIndex < TotalElements(&Normals));

      v3 P = Positions.Start[PositionIndex];
      ChunkData->Mesh.Verts[ChunkData->Mesh.At] = P;
      MaxP = Max(P, MaxP);
      MinP = Min(P, MinP);

      ChunkData->Mesh.Normals[ChunkData->Mesh.At] = Normalize(Normals.Start[NormalIndex]);

      ChunkData->Mesh.At++;
    }
  }

  loaded_collada_mesh Result = {};
  Result.ChunkData = ChunkData;
  Result.Dim = MaxP - MinP;

  return Result;
}

xml_tag*
ParseKeyframesForAxis(xml_token_stream* XmlTokens, char Axis, xml_tag** TimeTag, counted_string* GeometryName)
{
  counted_string xChannelSelector = FormatCountedString(TranArena, "library_animations channel:target=%.*s/location.%c", GeometryName->Count, GeometryName->Start, Axis);
  xml_tag* xChannel = GetFirstMatchingTag(XmlTokens, &xChannelSelector);
  counted_string* xChannelId = GetPropertyValue(xChannel, CS("source"));

  counted_string xInputSelector = FormatCountedString(TranArena, "library_animations sampler%.*s input:semantic=INPUT", xChannelId->Count, xChannelId->Start);
  xml_tag* xInputSourceTag = GetFirstMatchingTag(XmlTokens, &xInputSelector);
  counted_string* xInputLocationSourceId = GetPropertyValue(xInputSourceTag, CS("source"));

  counted_string xInputLocationSelector = FormatCountedString(TranArena, "library_animations animation source%.*s float_array", xInputLocationSourceId->Count, xInputLocationSourceId->Start);
  xml_tag* xKeyframeTimeTag = GetFirstMatchingTag(XmlTokens, &xInputLocationSelector);



  counted_string xOutputSelector = FormatCountedString(TranArena, "library_animations sampler%.*s input:semantic=OUTPUT", xChannelId->Count, xChannelId->Start);
  xml_tag* xOutputSourceTag = GetFirstMatchingTag(XmlTokens, &xOutputSelector);
  counted_string* xOutputLocationSourceId = GetPropertyValue(xOutputSourceTag, CS("source"));

  counted_string xOutputLocationSelector = FormatCountedString(TranArena, "library_animations animation source%.*s float_array", xOutputLocationSourceId->Count, xOutputLocationSourceId->Start);
  xml_tag* xKeyframePositionsTag = GetFirstMatchingTag(XmlTokens, &xOutputLocationSelector);


  *TimeTag = xKeyframeTimeTag;
  return xKeyframePositionsTag;
}

r32
CopyKeyframeData(keyframe* DestKeyframes, r32_stream* KeyframePositions, r32_stream*  KeyframeTimes)
{
  u32 KeyframeCount = TotalElements(KeyframePositions);
  Assert(KeyframeCount == TotalElements(KeyframeTimes));

  r32 MaxKeyframeTime = 0.0f;
  for (u32 KeyframeIndex = 0;
      KeyframeIndex < KeyframeCount;
      ++KeyframeIndex)
  {
    DestKeyframes[KeyframeIndex].Value = KeyframePositions->Start[KeyframeIndex];
    DestKeyframes[KeyframeIndex].tEnd = KeyframeTimes->Start[KeyframeIndex];
    MaxKeyframeTime = Max(MaxKeyframeTime, KeyframeTimes->Start[KeyframeIndex]);
  }

  return MaxKeyframeTime;
}

model
LoadCollada(memory_arena *Memory, const char * FilePath)
{
  Info("Loading .dae file : %s", FilePath);

  ansi_stream AnsiXml = AnsiStreamFromFile(FilePath, Memory);
  if (!AnsiXml.Start) { model M = {}; return M; }

  xml_token_stream XmlTokens = TokenizeXmlStream(&AnsiXml, Memory);
  model Result = {};

  counted_string VisualSceneElementSelector = CS("library_visual_scenes node:type=NODE instance_geometry");
  xml_tag_stream SceneElements = GetAllMatchingTags(&XmlTokens, &VisualSceneElementSelector, Memory);

  u32 SceneObjects = TotalElements(&SceneElements);

  // NOTE(Jesse): At the moment we only support loading one meshed object per
  // .dae file but in the future we could support more!
  Assert(SceneObjects <= 1);

  for ( u32 ObjectIndex = 0;
       ObjectIndex < SceneObjects;
       ++ObjectIndex )
  {
    xml_tag* Tag = SceneElements.Start[ObjectIndex];
    counted_string* GeometryName = GetPropertyValue(Tag, CS("name"));
    counted_string* GeometryId = GetPropertyValue(Tag, CS("url"));

    Assert(GeometryName && GeometryId);

    loaded_collada_mesh ColladaMesh = LoadMeshData(&XmlTokens, GeometryId, Memory, TranArena);
    Result.Chunk = ColladaMesh.ChunkData;
    Result.Dim = Voxel_Position(ColladaMesh.Dim);

    xml_tag* xKeyframeTimeTag = 0;
    xml_tag* yKeyframeTimeTag = 0;
    xml_tag* zKeyframeTimeTag = 0;

    xml_tag* xKeyframePositionsTag = ParseKeyframesForAxis(&XmlTokens, 'X', &xKeyframeTimeTag, GeometryName);
    xml_tag* yKeyframePositionsTag = ParseKeyframesForAxis(&XmlTokens, 'Y', &yKeyframeTimeTag, GeometryName);
    xml_tag* zKeyframePositionsTag = ParseKeyframesForAxis(&XmlTokens, 'Z', &zKeyframeTimeTag, GeometryName);

    u32 xKeyframeCount = StringToInt(GetPropertyValue(xKeyframeTimeTag, CS("count")));
    u32 yKeyframeCount = StringToInt(GetPropertyValue(yKeyframeTimeTag, CS("count")));
    u32 zKeyframeCount = StringToInt(GetPropertyValue(zKeyframeTimeTag, CS("count")));

    Assert( xKeyframeCount == StringToInt(GetPropertyValue(xKeyframePositionsTag, CS("count"))) );
    Assert( yKeyframeCount == StringToInt(GetPropertyValue(yKeyframePositionsTag, CS("count"))) );
    Assert( zKeyframeCount == StringToInt(GetPropertyValue(zKeyframePositionsTag, CS("count"))) );

    r32_stream xKeyframeTimes = ParseFloatArray(xKeyframeCount, AnsiStream(xKeyframeTimeTag->Value), Memory);
    r32_stream yKeyframeTimes = ParseFloatArray(yKeyframeCount, AnsiStream(yKeyframeTimeTag->Value), Memory);
    r32_stream zKeyframeTimes = ParseFloatArray(zKeyframeCount, AnsiStream(zKeyframeTimeTag->Value), Memory);

    r32_stream xKeyframePositions = ParseFloatArray(xKeyframeCount, AnsiStream(xKeyframePositionsTag->Value), Memory);
    r32_stream yKeyframePositions = ParseFloatArray(yKeyframeCount, AnsiStream(yKeyframePositionsTag->Value), Memory);
    r32_stream zKeyframePositions = ParseFloatArray(zKeyframeCount, AnsiStream(zKeyframePositionsTag->Value), Memory);

    animation Animation = AllocateAnimation(V3i(xKeyframeCount, yKeyframeCount, zKeyframeCount), Memory);


    r32 xMaxKeyframeTime = CopyKeyframeData(Animation.xKeyframes, &xKeyframePositions, &xKeyframeTimes);
    r32 yMaxKeyframeTime = CopyKeyframeData(Animation.yKeyframes, &yKeyframePositions, &yKeyframeTimes);
    r32 zMaxKeyframeTime = CopyKeyframeData(Animation.zKeyframes, &zKeyframePositions, &zKeyframeTimes);


    Animation.tEnd = Max(Max(xMaxKeyframeTime, yMaxKeyframeTime), zMaxKeyframeTime);
    Result.Animation = Animation;
  }

  Result.Chunk->Flags = Chunk_Initialized;
  return Result;
}
