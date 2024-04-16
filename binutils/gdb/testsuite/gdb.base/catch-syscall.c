/* This file is used to test the 'catch syscall' feature on GDB.
 
   Please, if you are going to edit this file DO NOT change the syscalls
   being called (nor the order of them).  If you really must do this, then
   take a look at catch-syscall.exp and modify there too.

   Written by Sergio Durigan Junior <sergiodj@linux.vnet.ibm.com>
   September, 2008 */

#include <unistd.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sched.h>

/* These are the syscalls numbers used by the test.  */

int close_syscall = SYS_close;
int chroot_syscall = SYS_chroot;
/* GDB had a bug where it couldn't catch syscall number 0 (PR 16297).
   In most GNU/Linux architectures, syscall number 0 is
   restart_syscall, which can't be called from userspace.  However,
   the "read" syscall is zero on x86_64.  */
int read_syscall = SYS_read;
#ifdef SYS_pipe
int pipe_syscall = SYS_pipe;
#endif
#ifdef SYS_pipe2
int pipe2_syscall = SYS_pipe2;
#endif
int write_syscall = SYS_write;
#if defined(__arm__)
/* Although 123456789 is an illegal syscall umber on arm linux, kernel
   sends SIGILL rather than returns -ENOSYS.  However, arm linux kernel
   returns -ENOSYS if syscall number is within 0xf0001..0xf07ff, so we
   can use 0xf07ff for unknown_syscall in test.  */
int unknown_syscall = 0x0f07ff;
#else
int unknown_syscall = 123456789;
#endif
#ifdef SYS_exit_group
int exit_group_syscall = SYS_exit_group;
#else
int exit_syscall = SYS_exit;
#endif

/* Set by the test when it wants execve.  */
int do_execve = 0;

int
main (int argc, char *const argv[])
{
	int fd[2];
	char buf1[2] = "a";
	char buf2[2];

	/* Test a simple self-exec, but only on request.  */
	if (do_execve)
	  execv (*argv, argv);

	/* A close() with a wrong argument.  We are only
	   interested in the syscall.  */
	close (-1);

	chroot (".");

	pipe (fd);

	write (fd[1], buf1, sizeof (buf1));
	read (fd[0], buf2, sizeof (buf2));

	/* Test vfork-event interactions.  Child exits immediately.
	   (Plain fork won't work on no-mmu kernel configurations.)  */
	if (vfork () == 0)
	  _exit (0);

	/* Trigger an intentional ENOSYS.  */
	syscall (unknown_syscall);

	/* The last syscall.  Do not change this.  */
	_exit (0);
}
