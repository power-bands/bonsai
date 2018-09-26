
inline b32
StringsMatch(const char *S1, const char *S2)
{
  const char *S1At = S1;
  const char *S2At = S2;

  b32 Result = (*S1At == *S2At);
  while (Result && (*S1At || *S2At))
  {
    Result &= (*S1At++ == *S2At++);
  }

  return Result;
}

inline b32
StringsMatch(counted_string* S1, counted_string* S2)
{
  b32 Result = True;

  if (S1->Count == S2->Count)
  {
    for (u32 CharIndex = 0;
        CharIndex < S1->Count;
        ++CharIndex)
    {
      Result = (Result && (S1->Start[CharIndex] == S2->Start[CharIndex]));
    }
  }
  else
  {
    Result = False;
  }

  return Result;
}

inline b32
Contains(const char *S1, const char *S2)
{
  const char *S1At = S1;
  while (*S1At)
  {
    const char *S2At = S2;

    b32 Result = (*S1At == *S2At);
    while (Result && *S1At && *S2At)
    {
      Result &= (*S1At++ == *S2At++);
    }

    if (Result && *S2At == 0)
    {
      return True;
    }
    else
    {
      ++S1At;
    }
  }

  return False;
}

char *
FormatString(memory_arena *Memory, const char* FormatString, ...)
{
  char *Buffer = AllocateProtection(char, Memory, 1024, False);

  va_list Arguments;
  va_start(Arguments, FormatString);
  vsnprintf(Buffer, 1023, FormatString, Arguments);
  va_end(Arguments);

  return Buffer;
}

char *
FormatString(const char* FormatString, ...)
{
  char *Buffer = AllocateProtection(char, TranArena, 1024, False);

  va_list Arguments;
  va_start(Arguments, FormatString);
  vsnprintf(Buffer, 1023, FormatString, Arguments);
  va_end(Arguments);

  return Buffer;
}
