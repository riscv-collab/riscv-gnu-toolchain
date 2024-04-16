/* m32r exception, interrupt, and trap (EIT) support
   Copyright (C) 1998-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions & Renesas.

   This file is part of GDB, the GNU debugger.

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

/* This must come before any other includes.  */
#include "defs.h"

#include "portability.h"
#include "sim-main.h"
#include "sim-signal.h"
#include "sim-syscall.h"
#include "sim/callback.h"
#include "syscall.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
/* TODO: The Linux syscall emulation needs work to support non-Linux hosts.
   Use an OS hack for now so the CPU emulation is available everywhere.
   NB: The emulation is also missing argument conversion (endian & bitsize)
   even on Linux hosts.  */
#ifdef __linux__
#include <syslog.h>
#include <sys/file.h>
#include <sys/fsuid.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/resource.h>
#include <sys/sendfile.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/timex.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/utsname.h>
#include <sys/vfs.h>
#include <linux/sysctl.h>
#include <linux/types.h>
#include <linux/unistd.h>
#endif

#include "m32r-sim.h"

#define TRAP_LINUX_SYSCALL 2
#define TRAP_FLUSH_CACHE 12
/* The semantic code invokes this for invalid (unrecognized) instructions.  */

SEM_PC
sim_engine_invalid_insn (SIM_CPU *current_cpu, IADDR cia, SEM_PC pc)
{
  SIM_DESC sd = CPU_STATE (current_cpu);

#if 0
  if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
    {
      h_bsm_set (current_cpu, h_sm_get (current_cpu));
      h_bie_set (current_cpu, h_ie_get (current_cpu));
      h_bcond_set (current_cpu, h_cond_get (current_cpu));
      /* sm not changed */
      h_ie_set (current_cpu, 0);
      h_cond_set (current_cpu, 0);

      h_bpc_set (current_cpu, cia);

      sim_engine_restart (CPU_STATE (current_cpu), current_cpu, NULL,
			  EIT_RSVD_INSN_ADDR);
    }
  else
#endif
    sim_engine_halt (sd, current_cpu, NULL, cia, sim_stopped, SIM_SIGILL);

  return pc;
}

/* Process an address exception.  */

void
m32r_core_signal (SIM_DESC sd, SIM_CPU *current_cpu, sim_cia cia,
		  unsigned int map, int nr_bytes, address_word addr,
		  transfer_type transfer, sim_core_signals sig)
{
  if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
    {
      m32rbf_h_cr_set (current_cpu, H_CR_BBPC,
		       m32rbf_h_cr_get (current_cpu, H_CR_BPC));
      switch (MACH_NUM (CPU_MACH (current_cpu)))
	{
	case MACH_M32R:
	  m32rbf_h_bpsw_set (current_cpu, m32rbf_h_psw_get (current_cpu));
	  /* sm not changed.  */
	  m32rbf_h_psw_set (current_cpu, m32rbf_h_psw_get (current_cpu) & 0x80);
	  break;
	case MACH_M32RX:
  	  m32rxf_h_bpsw_set (current_cpu, m32rxf_h_psw_get (current_cpu));
  	  /* sm not changed.  */
  	  m32rxf_h_psw_set (current_cpu, m32rxf_h_psw_get (current_cpu) & 0x80);
	  break;
	case MACH_M32R2:
	  m32r2f_h_bpsw_set (current_cpu, m32r2f_h_psw_get (current_cpu));
	  /* sm not changed.  */
	  m32r2f_h_psw_set (current_cpu, m32r2f_h_psw_get (current_cpu) & 0x80);
	  break;
	default:
	  abort ();
	}

      m32rbf_h_cr_set (current_cpu, H_CR_BPC, cia);

      sim_engine_restart (CPU_STATE (current_cpu), current_cpu, NULL,
			  EIT_ADDR_EXCP_ADDR);
    }
  else
    sim_core_signal (sd, current_cpu, cia, map, nr_bytes, addr,
		     transfer, sig);
}

/* Translate target's address to host's address.  */

static void *
t2h_addr (host_callback *cb, struct cb_syscall *sc,
	  unsigned long taddr)
{
  SIM_DESC sd = (SIM_DESC) sc->p1;
  SIM_CPU *cpu = (SIM_CPU *) sc->p2;

  if (taddr == 0)
    return NULL;

  return sim_core_trans_addr (sd, cpu, read_map, taddr);
}

/* TODO: These functions are a big hack and assume that the host runtime has
   type sizes and struct layouts that match the target.  So the Linux emulation
   probaly only really works in 32-bit runtimes.  */

static void
translate_endian_h2t (void *addr, size_t size)
{
  unsigned int *p = (unsigned int *) addr;
  int i;

  for (i = 0; i <= size - 4; i += 4,p++)
    *p = H2T_4 (*p);

  if (i <= size - 2)
    *((unsigned short *) p) = H2T_2 (*((unsigned short *) p));
}

static void
translate_endian_t2h (void *addr, size_t size)
{
  unsigned int *p = (unsigned int *) addr;
  int i;

  for (i = 0; i <= size - 4; i += 4,p++)
    *p = T2H_4 (*p);

  if (i <= size - 2)
    *((unsigned short *) p) = T2H_2 (*((unsigned short *) p));
}

/* Trap support.
   The result is the pc address to continue at.
   Preprocessing like saving the various registers has already been done.  */

USI
m32r_trap (SIM_CPU *current_cpu, PCADDR pc, int num)
{
  SIM_DESC sd = CPU_STATE (current_cpu);
  host_callback *cb = STATE_CALLBACK (sd);

  if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT)
    goto case_default;

  switch (num)
    {
    case TRAP_SYSCALL:
      {
	long result, result2;
	int errcode;

	sim_syscall_multi (current_cpu,
			   m32rbf_h_gr_get (current_cpu, 0),
			   m32rbf_h_gr_get (current_cpu, 1),
			   m32rbf_h_gr_get (current_cpu, 2),
			   m32rbf_h_gr_get (current_cpu, 3),
			   m32rbf_h_gr_get (current_cpu, 4),
			   &result, &result2, &errcode);

	m32rbf_h_gr_set (current_cpu, 2, errcode);
	m32rbf_h_gr_set (current_cpu, 0, result);
	m32rbf_h_gr_set (current_cpu, 1, result2);
	break;
      }

#ifdef __linux__
    case TRAP_LINUX_SYSCALL:
      {
	CB_SYSCALL s;
	unsigned int func, arg1, arg2, arg3, arg4, arg5, arg6, arg7;
	int result, errcode;

	if (STATE_ENVIRONMENT (sd) != USER_ENVIRONMENT)
	  goto case_default;

	func = m32rbf_h_gr_get (current_cpu, 7);
	arg1 = m32rbf_h_gr_get (current_cpu, 0);
	arg2 = m32rbf_h_gr_get (current_cpu, 1);
	arg3 = m32rbf_h_gr_get (current_cpu, 2);
	arg4 = m32rbf_h_gr_get (current_cpu, 3);
	arg5 = m32rbf_h_gr_get (current_cpu, 4);
	arg6 = m32rbf_h_gr_get (current_cpu, 5);
	arg7 = m32rbf_h_gr_get (current_cpu, 6);

	CB_SYSCALL_INIT (&s);
	s.func = func;
	s.arg1 = arg1;
	s.arg2 = arg2;
	s.arg3 = arg3;
	s.arg4 = arg4;
	s.arg5 = arg5;
	s.arg6 = arg6;
	s.arg7 = arg7;

	s.p1 = sd;
	s.p2 = current_cpu;
	s.read_mem = sim_syscall_read_mem;
	s.write_mem = sim_syscall_write_mem;

	result = 0;
	errcode = 0;

	switch (func)
	  {
	  case TARGET_LINUX_SYS_exit:
	    sim_engine_halt (sd, current_cpu, NULL, pc, sim_exited, arg1);
	    break;

	  case TARGET_LINUX_SYS_read:
	    result = read (arg1, t2h_addr (cb, &s, arg2), arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_write:
	    result = write (arg1, t2h_addr (cb, &s, arg2), arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_open:
	    result = open ((char *) t2h_addr (cb, &s, arg1), arg2, arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_close:
	    result = close (arg1);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_creat:
	    result = creat ((char *) t2h_addr (cb, &s, arg1), arg2);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_link:
	    result = link ((char *) t2h_addr (cb, &s, arg1),
			   (char *) t2h_addr (cb, &s, arg2));
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_unlink:
	    result = unlink ((char *) t2h_addr (cb, &s, arg1));
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_chdir:
	    result = chdir ((char *) t2h_addr (cb, &s, arg1));
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_time:
	    {
	      time_t t;

	      if (arg1 == 0)
		{
		  result = (int) time (NULL);
		  errcode = errno;
		}
	      else
		{
		  result = (int) time (&t);
		  errcode = errno;

		  if (result != 0)
		    break;

		  t = H2T_4 (t);
		  if ((s.write_mem) (cb, &s, arg1, (char *) &t, sizeof(t)) != sizeof(t))
		    {
		      result = -1;
		      errcode = EINVAL;
		    }
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_mknod:
	    result = mknod ((char *) t2h_addr (cb, &s, arg1),
			    (mode_t) arg2, (dev_t) arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_chmod:
	    result = chmod ((char *) t2h_addr (cb, &s, arg1), (mode_t) arg2);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_lchown32:
	  case TARGET_LINUX_SYS_lchown:
	    result = lchown ((char *) t2h_addr (cb, &s, arg1),
			     (uid_t) arg2, (gid_t) arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_lseek:
	    result = (int) lseek (arg1, (off_t) arg2, arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_getpid:
	    result = getpid ();
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_getuid32:
	  case TARGET_LINUX_SYS_getuid:
	    result = getuid ();
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_utime:
	    {
	      struct utimbuf buf;

	      if (arg2 == 0)
		{
		  result = utime ((char *) t2h_addr (cb, &s, arg1), NULL);
		  errcode = errno;
		}
	      else
		{
		  buf = *((struct utimbuf *) t2h_addr (cb, &s, arg2));
		  translate_endian_t2h (&buf, sizeof(buf));
		  result = utime ((char *) t2h_addr (cb, &s, arg1), &buf);
		  errcode = errno;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_access:
	    result = access ((char *) t2h_addr (cb, &s, arg1), arg2);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_ftime:
	    {
	      struct timeb t;
	      struct timespec ts;

	      result = clock_gettime (CLOCK_REALTIME, &ts);
	      errcode = errno;

	      if (result != 0)
		break;

	      t.time = H2T_4 (ts.tv_sec);
	      t.millitm = H2T_2 (ts.tv_nsec / 1000000);
	      /* POSIX.1-2001 says the contents of the timezone and dstflag
		 members of tp after a call to ftime() are unspecified.  */
	      t.timezone = H2T_2 (0);
	      t.dstflag = H2T_2 (0);
	      if ((s.write_mem) (cb, &s, arg1, (char *) &t, sizeof(t))
		  != sizeof(t))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_sync:
	    sync ();
	    result = 0;
	    break;

	  case TARGET_LINUX_SYS_rename:
	    result = rename ((char *) t2h_addr (cb, &s, arg1),
			     (char *) t2h_addr (cb, &s, arg2));
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_mkdir:
	    result = mkdir ((char *) t2h_addr (cb, &s, arg1), arg2);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_rmdir:
	    result = rmdir ((char *) t2h_addr (cb, &s, arg1));
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_dup:
	    result = dup (arg1);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_brk:
	    result = brk ((void *) (uintptr_t) arg1);
	    errcode = errno;
	    //result = arg1;
	    break;

	  case TARGET_LINUX_SYS_getgid32:
	  case TARGET_LINUX_SYS_getgid:
	    result = getgid ();
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_geteuid32:
	  case TARGET_LINUX_SYS_geteuid:
	    result = geteuid ();
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_getegid32:
	  case TARGET_LINUX_SYS_getegid:
	    result = getegid ();
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_ioctl:
	    result = ioctl (arg1, arg2, arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_fcntl:
	    result = fcntl (arg1, arg2, arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_dup2:
	    result = dup2 (arg1, arg2);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_getppid:
	    result = getppid ();
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_getpgrp:
	    result = getpgrp ();
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_getrlimit:
	    {
	      struct rlimit rlim;

	      result = getrlimit (arg1, &rlim);
	      errcode = errno;

	      if (result != 0)
		break;

	      translate_endian_h2t (&rlim, sizeof(rlim));
	      if ((s.write_mem) (cb, &s, arg2, (char *) &rlim, sizeof(rlim))
		  != sizeof(rlim))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_getrusage:
	    {
	      struct rusage usage;

	      result = getrusage (arg1, &usage);
	      errcode = errno;

	      if (result != 0)
		break;

	      translate_endian_h2t (&usage, sizeof(usage));
	      if ((s.write_mem) (cb, &s, arg2, (char *) &usage, sizeof(usage))
		  != sizeof(usage))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_gettimeofday:
	    {
	      struct timeval tv;
	      struct timezone tz;

	      result = gettimeofday (&tv, &tz);
	      errcode = errno;

	      if (result != 0)
		break;

	      translate_endian_h2t (&tv, sizeof(tv));
	      if ((s.write_mem) (cb, &s, arg1, (char *) &tv, sizeof(tv))
		  != sizeof(tv))
		{
		  result = -1;
		  errcode = EINVAL;
		}

	      translate_endian_h2t (&tz, sizeof(tz));
	      if ((s.write_mem) (cb, &s, arg2, (char *) &tz, sizeof(tz))
		  != sizeof(tz))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_getgroups32:
	  case TARGET_LINUX_SYS_getgroups:
	    {
	      gid_t *list = NULL;

	      if (arg1 > 0)
		list = (gid_t *) malloc (arg1 * sizeof(gid_t));

	      result = getgroups (arg1, list);
	      errcode = errno;

	      if (result != 0)
		break;

	      translate_endian_h2t (list, arg1 * sizeof(gid_t));
	      if (arg1 > 0)
		if ((s.write_mem) (cb, &s, arg2, (char *) list, arg1 * sizeof(gid_t))
		    != arg1 * sizeof(gid_t))
		  {
		    result = -1;
		     errcode = EINVAL;
		  }
	    }
	    break;

	  case TARGET_LINUX_SYS_select:
	    {
	      int n;
	      fd_set readfds;
	      unsigned int treadfdsp;
	      fd_set *hreadfdsp;
	      fd_set writefds;
	      unsigned int twritefdsp;
	      fd_set *hwritefdsp;
	      fd_set exceptfds;
	      unsigned int texceptfdsp;
	      fd_set *hexceptfdsp;
	      unsigned int ttimeoutp;
	      struct timeval timeout;

	      n = arg1;

	      treadfdsp = arg2;
	      if (treadfdsp !=0)
		{
		  readfds = *((fd_set *) t2h_addr (cb, &s, treadfdsp));
		  translate_endian_t2h (&readfds, sizeof(readfds));
		  hreadfdsp = &readfds;
		}
	      else
		hreadfdsp = NULL;

	      twritefdsp = arg3;
	      if (twritefdsp != 0)
		{
		  writefds = *((fd_set *) t2h_addr (cb, &s, twritefdsp));
		  translate_endian_t2h (&writefds, sizeof(writefds));
		  hwritefdsp = &writefds;
		}
	      else
		hwritefdsp = NULL;

	      texceptfdsp = arg4;
	      if (texceptfdsp != 0)
		{
		  exceptfds = *((fd_set *) t2h_addr (cb, &s, texceptfdsp));
		  translate_endian_t2h (&exceptfds, sizeof(exceptfds));
		  hexceptfdsp = &exceptfds;
		}
	      else
		hexceptfdsp = NULL;

	      ttimeoutp = arg5;
	      timeout = *((struct timeval *) t2h_addr (cb, &s, ttimeoutp));
	      translate_endian_t2h (&timeout, sizeof(timeout));

	      result = select (n, hreadfdsp, hwritefdsp, hexceptfdsp, &timeout);
	      errcode = errno;

	      if (result != 0)
		break;

	      if (treadfdsp != 0)
		{
		  translate_endian_h2t (&readfds, sizeof(readfds));
		  if ((s.write_mem) (cb, &s, treadfdsp,
		       (char *) &readfds, sizeof(readfds)) != sizeof(readfds))
		    {
		      result = -1;
		      errcode = EINVAL;
		    }
		}

	      if (twritefdsp != 0)
		{
		  translate_endian_h2t (&writefds, sizeof(writefds));
		  if ((s.write_mem) (cb, &s, twritefdsp,
		       (char *) &writefds, sizeof(writefds)) != sizeof(writefds))
		    {
		      result = -1;
		      errcode = EINVAL;
		    }
		}

	      if (texceptfdsp != 0)
		{
		  translate_endian_h2t (&exceptfds, sizeof(exceptfds));
		  if ((s.write_mem) (cb, &s, texceptfdsp,
		       (char *) &exceptfds, sizeof(exceptfds)) != sizeof(exceptfds))
		    {
		      result = -1;
		      errcode = EINVAL;
		    }
		}

	      translate_endian_h2t (&timeout, sizeof(timeout));
	      if ((s.write_mem) (cb, &s, ttimeoutp,
		   (char *) &timeout, sizeof(timeout)) != sizeof(timeout))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_symlink:
	    result = symlink ((char *) t2h_addr (cb, &s, arg1),
			      (char *) t2h_addr (cb, &s, arg2));
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_readlink:
	    result = readlink ((char *) t2h_addr (cb, &s, arg1),
			       (char *) t2h_addr (cb, &s, arg2),
			       arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_readdir:
#if SIZEOF_VOID_P == 4
	    result = (int) readdir ((DIR *) t2h_addr (cb, &s, arg1));
	    errcode = errno;
#else
	    result = 0;
	    errcode = ENOSYS;
#endif
	    break;

#if 0
	  case TARGET_LINUX_SYS_mmap:
	    {
	      result = (int) mmap ((void *) t2h_addr (cb, &s, arg1),
				   arg2, arg3, arg4, arg5, arg6);
	      errcode = errno;

	      if (errno == 0)
		{
		  sim_core_attach (sd, NULL,
				   0, access_read_write_exec, 0,
				   result, arg2, 0, NULL, NULL);
		}
	    }
	    break;
#endif
	  case TARGET_LINUX_SYS_mmap2:
	    {
#if SIZEOF_VOID_P == 4  /* Code assumes m32r pointer size matches host.  */
	      void *addr;
	      size_t len;
	      int prot, flags, fildes;
	      off_t off;

	      addr   = (void *) t2h_addr (cb, &s, arg1);
	      len    = arg2;
	      prot   = arg3;
	      flags  = arg4;
	      fildes = arg5;
	      off    = arg6 << 12;

	      result = (int) mmap (addr, len, prot, flags, fildes, off);
	      errcode = errno;
	      if (result != -1)
		{
		  char c;
		  if (sim_core_read_buffer (sd, NULL, read_map, &c, result, 1) == 0)
		    sim_core_attach (sd, NULL,
				     0, access_read_write_exec, 0,
				     result, len, 0, NULL, NULL);
		}
#else
	      result = 0;
	      errcode = ENOSYS;
#endif
	    }
	    break;

	  case TARGET_LINUX_SYS_mmap:
	    {
#if SIZEOF_VOID_P == 4  /* Code assumes m32r pointer size matches host.  */
	      void *addr;
	      size_t len;
	      int prot, flags, fildes;
	      off_t off;

	      addr   = *((void **)  t2h_addr (cb, &s, arg1));
	      len    = *((size_t *) t2h_addr (cb, &s, arg1 + 4));
	      prot   = *((int *)    t2h_addr (cb, &s, arg1 + 8));
	      flags  = *((int *)    t2h_addr (cb, &s, arg1 + 12));
	      fildes = *((int *)    t2h_addr (cb, &s, arg1 + 16));
	      off    = *((off_t *)  t2h_addr (cb, &s, arg1 + 20));

	      addr   = (void *) T2H_4 ((unsigned int) addr);
	      len    = T2H_4 (len);
	      prot   = T2H_4 (prot);
	      flags  = T2H_4 (flags);
	      fildes = T2H_4 (fildes);
	      off    = T2H_4 (off);

	      //addr   = (void *) t2h_addr (cb, &s, (unsigned int) addr);
	      result = (int) mmap (addr, len, prot, flags, fildes, off);
	      errcode = errno;

	      //if (errno == 0)
	      if (result != -1)
		{
		  char c;
		  if (sim_core_read_buffer (sd, NULL, read_map, &c, result, 1) == 0)
		    sim_core_attach (sd, NULL,
				     0, access_read_write_exec, 0,
				     result, len, 0, NULL, NULL);
		}
#else
	      result = 0;
	      errcode = ENOSYS;
#endif
	    }
	    break;

	  case TARGET_LINUX_SYS_munmap:
	    result = munmap ((void *) (uintptr_t) arg1, arg2);
	    errcode = errno;
	    if (result != -1)
	      sim_core_detach (sd, NULL, 0, arg2, result);
	    break;

	  case TARGET_LINUX_SYS_truncate:
	    result = truncate ((char *) t2h_addr (cb, &s, arg1), arg2);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_ftruncate:
	    result = ftruncate (arg1, arg2);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_fchmod:
	    result = fchmod (arg1, arg2);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_fchown32:
	  case TARGET_LINUX_SYS_fchown:
	    result = fchown (arg1, arg2, arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_statfs:
	    {
	      struct statfs statbuf;

	      result = statfs ((char *) t2h_addr (cb, &s, arg1), &statbuf);
	      errcode = errno;

	      if (result != 0)
		break;

	      translate_endian_h2t (&statbuf, sizeof(statbuf));
	      if ((s.write_mem) (cb, &s, arg2, (char *) &statbuf, sizeof(statbuf))
		  != sizeof(statbuf))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_fstatfs:
	    {
	      struct statfs statbuf;

	      result = fstatfs (arg1, &statbuf);
	      errcode = errno;

	      if (result != 0)
		break;

	      translate_endian_h2t (&statbuf, sizeof(statbuf));
	      if ((s.write_mem) (cb, &s, arg2, (char *) &statbuf, sizeof(statbuf))
		  != sizeof(statbuf))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_syslog:
	    syslog (arg1, "%s", (char *) t2h_addr (cb, &s, arg2));
	    result = 0;
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_setitimer:
	    {
	      struct itimerval value, ovalue;

	      value = *((struct itimerval *) t2h_addr (cb, &s, arg2));
	      translate_endian_t2h (&value, sizeof(value));

	      if (arg2 == 0)
		{
		  result = setitimer (arg1, &value, NULL);
		  errcode = errno;
		}
	      else
		{
		  result = setitimer (arg1, &value, &ovalue);
		  errcode = errno;

		  if (result != 0)
		    break;

		  translate_endian_h2t (&ovalue, sizeof(ovalue));
		  if ((s.write_mem) (cb, &s, arg3, (char *) &ovalue, sizeof(ovalue))
		      != sizeof(ovalue))
		    {
		      result = -1;
		      errcode = EINVAL;
		    }
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_getitimer:
	    {
	      struct itimerval value;

	      result = getitimer (arg1, &value);
	      errcode = errno;

	      if (result != 0)
		break;

	      translate_endian_h2t (&value, sizeof(value));
	      if ((s.write_mem) (cb, &s, arg2, (char *) &value, sizeof(value))
		  != sizeof(value))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_stat:
	    {
	      char *buf;
	      int buflen;
	      struct stat statbuf;

	      result = stat ((char *) t2h_addr (cb, &s, arg1), &statbuf);
	      errcode = errno;
	      if (result < 0)
		break;

	      buflen = cb_host_to_target_stat (cb, NULL, NULL);
	      buf = xmalloc (buflen);
	      if (cb_host_to_target_stat (cb, &statbuf, buf) != buflen)
		{
		  /* The translation failed.  This is due to an internal
		     host program error, not the target's fault.  */
		  free (buf);
		  result = -1;
		  errcode = ENOSYS;
		  break;
		}
	      if ((s.write_mem) (cb, &s, arg2, buf, buflen) != buflen)
		{
		  free (buf);
		  result = -1;
		  errcode = EINVAL;
		  break;
		}
	      free (buf);
	    }
	    break;

	  case TARGET_LINUX_SYS_lstat:
	    {
	      char *buf;
	      int buflen;
	      struct stat statbuf;

	      result = lstat ((char *) t2h_addr (cb, &s, arg1), &statbuf);
	      errcode = errno;
	      if (result < 0)
		break;

	      buflen = cb_host_to_target_stat (cb, NULL, NULL);
	      buf = xmalloc (buflen);
	      if (cb_host_to_target_stat (cb, &statbuf, buf) != buflen)
		{
		  /* The translation failed.  This is due to an internal
		     host program error, not the target's fault.  */
		  free (buf);
		  result = -1;
		  errcode = ENOSYS;
		  break;
		}
	      if ((s.write_mem) (cb, &s, arg2, buf, buflen) != buflen)
		{
		  free (buf);
		  result = -1;
		  errcode = EINVAL;
		  break;
		}
	      free (buf);
	    }
	    break;

	  case TARGET_LINUX_SYS_fstat:
	    {
	      char *buf;
	      int buflen;
	      struct stat statbuf;

	      result = fstat (arg1, &statbuf);
	      errcode = errno;
	      if (result < 0)
		break;

	      buflen = cb_host_to_target_stat (cb, NULL, NULL);
	      buf = xmalloc (buflen);
	      if (cb_host_to_target_stat (cb, &statbuf, buf) != buflen)
		{
		  /* The translation failed.  This is due to an internal
		     host program error, not the target's fault.  */
		  free (buf);
		  result = -1;
		  errcode = ENOSYS;
		  break;
		}
	      if ((s.write_mem) (cb, &s, arg2, buf, buflen) != buflen)
		{
		  free (buf);
		  result = -1;
		  errcode = EINVAL;
		  break;
		}
	      free (buf);
	    }
	    break;

	  case TARGET_LINUX_SYS_sysinfo:
	    {
	      struct sysinfo info;

	      result = sysinfo (&info);
	      errcode = errno;

	      if (result != 0)
		break;

	      info.uptime    = H2T_4 (info.uptime);
	      info.loads[0]  = H2T_4 (info.loads[0]);
	      info.loads[1]  = H2T_4 (info.loads[1]);
	      info.loads[2]  = H2T_4 (info.loads[2]);
	      info.totalram  = H2T_4 (info.totalram);
	      info.freeram   = H2T_4 (info.freeram);
	      info.sharedram = H2T_4 (info.sharedram);
	      info.bufferram = H2T_4 (info.bufferram);
	      info.totalswap = H2T_4 (info.totalswap);
	      info.freeswap  = H2T_4 (info.freeswap);
	      info.procs     = H2T_2 (info.procs);
#if LINUX_VERSION_CODE >= 0x20400
	      info.totalhigh = H2T_4 (info.totalhigh);
	      info.freehigh  = H2T_4 (info.freehigh);
	      info.mem_unit  = H2T_4 (info.mem_unit);
#endif
	      if ((s.write_mem) (cb, &s, arg1, (char *) &info, sizeof(info))
		  != sizeof(info))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

#if 0
	  case TARGET_LINUX_SYS_ipc:
	    {
	      result = ipc (arg1, arg2, arg3, arg4,
			    (void *) t2h_addr (cb, &s, arg5), arg6);
	      errcode = errno;
	    }
	    break;
#endif

	  case TARGET_LINUX_SYS_fsync:
	    result = fsync (arg1);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_uname:
	    /* utsname contains only arrays of char, so it is not necessary
	       to translate endian. */
	    result = uname ((struct utsname *) t2h_addr (cb, &s, arg1));
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_adjtimex:
	    {
	      struct timex buf;

	      result = adjtimex (&buf);
	      errcode = errno;

	      if (result != 0)
		break;

	      translate_endian_h2t (&buf, sizeof(buf));
	      if ((s.write_mem) (cb, &s, arg1, (char *) &buf, sizeof(buf))
		  != sizeof(buf))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_mprotect:
	    result = mprotect ((void *) (uintptr_t) arg1, arg2, arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_fchdir:
	    result = fchdir (arg1);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_setfsuid32:
	  case TARGET_LINUX_SYS_setfsuid:
	    result = setfsuid (arg1);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_setfsgid32:
	  case TARGET_LINUX_SYS_setfsgid:
	    result = setfsgid (arg1);
	    errcode = errno;
	    break;

#if 0
	  case TARGET_LINUX_SYS__llseek:
	    {
	      loff_t buf;

	      result = _llseek (arg1, arg2, arg3, &buf, arg5);
	      errcode = errno;

	      if (result != 0)
		break;

	      translate_endian_h2t (&buf, sizeof(buf));
	      if ((s.write_mem) (cb, &s, t2h_addr (cb, &s, arg4),
				 (char *) &buf, sizeof(buf)) != sizeof(buf))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_getdents:
	    {
	      struct dirent dir;

	      result = getdents (arg1, &dir, arg3);
	      errcode = errno;

	      if (result != 0)
		break;

	      dir.d_ino = H2T_4 (dir.d_ino);
	      dir.d_off = H2T_4 (dir.d_off);
	      dir.d_reclen = H2T_2 (dir.d_reclen);
	      if ((s.write_mem) (cb, &s, arg2, (char *) &dir, sizeof(dir))
		  != sizeof(dir))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;
#endif

	  case TARGET_LINUX_SYS_flock:
	    result = flock (arg1, arg2);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_msync:
	    result = msync ((void *) (uintptr_t) arg1, arg2, arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_readv:
	    {
	      struct iovec vector;

	      vector = *((struct iovec *) t2h_addr (cb, &s, arg2));
	      translate_endian_t2h (&vector, sizeof(vector));

	      result = readv (arg1, &vector, arg3);
	      errcode = errno;
	    }
	    break;

	  case TARGET_LINUX_SYS_writev:
	    {
	      struct iovec vector;

	      vector = *((struct iovec *) t2h_addr (cb, &s, arg2));
	      translate_endian_t2h (&vector, sizeof(vector));

	      result = writev (arg1, &vector, arg3);
	      errcode = errno;
	    }
	    break;

	  case TARGET_LINUX_SYS_fdatasync:
	    result = fdatasync (arg1);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_mlock:
	    result = mlock ((void *) t2h_addr (cb, &s, arg1), arg2);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_munlock:
	    result = munlock ((void *) t2h_addr (cb, &s, arg1), arg2);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_nanosleep:
	    {
	      struct timespec req, rem;

	      req = *((struct timespec *) t2h_addr (cb, &s, arg2));
	      translate_endian_t2h (&req, sizeof(req));

	      result = nanosleep (&req, &rem);
	      errcode = errno;

	      if (result != 0)
		break;

	      translate_endian_h2t (&rem, sizeof(rem));
	      if ((s.write_mem) (cb, &s, arg2, (char *) &rem, sizeof(rem))
		  != sizeof(rem))
		{
		  result = -1;
		  errcode = EINVAL;
		}
	    }
	    break;

	  case TARGET_LINUX_SYS_mremap: /* FIXME */
#if SIZEOF_VOID_P == 4  /* Code assumes m32r pointer size matches host.  */
	    result = (int) mremap ((void *) t2h_addr (cb, &s, arg1), arg2, arg3, arg4);
	    errcode = errno;
#else
	    result = -1;
	    errcode = ENOSYS;
#endif
	    break;

	  case TARGET_LINUX_SYS_getresuid32:
	  case TARGET_LINUX_SYS_getresuid:
	    {
	      uid_t ruid, euid, suid;

	      result = getresuid (&ruid, &euid, &suid);
	      errcode = errno;

	      if (result != 0)
		break;

	      *((uid_t *) t2h_addr (cb, &s, arg1)) = H2T_4 (ruid);
	      *((uid_t *) t2h_addr (cb, &s, arg2)) = H2T_4 (euid);
	      *((uid_t *) t2h_addr (cb, &s, arg3)) = H2T_4 (suid);
	    }
	    break;

	  case TARGET_LINUX_SYS_poll:
	    {
	      struct pollfd ufds;

	      ufds = *((struct pollfd *) t2h_addr (cb, &s, arg1));
	      ufds.fd = T2H_4 (ufds.fd);
	      ufds.events = T2H_2 (ufds.events);
	      ufds.revents = T2H_2 (ufds.revents);

	      result = poll (&ufds, arg2, arg3);
	      errcode = errno;
	    }
	    break;

	  case TARGET_LINUX_SYS_getresgid32:
	  case TARGET_LINUX_SYS_getresgid:
	    {
	      uid_t rgid, egid, sgid;

	      result = getresgid (&rgid, &egid, &sgid);
	      errcode = errno;

	      if (result != 0)
		break;

	      *((uid_t *) t2h_addr (cb, &s, arg1)) = H2T_4 (rgid);
	      *((uid_t *) t2h_addr (cb, &s, arg2)) = H2T_4 (egid);
	      *((uid_t *) t2h_addr (cb, &s, arg3)) = H2T_4 (sgid);
	    }
	    break;

	  case TARGET_LINUX_SYS_pread:
	    result =  pread (arg1, (void *) t2h_addr (cb, &s, arg2), arg3, arg4);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_pwrite:
	    result =  pwrite (arg1, (void *) t2h_addr (cb, &s, arg2), arg3, arg4);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_chown32:
	  case TARGET_LINUX_SYS_chown:
	    result = chown ((char *) t2h_addr (cb, &s, arg1), arg2, arg3);
	    errcode = errno;
	    break;

	  case TARGET_LINUX_SYS_getcwd:
	    {
	      void *ret;

	      ret = getcwd ((char *) t2h_addr (cb, &s, arg1), arg2);
	      result = ret == NULL ? 0 : arg1;
	      errcode = errno;
	    }
	    break;

	  case TARGET_LINUX_SYS_sendfile:
	    {
	      off_t offset;

	      offset = *((off_t *) t2h_addr (cb, &s, arg3));
	      offset = T2H_4 (offset);

	      result = sendfile (arg1, arg2, &offset, arg3);
	      errcode = errno;

	      if (result != 0)
		break;

	      *((off_t *) t2h_addr (cb, &s, arg3)) = H2T_4 (offset);
	    }
	    break;

	  default:
	    result = -1;
	    errcode = ENOSYS;
	    break;
	  }

	if (result == -1)
	  m32rbf_h_gr_set (current_cpu, 0, -errcode);
	else
	  m32rbf_h_gr_set (current_cpu, 0, result);
	break;
      }
#endif

    case TRAP_BREAKPOINT:
      sim_engine_halt (sd, current_cpu, NULL, pc,
		       sim_stopped, SIM_SIGTRAP);
      break;

    case TRAP_FLUSH_CACHE:
      /* Do nothing.  */
      break;

    case_default:
    default:
      {
	/* The new pc is the trap vector entry.
	   We assume there's a branch there to some handler.
	   Use cr5 as EVB (EIT Vector Base) register.  */
	/* USI new_pc = EIT_TRAP_BASE_ADDR + num * 4; */
	USI new_pc = m32rbf_h_cr_get (current_cpu, 5) + 0x40 + num * 4;
	return new_pc;
      }
    }

  /* Fake an "rte" insn.  */
  /* FIXME: Should duplicate all of rte processing.  */
  return (pc & -4) + 4;
}
