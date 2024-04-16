
#include <pthread.h>

__thread int i_tls = 1;
int foo ()
{
  /* Ensure we link against pthreads even with --as-needed.  */
  pthread_testcancel();
  return i_tls;
}

