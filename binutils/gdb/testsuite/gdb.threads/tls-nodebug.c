/* Test accessing TLS based variable without any debug info compiled.  */

#include <pthread.h>

__thread int thread_local = 42;

int main(void)
{
  /* Ensure we link against pthreads even with --as-needed.  */
  pthread_testcancel();
  return 0;
}
