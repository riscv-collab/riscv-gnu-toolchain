#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#define CHUNK_SIZE 16000 /* same as findcmd.c's */
#define BUF_SIZE (2 * CHUNK_SIZE) /* at least two chunks */
#define NUMTH 8

int8_t int8_search_buf[100];
int16_t int16_search_buf[100];
int32_t int32_search_buf[100];
int64_t int64_search_buf[100];

static char *search_buf;
static int search_buf_size;

int8_t int8_global = 42;

int f2 (int a)
{
  /* We use a `char[]' type below rather than the typical `char *'
     to make sure that `str' gets allocated on the stack.  Otherwise,
     the compiler may place the "hello, testsuite" string inside
     a read-only section, preventing us from over-writing it from GDB.  */
  char str[] = "hello, testsuite";

  puts (str);	/* Break here.  */

  return ++a;
}

int f1 (int a, int b)
{
  return f2(a) + b;
}

static void
init_bufs ()
{
  search_buf_size = BUF_SIZE;
  search_buf = malloc (search_buf_size);
  if (search_buf == NULL)
    exit (1);
  memset (search_buf, 'x', search_buf_size);
}

static void *
thread (void *param)
{
  pthread_barrier_t *barrier = (pthread_barrier_t *) param;

  pthread_barrier_wait (barrier);

  return param;
}

static void
check_threads (pthread_barrier_t *barrier)
{
  pthread_barrier_wait (barrier);
}

extern int
test_threads (void)
{
  pthread_t threads[NUMTH];
  pthread_barrier_t barrier;
  int i;

  pthread_barrier_init (&barrier, NULL, NUMTH + 1);

  for (i = 0; i < NUMTH; ++i)
    pthread_create (&threads[i], NULL, thread, &barrier);

  check_threads (&barrier);

  for (i = 0; i < NUMTH; ++i)
    pthread_join (threads[i], NULL);

  pthread_barrier_destroy (&barrier);

  return 0;
}

int main (int argc, char *argv[])
{
  test_threads ();
  init_bufs ();

  return f1 (1, 2);
}
