
// NOTE(Jesse): This is where you put data that's specific to your game
struct game_state
{
  random_series Entropy;

  model   *Models;
  entity  *Player;

#if DEBUG_SYSTEM_API
  get_debug_state_proc GetDebugState;
#endif
};

