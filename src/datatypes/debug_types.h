#if BONSAI_INTERNAL


struct layout
{
  r32 AtX;
  r32 AtY;
  r32 FontSize;

  layout(u32 FontSize = 12)
  {
    this->FontSize = FontSize;
    this->AtX = 0;
    this->AtY = 0;
  }
};

struct debug_profile_scope
{
  u64 CycleCount;
  const char* Name;

  debug_profile_scope *Parent;
  debug_profile_scope *Sibling;
  debug_profile_scope *Child;
};

struct debug_scope_tree
{
  r32 FrameMs;
  debug_profile_scope *Root;
};

#define ROOT_SCOPE_COUNT 60
struct debug_state
{
  u64 (*GetCycleCount)(void);
  u64 FrameCount;

  debug_text_render_group *TextRenderGroup;

  memory_arena *Memory;

  debug_profile_scope **WriteScope;
  debug_profile_scope *CurrentScope;
  debug_scope_tree ScopeTrees[ROOT_SCOPE_COUNT];
  u32 RootScopeIndex;

  debug_profile_scope FreeScopeSentinel;

  u64 NumScopes;
  b32 Initialized;

  debug_profile_scope *GetReadScopeTree()
  {
    debug_profile_scope *RootScope = this->ScopeTrees[this->RootScopeIndex].Root;
    return RootScope;
  }

  debug_profile_scope **GetWriteScopeTree()
  {
    s32 Index = (this->RootScopeIndex + 1) % ROOT_SCOPE_COUNT;
    debug_profile_scope **RootScope = &this->ScopeTrees[Index].Root;
    return RootScope;
  }
};

debug_global debug_state *GlobalDebugState;
inline debug_state* GetDebugState() {
  Assert(GlobalDebugState->Initialized);
  return GlobalDebugState;
}

enum debug_recording_mode
{
  RecordingMode_Clear,
  RecordingMode_Record,
  RecordingMode_Playback,

  RecordingMode_Count,
};

#define DEBUG_RECORD_INPUT_SIZE 3600
struct debug_recording_state
{
  s32 FramesRecorded;
  s32 FramesPlayedBack;
  debug_recording_mode Mode;

  memory_arena RecordedMainMemory;

  hotkeys Inputs[DEBUG_RECORD_INPUT_SIZE];
};

void
PrintScopeTree(debug_profile_scope *Scope, s32 Depth = 0)
{
  if (!Scope)
    return;

  s32 CurDepth = Depth;

  while (CurDepth--)
  {
    printf("%s", "  ");
  }

  if (Depth > 0)
    printf("%s", " `- ");

  printf("%d %s", Depth, Scope->Name);

  debug_state *DebugState = GetDebugState();
  if (DebugState->WriteScope == &Scope->Child)
    printf(" %s", "<-- Child \n");
  else if (DebugState->WriteScope == &Scope->Sibling)
    printf(" %s", "<-- Sibling \n");
  else
    printf("%s", "\n");


  PrintScopeTree(Scope->Child, Depth+1);
  PrintScopeTree(Scope->Sibling, Depth);

  return;
}


debug_profile_scope NullDebugProfileScope = {};

debug_profile_scope *
GetProfileScope(debug_state *State)
{
  debug_profile_scope *Result = 0;
#if 1
  debug_profile_scope *Sentinel = &State->FreeScopeSentinel;

  if (Sentinel->Child != Sentinel)
  {
    Result = Sentinel->Child;

    Sentinel->Child = Sentinel->Child->Child;
    Sentinel->Child->Child->Parent = Sentinel;
  }
  else
  {
    Result = PUSH_STRUCT_CHECKED(debug_profile_scope, State->Memory, 1);
  }
#else
    Result = PUSH_STRUCT_CHECKED(debug_profile_scope, State->Memory, 1);
#endif

  Assert(Result);
  *Result = NullDebugProfileScope;
  return Result;
}

struct debug_timed_function
{
  u64 StartingCycleCount;

  debug_timed_function(const char *Name)
  {
    debug_state *DebugState = GetDebugState();
    ++DebugState->NumScopes;


    // FIXME(Jesse): Recycle these
    debug_profile_scope *NewScope = GetProfileScope(DebugState);

    NewScope->Parent = DebugState->CurrentScope;

    (*DebugState->WriteScope) = NewScope;
    DebugState->CurrentScope = NewScope;

    DebugState->WriteScope = &NewScope->Child;

    NewScope->Name = Name;
    this->StartingCycleCount = DebugState->GetCycleCount(); // Intentionally last

    /* Debug(" "); */
    /* Debug("Pushing %s", Name); */
    /* PrintScopeTree(DebugState->RootScope); */

  }

  ~debug_timed_function()
  {
    debug_state *DebugState = GetDebugState();
    u64 EndingCycleCount = DebugState->GetCycleCount(); // Intentionally first
    u64 CycleCount = (EndingCycleCount - this->StartingCycleCount);
    DebugState->CurrentScope->CycleCount = CycleCount;

    /* Debug(" "); */
    /* Debug("Popping %s", DebugState->CurrentScope->Name); */

    // 'Pop' the scope stack
    DebugState->WriteScope = &DebugState->CurrentScope->Sibling;
    DebugState->CurrentScope = DebugState->CurrentScope->Parent;
    /* PrintScopeTree(DebugState->RootScope); */
  }
};

#define INIT_DEUBG_STATE(Plat) InitDebugState(Plat)

#define TIMED_FUNCTION() debug_timed_function FunctionTimer(__FUNCTION_NAME__)
#define TIMED_BLOCK(BlockName) { debug_timed_function BlockTimer(BlockName)
#define END_BLOCK(BlockName) }

#define DEBUG_FRAME_RECORD(...) DoDebugFrameRecord(__VA_ARGS__)
#define DEBUG_FRAME_END(Plat) DebugFrameEnd(Plat)

#else

#define INIT_DEUBG_STATE(...)

#define TIMED_FUNCTION(...)
#define TIMED_BLOCK(...)
#define END_BLOCK(...)

#define DEBUG_FRAME_RECORD(...)
#define DEBUG_FRAME_END(...)

#endif


