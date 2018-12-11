#include <pthread.h>

// Assert
#include <signal.h>

// chdir
#include <unistd.h>

// dlopen
#include <dlfcn.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysymdef.h>

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

#include <semaphore.h>

// Backtrace
#include <errno.h>
#include <execinfo.h>

#include <arpa/inet.h>  // inet_addr
#include <sys/socket.h>

#include <sys/mman.h>   // mmap




#define THREAD_MAIN_RETURN void*

#define exported_function extern "C" __attribute__((visibility("default")))
#define function static

#define ReadBarrier  asm volatile("" ::: "memory"); _mm_lfence()
#define WriteBarrier asm volatile("" ::: "memory"); _mm_sfence()
#define FullBarrier  asm volatile("" ::: "memory"); _mm_sfence(); _mm_lfence()

/*
 * glX Business
 */
#define bonsaiGlGetProcAddress(procName) glXGetProcAddress((GLubyte*)procName)
typedef PFNGLXSWAPINTERVALEXTPROC PFNSWAPINTERVALPROC;

#define GlobalCwdBufferLength 2048

#define WindowEventMasks StructureNotifyMask | PointerMotionMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask

// FIXME(Jesse): Write own snprintf
#define Snprintf(...) snprintf(__VA_ARGS__)

// In Cygwin printing to the console with printf doesn't work, so we have a
// wrapper that does some additional crazyness on Win32
#define PrintConsole(Message) printf(Message)

typedef int thread_id;
typedef sem_t semaphore;

typedef void* shared_lib;
typedef Window window;

typedef Display* display;
typedef GLXContext gl_context;

typedef pthread_mutex_t native_mutex;


inline void
WakeThread( semaphore *Semaphore )
{
  sem_post(Semaphore);
  return;
}

__inline__ u64
GetCycleCount()
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  u64 Result = ( (u64)lo)|( ((u64)hi)<<32 );
  return Result;
}

void
PlatformDebugStacktrace()
{
  void *StackSymbols[32];
  s32 SymbolCount = backtrace(StackSymbols, 32);
  backtrace_symbols_fd(StackSymbols, SymbolCount, STDERR_FILENO);
  return;
}

inline void
AtomicDecrement( volatile u32 *Source )
{
  __sync_sub_and_fetch( Source, 1 );
  return;
}

inline u64
AtomicIncrement( volatile u64 *Source )
{
  u64 Result = __sync_fetch_and_add( Source, 1 );
  return Result;
}

inline u32
AtomicIncrement( volatile u32 *Source )
{
  u32 Result = __sync_fetch_and_add( Source, 1 );
  return Result;
}

inline void
AtomicDecrement( volatile s32 *Source )
{
  __sync_sub_and_fetch( Source, 1 );
  return;
}

inline void
AtomicIncrement( volatile s32 *Source )
{
  __sync_add_and_fetch( Source, 1 );
  return;
}

inline u32
AtomicExchange( volatile u32 *Source, const u32 Exchange )
{
  u32 Result = __sync_lock_test_and_set( Source, Exchange );
  return Result;
}

inline b32
AtomicCompareExchange( volatile char **Source, volatile char *Exchange, volatile char *Comparator )
{
  bool Result = __sync_bool_compare_and_swap ( Source, Comparator, Exchange );
  return Result;
}

inline bool
AtomicCompareExchange( volatile void** Source, void* Exchange, void* Comparator )
{
  bool Result = __sync_bool_compare_and_swap ( Source, Comparator, Exchange );
  return Result;
}

inline bool
AtomicCompareExchange( volatile u64 *Source, u64 Exchange, u64 Comparator )
{
  bool Result = __sync_bool_compare_and_swap ( Source, Comparator, Exchange );
  return Result;
}

inline bool
AtomicCompareExchange( volatile u32 *Source, u32 Exchange, u32 Comparator )
{
  bool Result = __sync_bool_compare_and_swap ( Source, Comparator, Exchange );
  return Result;
}

void
ReadBytes(u8* Dest, u64 BytesToRead, FILE *Src)
{
  Assert(BytesToRead);
  s64 BytesRead = fread(Dest, 1, BytesToRead, Src);
  Assert(BytesRead != 0);
  return;
}


