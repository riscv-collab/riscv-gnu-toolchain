/* Test case for setting a memory-write unaligned watchpoint on aarch64.

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely.  */

#define _GNU_SOURCE 1
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <asm/ptrace.h>
#include <assert.h>
#include <sys/wait.h>
#include <stddef.h>
#include <errno.h>
#include <sys/uio.h>
#include <elf.h>
#include <error.h>

static pid_t child;

static void
cleanup (void)
{
  if (child > 0)
    kill (child, SIGKILL);
  child = 0;
}

/* Macros to extract fields from the hardware debug information word.  */
#define AARCH64_DEBUG_NUM_SLOTS(x) ((x) & 0xff)
#define AARCH64_DEBUG_ARCH(x) (((x) >> 8) & 0xff)
/* Macro for the expected version of the ARMv8-A debug architecture.  */
#define AARCH64_DEBUG_ARCH_V8 0x6
#define DR_CONTROL_ENABLED(ctrl)        (((ctrl) & 0x1) == 1)
#define DR_CONTROL_LENGTH(ctrl)         (((ctrl) >> 5) & 0xff)

static void
set_watchpoint (pid_t pid, volatile void *addr, unsigned len_mask)
{
  struct user_hwdebug_state dreg_state;
  struct iovec iov;
  long l;

  assert (len_mask >= 0x01);
  assert (len_mask <= 0xff);

  iov.iov_base = &dreg_state;
  iov.iov_len = sizeof (dreg_state);
  errno = 0;
  l = ptrace (PTRACE_GETREGSET, pid, NT_ARM_HW_WATCH, &iov);
  assert (l == 0);
  assert (AARCH64_DEBUG_ARCH (dreg_state.dbg_info) >= AARCH64_DEBUG_ARCH_V8);
  assert (AARCH64_DEBUG_NUM_SLOTS (dreg_state.dbg_info) >= 1);

  assert (!DR_CONTROL_ENABLED (dreg_state.dbg_regs[0].ctrl));
  dreg_state.dbg_regs[0].ctrl |= 1;
  assert ( DR_CONTROL_ENABLED (dreg_state.dbg_regs[0].ctrl));

  assert (DR_CONTROL_LENGTH (dreg_state.dbg_regs[0].ctrl) == 0);
  dreg_state.dbg_regs[0].ctrl |= len_mask << 5;
  assert (DR_CONTROL_LENGTH (dreg_state.dbg_regs[0].ctrl) == len_mask);

  dreg_state.dbg_regs[0].ctrl |= 2 << 3; // write
  dreg_state.dbg_regs[0].ctrl |= 2 << 1; // enabled at el0
  dreg_state.dbg_regs[0].addr = (uintptr_t) addr;

  iov.iov_base = &dreg_state;
  iov.iov_len = (offsetof (struct user_hwdebug_state, dbg_regs)
                 + sizeof (dreg_state.dbg_regs[0]));
  errno = 0;
  l = ptrace (PTRACE_SETREGSET, pid, NT_ARM_HW_WATCH, &iov);
  if (errno != 0)
    error (1, errno, "PTRACE_SETREGSET: NT_ARM_HW_WATCH");
  assert (l == 0);
}

static volatile long long check;

int
main (void)
{
  pid_t got_pid;
  int i, status;
  long l;

  atexit (cleanup);

  child = fork ();
  assert (child >= 0);
  if (child == 0)
    {
      l = ptrace (PTRACE_TRACEME, 0, NULL, NULL);
      assert (l == 0);
      i = raise (SIGUSR1);
      assert (i == 0);
      check = -1;
      i = raise (SIGUSR2);
      /* NOTREACHED */
      assert (0);
    }

  got_pid = waitpid (child, &status, 0);
  assert (got_pid == child);
  assert (WIFSTOPPED (status));
  assert (WSTOPSIG (status) == SIGUSR1);

  /* Add a watchpoint to check.
     Restart the child. It will write to check.
     Check child has stopped on the watchpoint.  */
  set_watchpoint (child, &check, 0x02);

  errno = 0;
  l = ptrace (PTRACE_CONT, child, 0l, 0l);
  assert_perror (errno);
  assert (l == 0);

  got_pid = waitpid (child, &status, 0);
  assert (got_pid == child);
  assert (WIFSTOPPED (status));
  if (WSTOPSIG (status) == SIGUSR2)
    {
      /* We missed the watchpoint - unsupported by hardware?  */
      cleanup ();
      return 2;
    }
  assert (WSTOPSIG (status) == SIGTRAP);

  return 0;
}
