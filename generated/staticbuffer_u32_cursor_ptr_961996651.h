// @bind staticbuffer_type_name 

struct entropy_lists
{
  u32_cursor Start[TILE_RULESETS_COUNT];
};

link_inline u32_cursor*
GetPtr( entropy_lists *Buf, umm Index)
{
  u32_cursor *Result = {};
  if ( Index < umm(TILE_RULESETS_COUNT) )
  {
    Result = Buf->Start+Index;
  }
  return Result;
}

link_inline u32_cursor
Get( entropy_lists *Buf, umm Index)
{
  Assert(Index >= 0);
  Assert(Index < umm(TILE_RULESETS_COUNT));
  u32_cursor Result = Buf->Start[Index];
  return Result;
}

link_internal umm
AtElements( entropy_lists  *Buf)
{
  return TILE_RULESETS_COUNT;
}

link_internal umm
TotalElements( entropy_lists *Buf)
{
  return TILE_RULESETS_COUNT;
}

link_internal void
DeepCopy( entropy_lists *Src, entropy_lists *Dest)
{
  Assert(TotalElements(Src) <= TotalElements(Dest));
  IterateOver(Src, Element, ElementIndex)
  {
    DeepCopy(Element, Dest->Start+ElementIndex);
  }
}


