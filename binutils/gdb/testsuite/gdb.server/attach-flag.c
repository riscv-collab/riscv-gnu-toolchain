#include <pthread.h>
#include <unistd.h>

static const int NTHREADS = 10;
static pthread_barrier_t barrier;

static void *
thread_func (void *p)
{
  pthread_barrier_wait (&barrier);
  return NULL;
}

int
main (void)
{
  alarm (60);

  pthread_t threads[NTHREADS];
  pthread_barrier_init (&barrier, NULL, NTHREADS + 2);

  for (int i = 0; i < NTHREADS; i++)
    pthread_create (&threads[i], NULL, thread_func, NULL);

  pthread_barrier_wait (&barrier);

  for (int i = 0; i < NTHREADS; i++)
    pthread_join (threads[i], NULL);
}
