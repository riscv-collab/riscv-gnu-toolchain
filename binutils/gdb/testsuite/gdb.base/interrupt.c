#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "unbuffer_output.c"

#ifdef SIGNALS
#include <signal.h>

static void
sigint_handler (int signo)
{
}
#endif

int
main ()
{
  char x;
  int nbytes;

  gdb_unbuffer_output ();

#ifdef SIGNALS
  signal (SIGINT, sigint_handler);
#endif
  printf ("talk to me baby\n");
  while (1)
    {
      nbytes = read (0, &x, 1);
      if (nbytes < 0)
	{
#ifdef EINTR
	  if (errno != EINTR)
#endif
	    {
	      perror ("");
	      return 1;
	    }
	}
      else if (nbytes == 0)
	{
	  printf ("end of file\n");
	  exit (0);
	}
      else
	write (1, &x, 1);
    }
  return 0;
}

int
func1 ()
{
  return 4;
}
