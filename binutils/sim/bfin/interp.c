/* Simulator for Analog Devices Blackfin processors.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Analog Devices, Inc.

   This file is part of simulators.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include "portability.h"
#include "sim/callback.h"
#include "gdb/signals.h"
#include "sim-main.h"
#include "sim-options.h"
#include "sim-syscall.h"
#include "sim-hw.h"

#include "bfin-sim.h"

/* The numbers here do not matter.  They just need to be unique.  They also
   need not be static across releases -- they're used internally only.  The
   mapping from the Linux ABI to the CB values is in linux-targ-map.h.  */
#define CB_SYS_ioctl        201
#define CB_SYS_mmap2        202
#define CB_SYS_munmap       203
#define CB_SYS_dup2         204
#define CB_SYS_getuid       205
#define CB_SYS_getuid32     206
#define CB_SYS_getgid       207
#define CB_SYS_getgid32     208
#define CB_SYS_setuid       209
#define CB_SYS_setuid32     210
#define CB_SYS_setgid       211
#define CB_SYS_setgid32     212
#define CB_SYS_pread        213
#define CB_SYS__llseek      214
#define CB_SYS_getcwd       215
#define CB_SYS_stat64       216
#define CB_SYS_lstat64      217
#define CB_SYS_fstat64      218
#define CB_SYS_ftruncate64  219
#define CB_SYS_gettimeofday 220
#define CB_SYS_access       221
#include "linux-targ-map.h"
#include "linux-fixed-code.h"

#include "elf/common.h"
#include "elf/external.h"
#include "elf/internal.h"
#include "elf/bfin.h"
#include "bfd/elf-bfd.h"

#include "dv-bfin_cec.h"
#include "dv-bfin_mmu.h"

static const char cb_linux_stat_map_32[] =
/* Linux kernel 32bit layout:  */
"st_dev,2:space,2:st_ino,4:st_mode,2:st_nlink,2:st_uid,2:st_gid,2:st_rdev,2:"
"space,2:st_size,4:st_blksize,4:st_blocks,4:st_atime,4:st_atimensec,4:"
"st_mtime,4:st_mtimensec,4:st_ctime,4:st_ctimensec,4:space,4:space,4";
/* uClibc public ABI 32bit layout:
"st_dev,8:space,2:space,2:st_ino,4:st_mode,4:st_nlink,4:st_uid,4:st_gid,4:"
"st_rdev,8:space,2:space,2:st_size,4:st_blksiez,4:st_blocks,4:st_atime,4:"
"st_atimensec,4:st_mtime,4:st_mtimensec,4:st_ctime,4:st_ctimensec,4:space,4:"
"space,4";  */
static const char cb_linux_stat_map_64[] =
"st_dev,8:space,4:space,4:st_mode,4:st_nlink,4:st_uid,4:st_gid,4:st_rdev,8:"
"space,4:st_size,8:st_blksize,4:st_blocks,8:st_atime,4:st_atimensec,4:"
"st_mtime,4:st_mtimensec,4:st_ctime,4:st_ctimensec,4:st_ino,8";
static const char cb_libgloss_stat_map_32[] =
"st_dev,2:st_ino,2:st_mode,4:st_nlink,2:st_uid,2:st_gid,2:st_rdev,2:"
"st_size,4:st_atime,4:space,4:st_mtime,4:space,4:st_ctime,4:"
"space,4:st_blksize,4:st_blocks,4:space,8";
static const char *stat_map_32, *stat_map_64;

/* Simulate a monitor trap, put the result into r0 and errno into r1
   return offset by which to adjust pc.  */

void
bfin_syscall (SIM_CPU *cpu)
{
  SIM_DESC sd = CPU_STATE (cpu);
  host_callback *cb = STATE_CALLBACK (sd);
  bu32 args[6];
  CB_SYSCALL sc;
  char *p;
  char _tbuf[1024 * 3], *tbuf = _tbuf, tstr[1024];
  int fmt_ret_hex = 0;

  CB_SYSCALL_INIT (&sc);

  if (STATE_ENVIRONMENT (sd) == USER_ENVIRONMENT)
    {
      /* Linux syscall.  */
      sc.func = PREG (0);
      sc.arg1 = args[0] = DREG (0);
      sc.arg2 = args[1] = DREG (1);
      sc.arg3 = args[2] = DREG (2);
      sc.arg4 = args[3] = DREG (3);
      sc.arg5 = args[4] = DREG (4);
      sc.arg6 = args[5] = DREG (5);
    }
  else
    {
      /* libgloss syscall.  */
      sc.func = PREG (0);
      sc.arg1 = args[0] = GET_LONG (DREG (0));
      sc.arg2 = args[1] = GET_LONG (DREG (0) + 4);
      sc.arg3 = args[2] = GET_LONG (DREG (0) + 8);
      sc.arg4 = args[3] = GET_LONG (DREG (0) + 12);
      sc.arg5 = args[4] = GET_LONG (DREG (0) + 16);
      sc.arg6 = args[5] = GET_LONG (DREG (0) + 20);
    }
  sc.p1 = sd;
  sc.p2 = cpu;
  sc.read_mem = sim_syscall_read_mem;
  sc.write_mem = sim_syscall_write_mem;

  /* Common cb_syscall() handles most functions.  */
  switch (cb_target_to_host_syscall (cb, sc.func))
    {
    case CB_SYS_exit:
      tbuf += sprintf (tbuf, "exit(%i)", args[0]);
      sim_engine_halt (sd, cpu, NULL, PCREG, sim_exited, sc.arg1);

    case CB_SYS_gettimeofday:
      {
	struct timeval _tv, *tv = &_tv;
	struct timezone _tz, *tz = &_tz;

	tbuf += sprintf (tbuf, "gettimeofday(%#x, %#x)", args[0], args[1]);

	if (sc.arg1 == 0)
	  tv = NULL;
	if (sc.arg2 == 0)
	  tz = NULL;
	sc.result = gettimeofday (tv, tz);

	if (sc.result == 0)
	  {
	    bu32 t;

	    if (tv)
	      {
		t = tv->tv_sec;
		sc.write_mem (cb, &sc, sc.arg1, (void *)&t, 4);
		t = tv->tv_usec;
		sc.write_mem (cb, &sc, sc.arg1 + 4, (void *)&t, 4);
	      }

	    if (sc.arg2)
	      {
		t = tz->tz_minuteswest;
		sc.write_mem (cb, &sc, sc.arg1, (void *)&t, 4);
		t = tz->tz_dsttime;
		sc.write_mem (cb, &sc, sc.arg1 + 4, (void *)&t, 4);
	      }
	  }
	else
	  goto sys_finish;
      }
      break;

    case CB_SYS_ioctl:
      /* XXX: hack just enough to get basic stdio w/uClibc ...  */
      tbuf += sprintf (tbuf, "ioctl(%i, %#x, %u)", args[0], args[1], args[2]);
      if (sc.arg2 == 0x5401)
	{
	  sc.result = !isatty (sc.arg1);
	  sc.errcode = 0;
	}
      else
	{
	  sc.result = -1;
	  sc.errcode = cb_host_to_target_errno (cb, EINVAL);
	}
      break;

    case CB_SYS_mmap2:
      {
	static bu32 heap = BFIN_DEFAULT_MEM_SIZE / 2;

	fmt_ret_hex = 1;
	tbuf += sprintf (tbuf, "mmap2(%#x, %u, %#x, %#x, %i, %u)",
			 args[0], args[1], args[2], args[3], args[4], args[5]);

	sc.errcode = 0;

	if (sc.arg4 & 0x20 /*MAP_ANONYMOUS*/)
	  /* XXX: We don't handle zeroing, but default is all zeros.  */;
	else if (args[4] >= MAX_CALLBACK_FDS)
	  sc.errcode = cb_host_to_target_errno (cb, ENOSYS);
	else
	  {
#ifdef HAVE_PREAD
	    char *data = xmalloc (sc.arg2);

	    /* XXX: Should add a cb->pread.  */
	    if (pread (cb->fdmap[args[4]], data, sc.arg2, args[5] << 12) == sc.arg2)
	      sc.write_mem (cb, &sc, heap, data, sc.arg2);
	    else
	      sc.errcode = cb_host_to_target_errno (cb, EINVAL);

	    free (data);
#else
	    sc.errcode = cb_host_to_target_errno (cb, ENOSYS);
#endif
	  }

	if (sc.errcode)
	  {
	    sc.result = -1;
	    break;
	  }

	sc.result = heap;
	heap += sc.arg2;
	/* Keep it page aligned.  */
	heap = align_up (heap, 4096);

	break;
      }

    case CB_SYS_munmap:
      /* XXX: meh, just lie for mmap().  */
      tbuf += sprintf (tbuf, "munmap(%#x, %u)", args[0], args[1]);
      sc.result = 0;
      break;

    case CB_SYS_dup2:
      tbuf += sprintf (tbuf, "dup2(%i, %i)", args[0], args[1]);
      if (sc.arg1 >= MAX_CALLBACK_FDS || sc.arg2 >= MAX_CALLBACK_FDS)
	{
	  sc.result = -1;
	  sc.errcode = cb_host_to_target_errno (cb, EINVAL);
	}
      else
	{
	  sc.result = dup2 (cb->fdmap[sc.arg1], cb->fdmap[sc.arg2]);
	  goto sys_finish;
	}
      break;

    case CB_SYS__llseek:
      tbuf += sprintf (tbuf, "llseek(%i, %u, %u, %#x, %u)",
		       args[0], args[1], args[2], args[3], args[4]);
      sc.func = TARGET_LINUX_SYS_lseek;
      if (sc.arg2)
	{
	  sc.result = -1;
	  sc.errcode = cb_host_to_target_errno (cb, EINVAL);
	}
      else
	{
	  sc.arg2 = sc.arg3;
	  sc.arg3 = args[4];
	  cb_syscall (cb, &sc);
	  if (sc.result != -1)
	    {
	      bu32 z = 0;
	      sc.write_mem (cb, &sc, args[3], (void *)&sc.result, 4);
	      sc.write_mem (cb, &sc, args[3] + 4, (void *)&z, 4);
	    }
	}
      break;

    /* XXX: Should add a cb->pread.  */
    case CB_SYS_pread:
      tbuf += sprintf (tbuf, "pread(%i, %#x, %u, %i)",
		       args[0], args[1], args[2], args[3]);
      if (sc.arg1 >= MAX_CALLBACK_FDS)
	{
	  sc.result = -1;
	  sc.errcode = cb_host_to_target_errno (cb, EINVAL);
	}
      else
	{
	  long old_pos, read_result, read_errcode;

	  /* Get current filepos.  */
	  sc.func = TARGET_LINUX_SYS_lseek;
	  sc.arg2 = 0;
	  sc.arg3 = SEEK_CUR;
	  cb_syscall (cb, &sc);
	  if (sc.result == -1)
	    break;
	  old_pos = sc.result;

	  /* Move to the new pos.  */
	  sc.func = TARGET_LINUX_SYS_lseek;
	  sc.arg2 = args[3];
	  sc.arg3 = SEEK_SET;
	  cb_syscall (cb, &sc);
	  if (sc.result == -1)
	    break;

	  /* Read the data.  */
	  sc.func = TARGET_LINUX_SYS_read;
	  sc.arg2 = args[1];
	  sc.arg3 = args[2];
	  cb_syscall (cb, &sc);
	  read_result = sc.result;
	  read_errcode = sc.errcode;

	  /* Move back to the old pos.  */
	  sc.func = TARGET_LINUX_SYS_lseek;
	  sc.arg2 = old_pos;
	  sc.arg3 = SEEK_SET;
	  cb_syscall (cb, &sc);

	  sc.result = read_result;
	  sc.errcode = read_errcode;
	}
      break;

    case CB_SYS_getcwd:
      tbuf += sprintf (tbuf, "getcwd(%#x, %u)", args[0], args[1]);

      p = alloca (sc.arg2);
      if (getcwd (p, sc.arg2) == NULL)
	{
	  sc.result = -1;
	  sc.errcode = cb_host_to_target_errno (cb, EINVAL);
	}
      else
	{
	  sc.write_mem (cb, &sc, sc.arg1, p, sc.arg2);
	  sc.result = sc.arg1;
	}
      break;

    case CB_SYS_stat64:
      if (cb_get_string (cb, &sc, tstr, sizeof (tstr), args[0]))
	strcpy (tstr, "???");
      tbuf += sprintf (tbuf, "stat64(%#x:\"%s\", %u)", args[0], tstr, args[1]);
      cb->stat_map = stat_map_64;
      sc.func = TARGET_LINUX_SYS_stat;
      cb_syscall (cb, &sc);
      cb->stat_map = stat_map_32;
      break;
    case CB_SYS_lstat64:
      if (cb_get_string (cb, &sc, tstr, sizeof (tstr), args[0]))
	strcpy (tstr, "???");
      tbuf += sprintf (tbuf, "lstat64(%#x:\"%s\", %u)", args[0], tstr, args[1]);
      cb->stat_map = stat_map_64;
      sc.func = TARGET_LINUX_SYS_lstat;
      cb_syscall (cb, &sc);
      cb->stat_map = stat_map_32;
      break;
    case CB_SYS_fstat64:
      tbuf += sprintf (tbuf, "fstat64(%#x, %u)", args[0], args[1]);
      cb->stat_map = stat_map_64;
      sc.func = TARGET_LINUX_SYS_fstat;
      cb_syscall (cb, &sc);
      cb->stat_map = stat_map_32;
      break;

    case CB_SYS_ftruncate64:
      tbuf += sprintf (tbuf, "ftruncate64(%u, %u)", args[0], args[1]);
      sc.func = TARGET_LINUX_SYS_ftruncate;
      cb_syscall (cb, &sc);
      break;

    case CB_SYS_getuid:
    case CB_SYS_getuid32:
      tbuf += sprintf (tbuf, "getuid()");
      sc.result = getuid ();
      goto sys_finish;
    case CB_SYS_getgid:
    case CB_SYS_getgid32:
      tbuf += sprintf (tbuf, "getgid()");
      sc.result = getgid ();
      goto sys_finish;
    case CB_SYS_setuid:
      sc.arg1 &= 0xffff;
      ATTRIBUTE_FALLTHROUGH;
    case CB_SYS_setuid32:
      tbuf += sprintf (tbuf, "setuid(%u)", args[0]);
      sc.result = setuid (sc.arg1);
      goto sys_finish;
    case CB_SYS_setgid:
      sc.arg1 &= 0xffff;
      ATTRIBUTE_FALLTHROUGH;
    case CB_SYS_setgid32:
      tbuf += sprintf (tbuf, "setgid(%u)", args[0]);
      sc.result = setgid (sc.arg1);
      goto sys_finish;

    case CB_SYS_kill:
      tbuf += sprintf (tbuf, "kill(%u, %i)", args[0], args[1]);
      /* Only let the app kill itself.  */
      if (sc.arg1 != getpid ())
	{
	  sc.result = -1;
	  sc.errcode = cb_host_to_target_errno (cb, EPERM);
	}
      else
	{
#ifdef HAVE_KILL
	  sc.result = kill (sc.arg1, sc.arg2);
	  goto sys_finish;
#else
	  sc.result = -1;
	  sc.errcode = cb_host_to_target_errno (cb, ENOSYS);
#endif
	}
      break;

    case CB_SYS_open:
      if (cb_get_string (cb, &sc, tstr, sizeof (tstr), args[0]))
	strcpy (tstr, "???");
      tbuf += sprintf (tbuf, "open(%#x:\"%s\", %#x, %o)",
		       args[0], tstr, args[1], args[2]);
      goto case_default;
    case CB_SYS_close:
      tbuf += sprintf (tbuf, "close(%i)", args[0]);
      goto case_default;
    case CB_SYS_read:
      tbuf += sprintf (tbuf, "read(%i, %#x, %u)", args[0], args[1], args[2]);
      goto case_default;
    case CB_SYS_write:
      if (cb_get_string (cb, &sc, tstr, sizeof (tstr), args[1]))
	strcpy (tstr, "???");
      tbuf += sprintf (tbuf, "write(%i, %#x:\"%s\", %u)",
		       args[0], args[1], tstr, args[2]);
      goto case_default;
    case CB_SYS_lseek:
      tbuf += sprintf (tbuf, "lseek(%i, %i, %i)", args[0], args[1], args[2]);
      goto case_default;
    case CB_SYS_unlink:
      if (cb_get_string (cb, &sc, tstr, sizeof (tstr), args[0]))
	strcpy (tstr, "???");
      tbuf += sprintf (tbuf, "unlink(%#x:\"%s\")", args[0], tstr);
      goto case_default;
    case CB_SYS_truncate:
      if (cb_get_string (cb, &sc, tstr, sizeof (tstr), args[0]))
	strcpy (tstr, "???");
      tbuf += sprintf (tbuf, "truncate(%#x:\"%s\", %i)", args[0], tstr, args[1]);
      goto case_default;
    case CB_SYS_ftruncate:
      tbuf += sprintf (tbuf, "ftruncate(%i, %i)", args[0], args[1]);
      goto case_default;
    case CB_SYS_rename:
      if (cb_get_string (cb, &sc, tstr, sizeof (tstr), args[0]))
	strcpy (tstr, "???");
      tbuf += sprintf (tbuf, "rename(%#x:\"%s\", ", args[0], tstr);
      if (cb_get_string (cb, &sc, tstr, sizeof (tstr), args[1]))
	strcpy (tstr, "???");
      tbuf += sprintf (tbuf, "%#x:\"%s\")", args[1], tstr);
      goto case_default;
    case CB_SYS_stat:
      if (cb_get_string (cb, &sc, tstr, sizeof (tstr), args[0]))
	strcpy (tstr, "???");
      tbuf += sprintf (tbuf, "stat(%#x:\"%s\", %#x)", args[0], tstr, args[1]);
      goto case_default;
    case CB_SYS_fstat:
      tbuf += sprintf (tbuf, "fstat(%i, %#x)", args[0], args[1]);
      goto case_default;
    case CB_SYS_lstat:
      if (cb_get_string (cb, &sc, tstr, sizeof (tstr), args[0]))
	strcpy (tstr, "???");
      tbuf += sprintf (tbuf, "lstat(%#x:\"%s\", %#x)", args[0], tstr, args[1]);
      goto case_default;
    case CB_SYS_pipe:
      tbuf += sprintf (tbuf, "pipe(%#x, %#x)", args[0], args[1]);
      goto case_default;

    default:
      tbuf += sprintf (tbuf, "???_%i(%#x, %#x, %#x, %#x, %#x, %#x)", sc.func,
		       args[0], args[1], args[2], args[3], args[4], args[5]);
    case_default:
      cb_syscall (cb, &sc);
      break;

    sys_finish:
      if (sc.result == -1)
	{
	  cb->last_errno = errno;
	  sc.errcode = cb->get_errno (cb);
	}
    }

  TRACE_EVENTS (cpu, "syscall_%i(%#x, %#x, %#x, %#x, %#x, %#x) = %li (error = %i)",
		sc.func, args[0], args[1], args[2], args[3], args[4], args[5],
		sc.result, sc.errcode);

  tbuf += sprintf (tbuf, " = ");
  if (STATE_ENVIRONMENT (sd) == USER_ENVIRONMENT)
    {
      if (sc.result == -1)
	{
	  tbuf += sprintf (tbuf, "-1 (error = %i)", sc.errcode);
	  if (sc.errcode == cb_host_to_target_errno (cb, ENOSYS))
	    {
	      sim_io_eprintf (sd, "bfin-sim: %#x: unimplemented syscall %i\n",
			      PCREG, sc.func);
	    }
	  SET_DREG (0, -sc.errcode);
	}
      else
	{
	  if (fmt_ret_hex)
	    tbuf += sprintf (tbuf, "%#lx", sc.result);
	  else
	    tbuf += sprintf (tbuf, "%lu", sc.result);
	  SET_DREG (0, sc.result);
	}
    }
  else
    {
      tbuf += sprintf (tbuf, "%lu (error = %i)", sc.result, sc.errcode);
      SET_DREG (0, sc.result);
      SET_DREG (1, sc.result2);
      SET_DREG (2, sc.errcode);
    }

  TRACE_SYSCALL (cpu, "%s", _tbuf);
}

/* Execute a single instruction.  */

static sim_cia
step_once (SIM_CPU *cpu)
{
  SIM_DESC sd = CPU_STATE (cpu);
  bu32 insn_len, oldpc = PCREG;
  int i;
  bool ssstep;

  if (TRACE_ANY_P (cpu))
    trace_prefix (sd, cpu, NULL_CIA, oldpc, TRACE_LINENUM_P (cpu),
		  NULL, 0, " "); /* Use a space for gcc warnings.  */

  TRACE_DISASM (cpu, oldpc);

  /* Handle hardware single stepping when lower than EVT3, and when SYSCFG
     has already had the SSSTEP bit enabled.  */
  ssstep = false;
  if (STATE_ENVIRONMENT (sd) == OPERATING_ENVIRONMENT
      && (SYSCFGREG & SYSCFG_SSSTEP))
    {
      int ivg = cec_get_ivg (cpu);
      if (ivg == -1 || ivg > 3)
	ssstep = true;
    }

#if 0
  /* XXX: Is this what happens on the hardware ?  */
  if (cec_get_ivg (cpu) == EVT_EMU)
    cec_return (cpu, EVT_EMU);
#endif

  BFIN_CPU_STATE.did_jump = false;

  insn_len = interp_insn_bfin (cpu, oldpc);

  /* If we executed this insn successfully, then we always decrement
     the loop counter.  We don't want to update the PC though if the
     last insn happened to be a change in code flow (jump/etc...).  */
  if (!BFIN_CPU_STATE.did_jump)
    SET_PCREG (hwloop_get_next_pc (cpu, oldpc, insn_len));
  for (i = 1; i >= 0; --i)
    if (LCREG (i) && oldpc == LBREG (i))
      {
	SET_LCREG (i, LCREG (i) - 1);
	if (LCREG (i))
	  break;
      }

  ++ PROFILE_TOTAL_INSN_COUNT (CPU_PROFILE_DATA (cpu));

  /* Handle hardware single stepping only if we're still lower than EVT3.
     XXX: May not be entirely correct wrt EXCPT insns.  */
  if (ssstep)
    {
      int ivg = cec_get_ivg (cpu);
      if (ivg == -1 || ivg > 3)
	{
	  INSN_LEN = 0;
	  cec_exception (cpu, VEC_STEP);
	}
    }

  return oldpc;
}

void
sim_engine_run (SIM_DESC sd,
		int next_cpu_nr, /* ignore  */
		int nr_cpus, /* ignore  */
		int siggnal) /* ignore  */
{
  bu32 ticks;
  SIM_CPU *cpu;

  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  cpu = STATE_CPU (sd, 0);

  while (1)
    {
      step_once (cpu);
      /* Process any events -- can't use tickn because it may
         advance right over the next event.  */
      for (ticks = 0; ticks < CYCLE_DELAY; ++ticks)
	if (sim_events_tick (sd))
	  sim_events_process (sd);
    }
}

/* Cover function of sim_state_free to free the cpu buffers as well.  */

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

/* Create an instance of the simulator.  */

static void
bfin_initialize_cpu (SIM_DESC sd, SIM_CPU *cpu)
{
  PROFILE_TOTAL_INSN_COUNT (CPU_PROFILE_DATA (cpu)) = 0;

  bfin_model_cpu_init (sd, cpu);

  /* Set default stack to top of scratch pad.  */
  SET_SPREG (BFIN_DEFAULT_MEM_SIZE);
  SET_KSPREG (BFIN_DEFAULT_MEM_SIZE);
  SET_USPREG (BFIN_DEFAULT_MEM_SIZE);

  /* This is what the hardware likes.  */
  SET_SYSCFGREG (0x30);
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *callback,
	  struct bfd *abfd, char * const *argv)
{
  char c;
  int i;
  SIM_DESC sd = sim_state_alloc_extra (kind, callback,
				       sizeof (struct bfin_board_data));

  /* Set default options before parsing user options.  */
  STATE_MACHS (sd) = bfin_sim_machs;
  STATE_MODEL_NAME (sd) = "bf537";
  current_alignment = STRICT_ALIGNMENT;
  current_target_byte_order = BFD_ENDIAN_LITTLE;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all_extra (sd, 0, sizeof (struct bfin_cpu_state))
      != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* XXX: Default to the Virtual environment.  */
  if (STATE_ENVIRONMENT (sd) == ALL_ENVIRONMENT)
    STATE_ENVIRONMENT (sd) = VIRTUAL_ENVIRONMENT;

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Allocate external memory if none specified by user.
     Use address 4 here in case the user wanted address 0 unmapped.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, 4, 1) == 0)
    {
      bu16 emuexcpt = 0x25;
      sim_do_commandf (sd, "memory-size 0x%x", BFIN_DEFAULT_MEM_SIZE);
      sim_write (sd, 0, &emuexcpt, 2);
    }

  /* Check for/establish the a reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Establish any remaining configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU *cpu = STATE_CPU (sd, i);
      bfin_initialize_cpu (sd, cpu);
    }

  return sd;
}

/* Some utils don't like having a NULL environ.  */
static char * const simple_env[] = { "HOME=/", "PATH=/bin", NULL };

static bu32 fdpic_load_offset;

static bool
bfin_fdpic_load (SIM_DESC sd, SIM_CPU *cpu, struct bfd *abfd, bu32 *sp,
		 bu32 *elf_addrs, char **ldso_path)
{
  bool ret;
  int i;

  Elf_Internal_Ehdr *iehdr;
  Elf32_External_Ehdr ehdr;
  Elf_Internal_Phdr *phdrs;
  unsigned char *data;
  long phdr_size;
  int phdrc;
  bu32 nsegs;

  bu32 max_load_addr;

  unsigned char null[4] = { 0, 0, 0, 0 };

  ret = false;
  *ldso_path = NULL;

  /* See if this an FDPIC ELF.  */
  phdrs = NULL;
  if (!abfd)
    goto skip_fdpic_init;
  if (bfd_seek (abfd, 0, SEEK_SET) != 0)
    goto skip_fdpic_init;
  if (bfd_read (&ehdr, sizeof (ehdr), abfd) != sizeof (ehdr))
    goto skip_fdpic_init;
  iehdr = elf_elfheader (abfd);
  if (!(iehdr->e_flags & EF_BFIN_FDPIC))
    goto skip_fdpic_init;

  if (STATE_OPEN_KIND (sd) == SIM_OPEN_DEBUG)
    sim_io_printf (sd, "Loading FDPIC ELF %s\n Load base: %#x\n ELF entry: %#x\n",
		   bfd_get_filename (abfd), fdpic_load_offset, elf_addrs[0]);

  /* Grab the Program Headers to set up the loadsegs on the stack.  */
  phdr_size = bfd_get_elf_phdr_upper_bound (abfd);
  if (phdr_size == -1)
    goto skip_fdpic_init;
  phdrs = xmalloc (phdr_size);
  phdrc = bfd_get_elf_phdrs (abfd, phdrs);
  if (phdrc == -1)
    goto skip_fdpic_init;

  /* Push the Ehdr onto the stack.  */
  *sp -= sizeof (ehdr);
  elf_addrs[3] = *sp;
  sim_write (sd, *sp, &ehdr, sizeof (ehdr));
  if (STATE_OPEN_KIND (sd) == SIM_OPEN_DEBUG)
    sim_io_printf (sd, " Elf_Ehdr: %#x\n", *sp);

  /* Since we're relocating things ourselves, we need to relocate
     the start address as well.  */
  elf_addrs[0] = bfd_get_start_address (abfd) + fdpic_load_offset;

  /* And the Exec's Phdrs onto the stack.  */
  if (STATE_PROG_BFD (sd) == abfd)
    {
      elf_addrs[4] = elf_addrs[0];

      phdr_size = iehdr->e_phentsize * iehdr->e_phnum;
      if (bfd_seek (abfd, iehdr->e_phoff, SEEK_SET) != 0)
	goto skip_fdpic_init;
      data = xmalloc (phdr_size);
      if (bfd_read (data, phdr_size, abfd) != phdr_size)
	goto skip_fdpic_init;
      *sp -= phdr_size;
      elf_addrs[1] = *sp;
      elf_addrs[2] = phdrc;
      sim_write (sd, *sp, data, phdr_size);
      free (data);
      if (STATE_OPEN_KIND (sd) == SIM_OPEN_DEBUG)
	sim_io_printf (sd, " Elf_Phdrs: %#x\n", *sp);
    }

  /* Now push all the loadsegs.  */
  nsegs = 0;
  max_load_addr = 0;
  for (i = phdrc; i >= 0; --i)
    if (phdrs[i].p_type == PT_LOAD)
      {
	Elf_Internal_Phdr *p = &phdrs[i];
	bu32 paddr, vaddr, memsz, filesz;

	paddr = p->p_paddr + fdpic_load_offset;
	vaddr = p->p_vaddr;
	memsz = p->p_memsz;
	filesz = p->p_filesz;

	if (STATE_OPEN_KIND (sd) == SIM_OPEN_DEBUG)
	  sim_io_printf (sd, " PHDR %i: vma %#x lma %#x filesz %#x memsz %#x\n",
			 i, vaddr, paddr, filesz, memsz);

	data = xmalloc (memsz);
	if (memsz != filesz)
	  memset (data + filesz, 0, memsz - filesz);

	if (bfd_seek (abfd, p->p_offset, SEEK_SET) == 0
	    && bfd_read (data, filesz, abfd) == filesz)
	  sim_write (sd, paddr, data, memsz);

	free (data);

	max_load_addr = max (paddr + memsz, max_load_addr);

	*sp -= 12;
	sim_write (sd, *sp+0, &paddr, 4); /* loadseg.addr  */
	sim_write (sd, *sp+4, &vaddr, 4); /* loadseg.p_vaddr  */
	sim_write (sd, *sp+8, &memsz, 4); /* loadseg.p_memsz  */
	++nsegs;
      }
    else if (phdrs[i].p_type == PT_DYNAMIC)
      {
	elf_addrs[5] = phdrs[i].p_paddr + fdpic_load_offset;
	if (STATE_OPEN_KIND (sd) == SIM_OPEN_DEBUG)
	  sim_io_printf (sd, " PT_DYNAMIC: %#x\n", elf_addrs[5]);
      }
    else if (phdrs[i].p_type == PT_INTERP)
      {
	uint32_t off = phdrs[i].p_offset;
	uint32_t len = phdrs[i].p_filesz;

	*ldso_path = xmalloc (len);
	if (bfd_seek (abfd, off, SEEK_SET) != 0
	    || bfd_read (*ldso_path, len, abfd) != len)
	  {
	    free (*ldso_path);
	    *ldso_path = NULL;
	  }
	else if (STATE_OPEN_KIND (sd) == SIM_OPEN_DEBUG)
	  sim_io_printf (sd, " PT_INTERP: %s\n", *ldso_path);
      }

  /* Update the load offset with a few extra pages.  */
  fdpic_load_offset = align_up (max (max_load_addr, fdpic_load_offset),
				0x10000);
  fdpic_load_offset += 0x10000;

  /* Push the summary loadmap info onto the stack last.  */
  *sp -= 4;
  sim_write (sd, *sp+0, null, 2); /* loadmap.version  */
  sim_write (sd, *sp+2, &nsegs, 2); /* loadmap.nsegs  */

  ret = true;
 skip_fdpic_init:
  free (phdrs);

  return ret;
}

static void
bfin_user_init (SIM_DESC sd, SIM_CPU *cpu, struct bfd *abfd,
		char * const *argv, char * const *env)
{
  /* XXX: Missing host -> target endian ...  */
  /* Linux starts the user app with the stack:
       argc
       argv[0]          -- pointers to the actual strings
       argv[1..N]
       NULL
       env[0]
       env[1..N]
       NULL
       auxvt[0].type    -- ELF Auxiliary Vector Table
       auxvt[0].value
       auxvt[1..N]
       AT_NULL
       0
       argv[0..N][0..M] -- actual argv/env strings
       env[0..N][0..M]
       FDPIC loadmaps   -- for FDPIC apps
     So set things up the same way.  */
  int i, argc, envc;
  bu32 argv_flat, env_flat;

  bu32 sp, sp_flat;

  /* start, at_phdr, at_phnum, at_base, at_entry, pt_dynamic  */
  bu32 elf_addrs[6];
  bu32 auxvt;
  bu32 exec_loadmap, ldso_loadmap;
  char *ldso_path;

  unsigned char null[4] = { 0, 0, 0, 0 };

  host_callback *cb = STATE_CALLBACK (sd);

  elf_addrs[0] = elf_addrs[4] = bfd_get_start_address (abfd);
  elf_addrs[1] = elf_addrs[2] = elf_addrs[3] = elf_addrs[5] = 0;

  /* Keep the load addresses consistent between runs.  Also make sure we make
     space for the fixed code region (part of the Blackfin Linux ABI).  */
  fdpic_load_offset = 0x1000;

  /* First try to load this as an FDPIC executable.  */
  sp = SPREG;
  if (!bfin_fdpic_load (sd, cpu, STATE_PROG_BFD (sd), &sp, elf_addrs, &ldso_path))
    goto skip_fdpic_init;
  exec_loadmap = sp;

  /* If that worked, then load the fixed code region.  We only do this for
     FDPIC ELFs atm because they are PIEs and let us relocate them without
     manual fixups.  FLAT files however require location processing which
     we do not do ourselves, and they link with a VMA of 0.  */
  sim_write (sd, 0x400, bfin_linux_fixed_code, sizeof (bfin_linux_fixed_code));

  /* If the FDPIC needs an interpreter, then load it up too.  */
  if (ldso_path)
    {
      const char *ldso_full_path = concat (simulator_sysroot, ldso_path, NULL);
      struct bfd *ldso_bfd;

      ldso_bfd = bfd_openr (ldso_full_path, STATE_TARGET (sd));
      if (!ldso_bfd)
	{
	  sim_io_eprintf (sd, "bfin-sim: bfd open failed: %s\n", ldso_full_path);
	  goto static_fdpic;
	}
      if (!bfd_check_format (ldso_bfd, bfd_object))
	sim_io_eprintf (sd, "bfin-sim: bfd format not valid: %s\n", ldso_full_path);
      bfd_set_arch_info (ldso_bfd, STATE_ARCHITECTURE (sd));

      if (!bfin_fdpic_load (sd, cpu, ldso_bfd, &sp, elf_addrs, &ldso_path))
	sim_io_eprintf (sd, "bfin-sim: FDPIC ldso failed to load: %s\n", ldso_full_path);
      if (ldso_path)
	sim_io_eprintf (sd, "bfin-sim: FDPIC ldso (%s) needs an interpreter (%s) !?\n",
			ldso_full_path, ldso_path);

      ldso_loadmap = sp;
    }
  else
 static_fdpic:
    ldso_loadmap = 0;

  /* Finally setup the registers required by the FDPIC ABI.  */
  SET_DREG (7, 0); /* Zero out FINI funcptr -- ldso will set this up.  */
  SET_PREG (0, exec_loadmap); /* Exec loadmap addr.  */
  SET_PREG (1, ldso_loadmap); /* Interp loadmap addr.  */
  SET_PREG (2, elf_addrs[5]); /* PT_DYNAMIC map addr.  */

  auxvt = 1;
  SET_SPREG (sp);
 skip_fdpic_init:
  sim_pc_set (cpu, elf_addrs[0]);

  /* Figure out how much storage the argv/env strings need.  */
  argc = countargv ((char **)argv);
  if (argc == -1)
    argc = 0;
  argv_flat = argc; /* NUL bytes  */
  for (i = 0; i < argc; ++i)
    argv_flat += strlen (argv[i]);

  if (!env)
    env = simple_env;
  envc = countargv ((char **)env);
  env_flat = envc; /* NUL bytes  */
  for (i = 0; i < envc; ++i)
    env_flat += strlen (env[i]);

  /* Push the Auxiliary Vector Table between argv/env and actual strings.  */
  sp_flat = sp = align_up (SPREG - argv_flat - env_flat - 4, 4);
  if (auxvt)
    {
# define AT_PUSH(at, val) \
  sp -= 4; \
  auxvt = (val); \
  sim_write (sd, sp, &auxvt, 4); \
  sp -= 4; \
  auxvt = (at); \
  sim_write (sd, sp, &auxvt, 4)
      unsigned int egid = getegid (), gid = getgid ();
      unsigned int euid = geteuid (), uid = getuid ();
      AT_PUSH (AT_NULL, 0);
      AT_PUSH (AT_SECURE, egid != gid || euid != uid);
      AT_PUSH (AT_EGID, egid);
      AT_PUSH (AT_GID, gid);
      AT_PUSH (AT_EUID, euid);
      AT_PUSH (AT_UID, uid);
      AT_PUSH (AT_ENTRY, elf_addrs[4]);
      AT_PUSH (AT_FLAGS, 0);
      AT_PUSH (AT_BASE, elf_addrs[3]);
      AT_PUSH (AT_PHNUM, elf_addrs[2]);
      AT_PUSH (AT_PHENT, sizeof (Elf32_External_Phdr));
      AT_PUSH (AT_PHDR, elf_addrs[1]);
      AT_PUSH (AT_CLKTCK, 100); /* XXX: This ever not 100 ?  */
      AT_PUSH (AT_PAGESZ, 4096);
      AT_PUSH (AT_HWCAP, 0);
#undef AT_PUSH
    }
  SET_SPREG (sp);

  /* Push the argc/argv/env after the auxvt.  */
  sp -= ((1 + argc + 1 + envc + 1) * 4);
  SET_SPREG (sp);

  /* First push the argc value.  */
  sim_write (sd, sp, &argc, 4);
  sp += 4;

  /* Then the actual argv strings so we know where to point argv[].  */
  for (i = 0; i < argc; ++i)
    {
      unsigned len = strlen (argv[i]) + 1;
      sim_write (sd, sp_flat, argv[i], len);
      sim_write (sd, sp, &sp_flat, 4);
      sp_flat += len;
      sp += 4;
    }
  sim_write (sd, sp, null, 4);
  sp += 4;

  /* Then the actual env strings so we know where to point env[].  */
  for (i = 0; i < envc; ++i)
    {
      unsigned len = strlen (env[i]) + 1;
      sim_write (sd, sp_flat, env[i], len);
      sim_write (sd, sp, &sp_flat, 4);
      sp_flat += len;
      sp += 4;
    }

  /* Set some callbacks.  */
  cb->syscall_map = cb_linux_syscall_map;
  cb->errno_map = cb_linux_errno_map;
  cb->open_map = cb_linux_open_map;
  cb->signal_map = cb_linux_signal_map;
  cb->stat_map = stat_map_32 = cb_linux_stat_map_32;
  stat_map_64 = cb_linux_stat_map_64;
}

static void
bfin_os_init (SIM_DESC sd, SIM_CPU *cpu, char * const *argv)
{
  /* Pass the command line via a string in R0 like Linux expects.  */
  int i;
  bu8 byte;
  bu32 cmdline = BFIN_L1_SRAM_SCRATCH;

  SET_DREG (0, cmdline);
  if (argv && argv[0])
    {
      i = 1;
      byte = ' ';
      while (argv[i])
	{
	  bu32 len = strlen (argv[i]);
	  sim_write (sd, cmdline, argv[i], len);
	  cmdline += len;
	  sim_write (sd, cmdline, &byte, 1);
	  ++cmdline;
	  ++i;
	}
    }
  byte = 0;
  sim_write (sd, cmdline, &byte, 1);
}

static void
bfin_virtual_init (SIM_DESC sd, SIM_CPU *cpu)
{
  host_callback *cb = STATE_CALLBACK (sd);

  cb->stat_map = stat_map_32 = cb_libgloss_stat_map_32;
  stat_map_64 = NULL;
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
		     char * const *argv, char * const *env)
{
  SIM_CPU *cpu = STATE_CPU (sd, 0);
  host_callback *cb = STATE_CALLBACK (sd);
  bfd_vma addr;

  /* Set the PC.  */
  if (abfd != NULL)
    addr = bfd_get_start_address (abfd);
  else
    addr = 0;
  sim_pc_set (cpu, addr);

  /* Standalone mode (i.e. `run`) will take care of the argv for us in
     sim_open() -> sim_parse_args().  But in debug mode (i.e. 'target sim'
     with `gdb`), we need to handle it because the user can change the
     argv on the fly via gdb's 'run'.  */
  if (STATE_PROG_ARGV (sd) != argv)
    {
      freeargv (STATE_PROG_ARGV (sd));
      STATE_PROG_ARGV (sd) = dupargv (argv);
    }

  if (STATE_PROG_ENVP (sd) != env)
    {
      freeargv (STATE_PROG_ENVP (sd));
      STATE_PROG_ENVP (sd) = dupargv (env);
    }

  cb->argv = STATE_PROG_ARGV (sd);
  cb->envp = STATE_PROG_ENVP (sd);

  switch (STATE_ENVIRONMENT (sd))
    {
    case USER_ENVIRONMENT:
      bfin_user_init (sd, cpu, abfd, argv, env);
      break;
    case OPERATING_ENVIRONMENT:
      bfin_os_init (sd, cpu, argv);
      break;
    default:
      bfin_virtual_init (sd, cpu);
      break;
    }

  return SIM_RC_OK;
}
