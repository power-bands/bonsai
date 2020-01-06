struct native_file
{
  FILE* Handle;
  counted_string Path;
};

function b32
CloseFile(native_file* File)
{
  b32 Result = False;
  if (File->Handle)
  {
    Result = fclose(File->Handle) == 0 ? True : False;
  }
  else
  {
    Error("Attempted to close %.*s, which was not open.", (s32)File->Path.Count, File->Path.Start);
  }

  return Result;
}

static const char* DefaultPermissions = "rw+b";

function b32
Rename(native_file CurrentFile, counted_string NewFilePath)
{
  const char* Old = GetNullTerminated(CurrentFile.Path);
  const char* New = GetNullTerminated(NewFilePath);
  b32 Result = (rename(Old, New) == 0) ? True : False;
  return Result;
}

function native_file
OpenFile(const char* FilePath, const char* Permissions = DefaultPermissions)
{
  native_file Result = {
    .Path = CS(FilePath)
  };

  FILE* Handle = fopen(FilePath, Permissions);
  if (Handle)
  {
    Result.Handle = Handle;
  }
  else
  {
    Error("Opening File %s, errno: %d", FilePath, errno);
  }

  return Result;
}

function native_file
OpenFile(counted_string FilePath, const char* Permissions = DefaultPermissions)
{
  const char* NullTerminatedFilePath = GetNullTerminated(FilePath);
  native_file Result = OpenFile(NullTerminatedFilePath, Permissions);
  return Result;
}

function native_file
GetTempFile(random_series* Entropy, memory_arena* Memory)
{
  u32 FilenameLength = 32;

  counted_string Filename = {
    .Start = Allocate(char, Memory, FilenameLength),
    .Count = FilenameLength
  };

  for (u32 CharIndex = 0;
      CharIndex < FilenameLength;
      ++CharIndex)
  {
    ((char*)Filename.Start)[CharIndex] = (s8)RandomBetween(97, Entropy, 122);
  }

  Filename = Concat(CS("tmp/"), Filename, Memory);

  native_file Result = OpenFile(Filename, "wb");
  return Result;
}

function inline b32
WriteToFile(native_file* File, counted_string Str)
{
  b32 Result = False;
  umm BytesWriten = fwrite(Str.Start, 1, Str.Count, File->Handle);
  if (BytesWriten == Str.Count)
  {
    Result = True;
  }
  else
  {
    Error("Writing to file %.*s", (s32)File->Path.Count, File->Path.Start);
  }
  return Result;
}
