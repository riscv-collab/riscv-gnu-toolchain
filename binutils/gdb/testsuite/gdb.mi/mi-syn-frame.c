#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

void foo (void);
void bar (void);

void subroutine (int);
void handler (int);
void have_a_very_merry_interrupt (void);

int
main ()
{
  foo ();   /* Put a breakpoint on foo() and call it to see a dummy frame */


  have_a_very_merry_interrupt ();
  return 0;
}

void
foo (void)
{
}

void 
bar (void)
{
  *(volatile char *)0 = 0;    /* try to cause a segfault */

  /* On MMU-less system, previous memory access to address zero doesn't
     trigger a SIGSEGV.  Trigger a SIGILL.  Each arch should define its
     own illegal instruction here.  */
#if defined(__arm__)
  asm(".word 0xf8f00000");
#elif defined(__TMS320C6X__)
  asm(".word 0x56454313");
#else
#endif

}

void
handler (int sig)
{
  subroutine (sig);
}

/* The first statement in subroutine () is a place for a breakpoint.  
   Without it, the breakpoint is put on the while comparison and will
   be hit at each iteration. */

void
subroutine (int in)
{
  int count = in;
  while (count < 100)
    count++;
}

void
have_a_very_merry_interrupt (void)
{
  signal (SIGALRM, handler);
  alarm (1);
  sleep (2);  /* We'll receive that signal while sleeping */
}

