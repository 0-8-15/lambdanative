// lambdanative hook
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <fcntl.h>              /* Obtain O_* constant definitions */
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>

#include <LNCONFIG.h>

// ---------------
// lambdanative payload bootstrap

#ifndef PAYLOADONLY

#include <lambdanative.h>

void lambdanative_payload_setup();
void lambdanative_payload_cleanup();
void lambdanative_payload_event(int,int,int);

void lambdanative_exit(int code)
{
  lambdanative_payload_cleanup();
  exit(code);
}
#ifdef STANDALONE
// standalone setup
char **cmd_argv;
int cmd_argc=0;
int main(int argc, char *argv[])
{
  cmd_argc=argc; cmd_argv=argv;
  lambdanative_payload_setup();
  lambdanative_payload_cleanup();
  return 0;
}
#else
// event loop setup
#if defined(ANDROID) || defined(MACOSX) || defined(IOS) || defined(LINUX) || defined(OPENBSD) || defined(BB10) || defined(PLAYBOOK) || defined(NETBSD)
  #define _USE_PTHREAD_FOR_EVENT_LOOP_ 1
  #include <pthread.h>
  pthread_mutex_t ffi_event_lock;
  #define FFI_EVENT_INIT  pthread_mutex_init(&ffi_event_lock, 0);
  #define FFI_EVENT_LOCK  pthread_mutex_lock( &ffi_event_lock);
  #define FFI_EVENT_UNLOCK  pthread_mutex_unlock( &ffi_event_lock);
#else
  #ifdef WIN32
    #define _USE_PTHREAD_FOR_EVENT_LOOP_ 0
    #include <windows.h>
    CRITICAL_SECTION ffi_event_cs;
    #define FFI_EVENT_INIT  InitializeCriticalSection(&ffi_event_cs);
    #define FFI_EVENT_LOCK  EnterCriticalSection(&ffi_event_cs);
    #define FFI_EVENT_UNLOCK LeaveCriticalSection( &ffi_event_cs);
    #include <pthread.h>
  #else
    static int ffi_event_lock;
    #define FFI_EVENT_INIT ffi_event_lock=0;
    #define FFI_EVENT_LOCK { while (ffi_event_lock) { }; ffi_event_lock=1; }
    #define FFI_EVENT_UNLOCK  ffi_event_lock=0;
  #endif
#endif

#if _USE_PTHREAD_FOR_EVENT_LOOP_
/* Note: This is a nice place for extensions.  Alternative
 * implementations could run any other code instead of a gambit
 * program.
 */
static void* gambit_pthread_job(void *arg) {
  ffi_event_params_t* args = (ffi_event_params_t*) arg;
  lambdanative_payload_setup();
  // fprintf(stderr, "delivering inital event %d %d %d\n", args->t, args->x, args->y);
  scm_event(args->t, args->x, args->y);
  return NULL;
}
#endif

void ln_gambit_lock()
{
  FFI_EVENT_LOCK
}
void ln_gambit_unlock()
{
  // fprintf(stderr, "unblocking main\n");
  FFI_EVENT_UNLOCK
}
ffi_event_params_t ffi_event_params;
void ffi_event(int t, int x, int y)
{
  static int lambdanative_needsinit=1;
  ffi_event_params.t=t; ffi_event_params.x=x; ffi_event_params.y=y;
  //fprintf(stderr, "F");
  #if _USE_PTHREAD_FOR_EVENT_LOOP_
    #ifdef WIN32
    // not a good place, but where should it go?
    #define pipe(x) _pipe(x, 256, _O_BINARY)
    #endif
  if(t == EVENT_INIT && lambdanative_needsinit) {
    pthread_t gambit_kernel;
    pthread_attr_t attr;
    FFI_EVENT_INIT
    lambdanative_needsinit=0;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    FFI_EVENT_LOCK
      if(pipe(ffi_event_params.fd)) {
      fprintf(stderr, "pipe failed"); exit(1);
    }
    int res = pthread_create(&gambit_kernel, &attr, gambit_pthread_job, (void*)&ffi_event_params);
    if(res!=0) exit(res);
    FFI_EVENT_LOCK
    FFI_EVENT_UNLOCK
      // fprintf(stderr, "initial EVENT_INIT processed\n");
    return;
  } else {
    //fprintf(stderr, "f");
  FFI_EVENT_LOCK
    write(ffi_event_params.fd[1], &ffi_event_params.t, 1);
  // fsync(ffi_event_params.fd[1]);
  // fprintf(stderr, "delivered event %d %d %d\n", ffi_event_params.t, ffi_event_params.x, ffi_event_params.y);
  FFI_EVENT_LOCK
  if (t==EVENT_TERMINATE) { lambdanative_exit(0); }
  FFI_EVENT_UNLOCK
    //fprintf(stderr, "ffi_event return\n");
    return;
  }
  #else
  if (lambdanative_needsinit) {
      FFI_EVENT_INIT
      lambdanative_needsinit=0;
      lambdanative_payload_setup();
  }
  FFI_EVENT_LOCK
  if (!lambdanative_needsinit&&t) lambdanative_payload_event(t,x,y);
  if (t==EVENT_TERMINATE) { lambdanative_exit(0); }
  FFI_EVENT_UNLOCK
  fprintf(stderr, "ffi_event return\n");
  #endif
}
#endif // STANDALONE

#endif // PAYLOADONLY

// eof
