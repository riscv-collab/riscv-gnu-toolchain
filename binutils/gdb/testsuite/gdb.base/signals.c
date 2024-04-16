/* Test GDB dealing with stuff like stepping into sigtramp.  */

#include <signal.h>
#include <unistd.h>


static int count = 0;

static void
handler (int sig)
{
  signal (sig, handler);
  ++count;
}

static void
func1 ()
{
  ++count;
}

static void
func2 ()
{
  ++count;
}

int
main ()
{
#ifdef SIGALRM
  signal (SIGALRM, handler);
#endif
#ifdef SIGUSR1
  signal (SIGUSR1, handler);
#endif
  alarm (1);
  ++count; /* first */
  alarm (1);
  ++count; /* second */
  func1 ();
  alarm (1);
  func2 ();
  return count;
}
