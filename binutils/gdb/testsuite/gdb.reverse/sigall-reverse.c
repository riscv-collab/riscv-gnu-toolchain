/* This testcase is part of GDB, the GNU debugger.

   Copyright 2009-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <signal.h>
#include <unistd.h>


/* Signal handlers, we set breakpoints in them to make sure that the
   signals really get delivered.  */

void
handle_ABRT (int sig)
{
}

void
handle_HUP (int sig)
{
}

void
handle_QUIT (int sig)
{
}

void
handle_ILL (int sig)
{
}

void
handle_EMT (int sig)
{
}

void
handle_FPE (int sig)
{
}

void
handle_BUS (int sig)
{
}

void
handle_SEGV (int sig)
{
}

void
handle_SYS (int sig)
{
}

void
handle_PIPE (int sig)
{
}

void
handle_ALRM (int sig)
{
}

void
handle_URG (int sig)
{
}

void
handle_TSTP (int sig)
{
}

void
handle_CONT (int sig)
{
}

void
handle_CHLD (int sig)
{
}

void
handle_TTIN (int sig)
{
}

void
handle_TTOU (int sig)
{
}

void
handle_IO (int sig)
{
}

void
handle_XCPU (int sig)
{
}

void
handle_XFSZ (int sig)
{
}

void
handle_VTALRM (int sig)
{
}

void
handle_PROF (int sig)
{
}

void
handle_WINCH (int sig)
{
}

void
handle_LOST (int sig)
{
}

void
handle_USR1 (int sig)
{
}

void
handle_USR2 (int sig)
{
}

void
handle_PWR (int sig)
{
}

void
handle_POLL (int sig)
{
}

void
handle_WIND (int sig)
{
}

void
handle_PHONE (int sig)
{
}

void
handle_WAITING (int sig)
{
}

void
handle_LWP (int sig)
{
}

void
handle_DANGER (int sig)
{
}

void
handle_GRANT (int sig)
{
}

void
handle_RETRACT (int sig)
{
}

void
handle_MSG (int sig)
{
}

void
handle_SOUND (int sig)
{
}

void
handle_SAK (int sig)
{
}

void
handle_PRIO (int sig)
{
}

void
handle_33 (int sig)
{
}

void
handle_34 (int sig)
{
}

void
handle_35 (int sig)
{
}

void
handle_36 (int sig)
{
}

void
handle_37 (int sig)
{
}

void
handle_38 (int sig)
{
}

void
handle_39 (int sig)
{
}

void
handle_40 (int sig)
{
}

void
handle_41 (int sig)
{
}

void
handle_42 (int sig)
{
}

void
handle_43 (int sig)
{
}

void
handle_44 (int sig)
{
}

void
handle_45 (int sig)
{
}

void
handle_46 (int sig)
{
}

void
handle_47 (int sig)
{
}

void
handle_48 (int sig)
{
}

void
handle_49 (int sig)
{
}

void
handle_50 (int sig)
{
}

void
handle_51 (int sig)
{
}

void
handle_52 (int sig)
{
}

void
handle_53 (int sig)
{
}

void
handle_54 (int sig)
{
}

void
handle_55 (int sig)
{
}

void
handle_56 (int sig)
{
}

void
handle_57 (int sig)
{
}

void
handle_58 (int sig)
{
}

void
handle_59 (int sig)
{
}

void
handle_60 (int sig)
{
}

void
handle_61 (int sig)
{
}

void
handle_62 (int sig)
{
}

void
handle_63 (int sig)
{
}

void
handle_TERM (int sig)
{
}

/* Functions to send signals.  These also serve as markers.
   Ordered ANSI-standard signals first, other signals second,
   with signals in each block ordered by their numerical values
   on a typical POSIX platform.  */

/* SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV and SIGTERM
   are ANSI-standard signals and are always available.  */

int
gen_ILL (void)
{
  kill (getpid (), SIGILL);
  return 0;
}

int
gen_ABRT (void)
{
  kill (getpid (), SIGABRT);
  return 0;
}  

int x;

int
gen_FPE (void)
{
  /* The intent behind generating SIGFPE this way is to check the mapping
     from the CPU exception itself to the signals.  It would be nice to
     do the same for SIGBUS, SIGSEGV, etc., but I suspect that even this
     test might turn out to be insufficiently portable.  */

#if 0
  /* Loses on the PA because after the signal handler executes we try to
     re-execute the failing instruction again.  Perhaps we could siglongjmp
     out of the signal handler?  */
  /* The expect script looks for the word "kill"; don't delete it.  */
  return 5 / x; /* and we both started jumping up and down yelling kill */
#else
  kill (getpid (), SIGFPE);
#endif
  return 0;
}

int
gen_SEGV (void)
{
  kill (getpid (), SIGSEGV);
  return 0;
}

int
gen_TERM (void)
{
  kill (getpid (), SIGTERM);
  return 0;
}

/* All other signals need preprocessor conditionals.  */

int
gen_HUP (void)
{
#ifdef SIGHUP
  kill (getpid (), SIGHUP);
#else
  handle_HUP (0);
#endif
return 0;
}  

int
gen_QUIT (void)
{
#ifdef SIGQUIT
  kill (getpid (), SIGQUIT);
#else
  handle_QUIT (0);
#endif
return 0;
}

int
gen_EMT (void)
{
#ifdef SIGEMT
  kill (getpid (), SIGEMT);
#else
  handle_EMT (0);
#endif
return 0;
}

int
gen_BUS (void)
{
#ifdef SIGBUS
  kill (getpid (), SIGBUS);
#else
  handle_BUS (0);
#endif
return 0;
}

int
gen_SYS (void)
{
#ifdef SIGSYS
  kill (getpid (), SIGSYS);
#else
  handle_SYS (0);
#endif
return 0;
}

int
gen_PIPE (void)
{
#ifdef SIGPIPE
  kill (getpid (), SIGPIPE);
#else
  handle_PIPE (0);
#endif
return 0;
}

int
gen_ALRM (void)
{
#ifdef SIGALRM
  kill (getpid (), SIGALRM);
#else
  handle_ALRM (0);
#endif
return 0;
}

int
gen_URG (void)
{
#ifdef SIGURG
  kill (getpid (), SIGURG);
#else
  handle_URG (0);
#endif
return 0;
}

int
gen_TSTP (void)
{
#ifdef SIGTSTP
  kill (getpid (), SIGTSTP);
#else
  handle_TSTP (0);
#endif
return 0;
}

int
gen_CONT (void)
{
#ifdef SIGCONT
  kill (getpid (), SIGCONT);
#else
  handle_CONT (0);
#endif
return 0;
}

int
gen_CHLD (void)
{
#ifdef SIGCHLD
  kill (getpid (), SIGCHLD);
#else
  handle_CHLD (0);
#endif
return 0;
}

int
gen_TTIN (void)
{
#ifdef SIGTTIN
  kill (getpid (), SIGTTIN);
#else
  handle_TTIN (0);
#endif
return 0;
}

int
gen_TTOU (void)
{
#ifdef SIGTTOU
  kill (getpid (), SIGTTOU);
#else
  handle_TTOU (0);
#endif
return 0;
}

int
gen_IO (void)
{
#ifdef SIGIO
  kill (getpid (), SIGIO);
#else
  handle_IO (0);
#endif
return 0;
}

int
gen_XCPU (void)
{
#ifdef SIGXCPU
  kill (getpid (), SIGXCPU);
#else
  handle_XCPU (0);
#endif
return 0;
}

int
gen_XFSZ (void)
{
#ifdef SIGXFSZ
  kill (getpid (), SIGXFSZ);
#else
  handle_XFSZ (0);
#endif
return 0;
}

int
gen_VTALRM (void)
{
#ifdef SIGVTALRM
  kill (getpid (), SIGVTALRM);
#else
  handle_VTALRM (0);
#endif
return 0;
}

int
gen_PROF (void)
{
#ifdef SIGPROF
  kill (getpid (), SIGPROF);
#else
  handle_PROF (0);
#endif
return 0;
}

int
gen_WINCH (void)
{
#ifdef SIGWINCH
  kill (getpid (), SIGWINCH);
#else
  handle_WINCH (0);
#endif
return 0;
}

int
gen_LOST (void)
{
#if defined(SIGLOST) && SIGLOST != SIGABRT
  kill (getpid (), SIGLOST);
#else
  handle_LOST (0);
#endif
return 0;
}

int
gen_USR1 (void)
{
#ifdef SIGUSR1
  kill (getpid (), SIGUSR1);
#else
  handle_USR1 (0);
#endif
return 0;
}

int
gen_USR2 (void)
{
#ifdef SIGUSR2
  kill (getpid (), SIGUSR2);
#else
  handle_USR2 (0);
#endif
return 0;
}  

int
gen_PWR (void)
{
#ifdef SIGPWR
  kill (getpid (), SIGPWR);
#else
  handle_PWR (0);
#endif
return 0;
}

int
gen_POLL (void)
{
#if defined (SIGPOLL) && (!defined (SIGIO) || SIGPOLL != SIGIO)
  kill (getpid (), SIGPOLL);
#else
  handle_POLL (0);
#endif
return 0;
}

int
gen_WIND (void)
{
#ifdef SIGWIND
  kill (getpid (), SIGWIND);
#else
  handle_WIND (0);
#endif
return 0;
}

int
gen_PHONE (void)
{
#ifdef SIGPHONE
  kill (getpid (), SIGPHONE);
#else
  handle_PHONE (0);
#endif
return 0;
}

int
gen_WAITING (void)
{
#ifdef SIGWAITING
  kill (getpid (), SIGWAITING);
#else
  handle_WAITING (0);
#endif
return 0;
}

int
gen_LWP (void)
{
#ifdef SIGLWP
  kill (getpid (), SIGLWP);
#else
  handle_LWP (0);
#endif
return 0;
}

int
gen_DANGER (void)
{
#ifdef SIGDANGER
  kill (getpid (), SIGDANGER);
#else
  handle_DANGER (0);
#endif
return 0;
}

int
gen_GRANT (void)
{
#ifdef SIGGRANT
  kill (getpid (), SIGGRANT);
#else
  handle_GRANT (0);
#endif
return 0;
}

int
gen_RETRACT (void)
{
#ifdef SIGRETRACT
  kill (getpid (), SIGRETRACT);
#else
  handle_RETRACT (0);
#endif
return 0;
}

int
gen_MSG (void)
{
#ifdef SIGMSG
  kill (getpid (), SIGMSG);
#else
  handle_MSG (0);
#endif
return 0;
}

int
gen_SOUND (void)
{
#ifdef SIGSOUND
  kill (getpid (), SIGSOUND);
#else
  handle_SOUND (0);
#endif
return 0;
}

int
gen_SAK (void)
{
#ifdef SIGSAK
  kill (getpid (), SIGSAK);
#else
  handle_SAK (0);
#endif
return 0;
}

int
gen_PRIO (void)
{
#ifdef SIGPRIO
  kill (getpid (), SIGPRIO);
#else
  handle_PRIO (0);
#endif
return 0;
}

int
gen_33 (void)
{
#ifdef SIG33
  kill (getpid (), 33);
#else
  handle_33 (0);
#endif
return 0;
}

int
gen_34 (void)
{
#ifdef SIG34
  kill (getpid (), 34);
#else
  handle_34 (0);
#endif
return 0;
}

int
gen_35 (void)
{
#ifdef SIG35
  kill (getpid (), 35);
#else
  handle_35 (0);
#endif
return 0;
}

int
gen_36 (void)
{
#ifdef SIG36
  kill (getpid (), 36);
#else
  handle_36 (0);
#endif
return 0;
}

int
gen_37 (void)
{
#ifdef SIG37
  kill (getpid (), 37);
#else
  handle_37 (0);
#endif
return 0;
}

int
gen_38 (void)
{
#ifdef SIG38
  kill (getpid (), 38);
#else
  handle_38 (0);
#endif
return 0;
}

int
gen_39 (void)
{
#ifdef SIG39
  kill (getpid (), 39);
#else
  handle_39 (0);
#endif
return 0;
}

int
gen_40 (void)
{
#ifdef SIG40
  kill (getpid (), 40);
#else
  handle_40 (0);
#endif
return 0;
}

int
gen_41 (void)
{
#ifdef SIG41
  kill (getpid (), 41);
#else
  handle_41 (0);
#endif
return 0;
}

int
gen_42 (void)
{
#ifdef SIG42
  kill (getpid (), 42);
#else
  handle_42 (0);
#endif
return 0;
}

int
gen_43 (void)
{
#ifdef SIG43
  kill (getpid (), 43);
#else
  handle_43 (0);
#endif
return 0;
}

int
gen_44 (void)
{
#ifdef SIG44
  kill (getpid (), 44);
#else
  handle_44 (0);
#endif
return 0;
}

int
gen_45 (void)
{
#ifdef SIG45
  kill (getpid (), 45);
#else
  handle_45 (0);
#endif
return 0;
}

int
gen_46 (void)
{
#ifdef SIG46
  kill (getpid (), 46);
#else
  handle_46 (0);
#endif
return 0;
}

int
gen_47 (void)
{
#ifdef SIG47
  kill (getpid (), 47);
#else
  handle_47 (0);
#endif
return 0;
}

int
gen_48 (void)
{
#ifdef SIG48
  kill (getpid (), 48);
#else
  handle_48 (0);
#endif
return 0;
}

int
gen_49 (void)
{
#ifdef SIG49
  kill (getpid (), 49);
#else
  handle_49 (0);
#endif
return 0;
}

int
gen_50 (void)
{
#ifdef SIG50
  kill (getpid (), 50);
#else
  handle_50 (0);
#endif
return 0;
}

int
gen_51 (void)
{
#ifdef SIG51
  kill (getpid (), 51);
#else
  handle_51 (0);
#endif
return 0;
}

int
gen_52 (void)
{
#ifdef SIG52
  kill (getpid (), 52);
#else
  handle_52 (0);
#endif
return 0;
}

int
gen_53 (void)
{
#ifdef SIG53
  kill (getpid (), 53);
#else
  handle_53 (0);
#endif
return 0;
}

int
gen_54 (void)
{
#ifdef SIG54
  kill (getpid (), 54);
#else
  handle_54 (0);
#endif
return 0;
}

int
gen_55 (void)
{
#ifdef SIG55
  kill (getpid (), 55);
#else
  handle_55 (0);
#endif
return 0;
}

int
gen_56 (void)
{
#ifdef SIG56
  kill (getpid (), 56);
#else
  handle_56 (0);
#endif
return 0;
}

int
gen_57 (void)
{
#ifdef SIG57
  kill (getpid (), 57);
#else
  handle_57 (0);
#endif
return 0;
}

int
gen_58 (void)
{
#ifdef SIG58
  kill (getpid (), 58);
#else
  handle_58 (0);
#endif
return 0;
}

int
gen_59 (void)
{
#ifdef SIG59
  kill (getpid (), 59);
#else
  handle_59 (0);
#endif
return 0;
}

int
gen_60 (void)
{
#ifdef SIG60
  kill (getpid (), 60);
#else
  handle_60 (0);
#endif
return 0;
}

int
gen_61 (void)
{
#ifdef SIG61
  kill (getpid (), 61);
#else
  handle_61 (0);
#endif
return 0;
}

int
gen_62 (void)
{
#ifdef SIG62
  kill (getpid (), 62);
#else
  handle_62 (0);
#endif
return 0;
}

int
gen_63 (void)
{
#ifdef SIG63
  kill (getpid (), 63);
#else
  handle_63 (0);
#endif
return 0;
}

int
main ()
{
#ifdef SIG_SETMASK
  /* Ensure all the signals aren't blocked.
     The environment in which the testsuite is run may have blocked some
     for whatever reason.  */
  {
    sigset_t newset;
    sigemptyset (&newset);
    sigprocmask (SIG_SETMASK, &newset, NULL);
  }
#endif

  /* Signals are ordered ANSI-standard signals first, other signals
     second, with signals in each block ordered by their numerical
     values on a typical POSIX platform.  */

  /* SIGINT, SIGILL, SIGABRT, SIGFPE, SIGSEGV and SIGTERM
     are ANSI-standard signals and are always available.  */
  signal (SIGILL, handle_ILL);
  signal (SIGABRT, handle_ABRT);
  signal (SIGFPE, handle_FPE);
  signal (SIGSEGV, handle_SEGV);
  signal (SIGTERM, handle_TERM);

  /* All other signals need preprocessor conditionals.  */
#ifdef SIGHUP
  signal (SIGHUP, handle_HUP);
#endif
#ifdef SIGQUIT
  signal (SIGQUIT, handle_QUIT);
#endif
#ifdef SIGEMT
  signal (SIGEMT, handle_EMT);
#endif
#ifdef SIGBUS
  signal (SIGBUS, handle_BUS);
#endif
#ifdef SIGSYS
  signal (SIGSYS, handle_SYS);
#endif
#ifdef SIGPIPE
  signal (SIGPIPE, handle_PIPE);
#endif
#ifdef SIGALRM
  signal (SIGALRM, handle_ALRM);
#endif
#ifdef SIGURG
  signal (SIGURG, handle_URG);
#endif
#ifdef SIGTSTP
  signal (SIGTSTP, handle_TSTP);
#endif
#ifdef SIGCONT
  signal (SIGCONT, handle_CONT);
#endif
#ifdef SIGCHLD
  signal (SIGCHLD, handle_CHLD);
#endif
#ifdef SIGTTIN
  signal (SIGTTIN, handle_TTIN);
#endif
#ifdef SIGTTOU
  signal (SIGTTOU, handle_TTOU);
#endif
#ifdef SIGIO
  signal (SIGIO, handle_IO);
#endif
#ifdef SIGXCPU
  signal (SIGXCPU, handle_XCPU);
#endif
#ifdef SIGXFSZ
  signal (SIGXFSZ, handle_XFSZ);
#endif
#ifdef SIGVTALRM
  signal (SIGVTALRM, handle_VTALRM);
#endif
#ifdef SIGPROF
  signal (SIGPROF, handle_PROF);
#endif
#ifdef SIGWINCH
  signal (SIGWINCH, handle_WINCH);
#endif
#if defined(SIGLOST) && SIGLOST != SIGABRT
  signal (SIGLOST, handle_LOST);
#endif
#ifdef SIGUSR1
  signal (SIGUSR1, handle_USR1);
#endif
#ifdef SIGUSR2
  signal (SIGUSR2, handle_USR2);
#endif
#ifdef SIGPWR
  signal (SIGPWR, handle_PWR);
#endif
#if defined (SIGPOLL) && (!defined (SIGIO) || SIGPOLL != SIGIO)
  signal (SIGPOLL, handle_POLL);
#endif
#ifdef SIGWIND
  signal (SIGWIND, handle_WIND);
#endif
#ifdef SIGPHONE
  signal (SIGPHONE, handle_PHONE);
#endif
#ifdef SIGWAITING
  signal (SIGWAITING, handle_WAITING);
#endif
#ifdef SIGLWP
  signal (SIGLWP, handle_LWP);
#endif
#ifdef SIGDANGER
  signal (SIGDANGER, handle_DANGER);
#endif
#ifdef SIGGRANT
  signal (SIGGRANT, handle_GRANT);
#endif
#ifdef SIGRETRACT
  signal (SIGRETRACT, handle_RETRACT);
#endif
#ifdef SIGMSG
  signal (SIGMSG, handle_MSG);
#endif
#ifdef SIGSOUND
  signal (SIGSOUND, handle_SOUND);
#endif
#ifdef SIGSAK
  signal (SIGSAK, handle_SAK);
#endif
#ifdef SIGPRIO
  signal (SIGPRIO, handle_PRIO);
#endif
#ifdef __Lynx__
  /* Lynx doesn't seem to have anything in signal.h for this.  */
  signal (33, handle_33);
  signal (34, handle_34);
  signal (35, handle_35);
  signal (36, handle_36);
  signal (37, handle_37);
  signal (38, handle_38);
  signal (39, handle_39);
  signal (40, handle_40);
  signal (41, handle_41);
  signal (42, handle_42);
  signal (43, handle_43);
  signal (44, handle_44);
  signal (45, handle_45);
  signal (46, handle_46);
  signal (47, handle_47);
  signal (48, handle_48);
  signal (49, handle_49);
  signal (50, handle_50);
  signal (51, handle_51);
  signal (52, handle_52);
  signal (53, handle_53);
  signal (54, handle_54);
  signal (55, handle_55);
  signal (56, handle_56);
  signal (57, handle_57);
  signal (58, handle_58);
  signal (59, handle_59);
  signal (60, handle_60);
  signal (61, handle_61);
  signal (62, handle_62);
  signal (63, handle_63);
#endif /* lynx */

  x = 0;

  gen_ABRT ();
  gen_HUP ();
  gen_QUIT ();
  gen_ILL ();
  gen_EMT ();
  gen_FPE ();
  gen_BUS ();
  gen_SEGV ();
  gen_SYS ();
  gen_PIPE ();
  gen_ALRM ();
  gen_URG ();
  gen_TSTP ();
  gen_CONT ();
  gen_CHLD ();
  gen_TTIN ();
  gen_TTOU ();
  gen_IO ();
  gen_XCPU ();
  gen_XFSZ ();
  gen_VTALRM ();
  gen_PROF ();
  gen_WINCH ();
  gen_LOST ();
  gen_USR1 ();
  gen_USR2 ();
  gen_PWR ();
  gen_POLL ();
  gen_WIND ();
  gen_PHONE ();
  gen_WAITING ();
  gen_LWP ();
  gen_DANGER ();
  gen_GRANT ();
  gen_RETRACT ();
  gen_MSG ();
  gen_SOUND ();
  gen_SAK ();
  gen_PRIO ();
  gen_33 ();
  gen_34 ();
  gen_35 ();
  gen_36 ();
  gen_37 ();
  gen_38 ();
  gen_39 ();
  gen_40 ();
  gen_41 ();
  gen_42 ();
  gen_43 ();
  gen_44 ();
  gen_45 ();
  gen_46 ();
  gen_47 ();
  gen_48 ();
  gen_49 ();
  gen_50 ();
  gen_51 ();
  gen_52 ();
  gen_53 ();
  gen_54 ();
  gen_55 ();
  gen_56 ();
  gen_57 ();
  gen_58 ();
  gen_59 ();
  gen_60 ();
  gen_61 ();
  gen_62 ();
  gen_63 ();
  gen_TERM ();

  return 0;	/* end of main */
}
