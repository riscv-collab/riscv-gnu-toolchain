/* This file is part of SIS (SPARC instruction simulator)

   Copyright (C) 1995-2024 Free Software Foundation, Inc.
   Contributed by Jiri Gaisler, European Space Agency

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

#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include "sis.h"
#include "libiberty.h"
#include "bfd.h"
#include <dis-asm.h>
#include "sim-config.h"

#include "sim/sim.h"
#include "gdb/signals.h"

#define PSR_CWP 0x7

extern struct disassemble_info dinfo;
extern struct pstate sregs;
extern struct estate ebase;

extern int      ctrl_c;
extern int      nfp;
extern int      ift;
extern int      rom8;
extern int      wrp;
extern int      uben;
extern int      sis_verbose;
extern char    *sis_version;
extern struct estate ebase;
extern struct evcell evbuf[];
extern struct irqcell irqarr[];
extern int      irqpend, ext_irl;
extern int      sparclite;
extern int      dumbio;
extern int      sparclite_board;
extern int      termsave;
extern char     uart_dev1[], uart_dev2[];

int             sis_gdb_break = 1;

host_callback *sim_callback;

int
run_sim(struct pstate *sregs, uint64_t icount, int dis)
{
    int             mexc, irq;

    if (sis_verbose)
	(*sim_callback->printf_filtered) (sim_callback, "resuming at %x\n",
					  sregs->pc);
   init_stdio();
   sregs->starttime = get_time();
   irq = 0;
   if ((sregs->pc != 0) && (ebase.simtime == 0))
	boot_init();
   while (!sregs->err_mode & (icount > 0)) {

	sregs->fhold = 0;
	sregs->hold = 0;
	sregs->icnt = 1;

        if (sregs->psr & 0x080)
            sregs->asi = 8;
        else
            sregs->asi = 9;

#if 0	/* DELETE ME! for debugging purposes only */
        if (sis_verbose > 1)
            if (sregs->pc == 0 || sregs->npc == 0)
                printf ("bogus pc or npc\n");
#endif
        mexc = memory_iread (sregs->pc, &sregs->inst, &sregs->hold);
#if 0	/* DELETE ME! for debugging purposes only */
        if (sis_verbose > 2)
            printf("pc %x, np %x, sp %x, fp %x, wm %x, cw %x, i %08x\n",
                   sregs->pc, sregs->npc,
                   sregs->r[(((sregs->psr & 7) << 4) + 14) & 0x7f],
                   sregs->r[(((sregs->psr & 7) << 4) + 30) & 0x7f],
                   sregs->wim,
                   sregs->psr & 7,
                   sregs->inst);
#endif
        if (sregs->annul) {
            sregs->annul = 0;
            sregs->icnt = 1;
            sregs->pc = sregs->npc;
            sregs->npc = sregs->npc + 4;
        } else {
	    if (ext_irl) irq = check_interrupts(sregs);
	    if (!irq) {
		if (mexc) {
		    sregs->trap = I_ACC_EXC;
		} else {
		    if ((sis_gdb_break) && (sregs->inst == 0x91d02001)) {
			if (sis_verbose)
			    (*sim_callback->printf_filtered) (sim_callback,
							      "SW BP hit at %x\n", sregs->pc);
                        sim_halt();
			restore_stdio();
			clearerr(stdin);
			return BPT_HIT;
		    } else
			dispatch_instruction(sregs);
		}
		icount--;
	    }
	    if (sregs->trap) {
                irq = 0;
		sregs->err_mode = execute_trap(sregs);
	    }
	}
	advance_time(sregs);
	if (ctrl_c) {
	    icount = 0;
	}
    }
    sim_halt();
    sregs->tottime += get_time() - sregs->starttime;
    restore_stdio();
    clearerr(stdin);
    if (sregs->err_mode)
	error_mode(sregs->pc);
    if (sregs->err_mode)
	return ERROR;
    if (sregs->bphit) {
	if (sis_verbose)
	    (*sim_callback->printf_filtered) (sim_callback,
					      "HW BP hit at %x\n", sregs->pc);
	return BPT_HIT;
    }
    if (ctrl_c) {
	ctrl_c = 0;
	return CTRL_C;
    }
    return TIME_OUT;
}

static int ATTRIBUTE_PRINTF (3, 4)
fprintf_styled (void *stream, enum disassembler_style style,
		const char *fmt, ...)
{
  int ret;
  FILE *out = (FILE *) stream;
  va_list args;

  va_start (args, fmt);
  ret = vfprintf (out, fmt, args);
  va_end (args);

  return ret;
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind, struct host_callback_struct *callback,
	  struct bfd *abfd, char * const *argv)
{

    int             argc = 0;
    int             stat = 1;
    int             freq = 0;

    sim_callback = callback;

    argc = countargv (argv);
    while (stat < argc) {
	if (argv[stat][0] == '-') {
	    if (strcmp(argv[stat], "-v") == 0) {
		sis_verbose++;
	    } else
	    if (strcmp(argv[stat], "-nfp") == 0) {
		nfp = 1;
	    } else
            if (strcmp(argv[stat], "-ift") == 0) {
                ift = 1;
	    } else
	    if (strcmp(argv[stat], "-sparclite") == 0) {
		sparclite = 1;
	    } else
	    if (strcmp(argv[stat], "-sparclite-board") == 0) {
		sparclite_board = 1;
            } else 
            if (strcmp(argv[stat], "-dumbio") == 0) {
		dumbio = 1;
	    } else
            if (strcmp(argv[stat], "-wrp") == 0) {
                wrp = 1;
	    } else
            if (strcmp(argv[stat], "-rom8") == 0) {
                rom8 = 1;
	    } else 
            if (strcmp(argv[stat], "-uben") == 0) {
                uben = 1;
	    } else 
	    if (strcmp(argv[stat], "-uart1") == 0) {
		if ((stat + 1) < argc)
		    strcpy(uart_dev1, argv[++stat]);
	    } else
	    if (strcmp(argv[stat], "-uart2") == 0) {
		if ((stat + 1) < argc)
		    strcpy(uart_dev2, argv[++stat]);
	    } else
	    if (strcmp(argv[stat], "-nogdb") == 0) {
		sis_gdb_break = 0;
	    } else
	    if (strcmp(argv[stat], "-freq") == 0) {
		if ((stat + 1) < argc) {
		    freq = strtol(argv[++stat], (char **)NULL, 0);
		}
	    } else
	    if (strncmp(argv[stat], "--sysroot=", sizeof("--sysroot=") - 1) == 0) {
		/* Ignore until we start to support this.  */
	    } else {
		(*sim_callback->printf_filtered) (sim_callback,
						  "unknown option %s\n",
						  argv[stat]);
	    }
	} else
	    bfd_load(argv[stat]);
	stat++;
    }

    if (sis_verbose) {
	(*sim_callback->printf_filtered) (sim_callback, "\n SIS - SPARC instruction simulator %s\n", sis_version);
	(*sim_callback->printf_filtered) (sim_callback, " Bug-reports to Jiri Gaisler ESA/ESTEC (jgais@wd.estec.esa.nl)\n");
	if (nfp)
	  (*sim_callback->printf_filtered) (sim_callback, "no FPU\n");
	if (sparclite)
	  (*sim_callback->printf_filtered) (sim_callback, "simulating Sparclite\n");
	if (dumbio)
	  (*sim_callback->printf_filtered) (sim_callback, "dumb IO (no input, dumb output)\n");
	if (sis_gdb_break == 0)
	  (*sim_callback->printf_filtered) (sim_callback, "disabling GDB trap handling for breakpoints\n");
	if (freq)
	  (*sim_callback->printf_filtered) (sim_callback, " ERC32 freq %d Mhz\n", freq);
    }

    sregs.freq = freq ? freq : 15;
#ifdef F_GETFL
    termsave = fcntl(0, F_GETFL, 0);
#endif
    INIT_DISASSEMBLE_INFO(dinfo, stdout,(fprintf_ftype)fprintf,
			  fprintf_styled);
#ifdef HOST_LITTLE_ENDIAN
    dinfo.endian = BFD_ENDIAN_LITTLE;
#else
    dinfo.endian = BFD_ENDIAN_BIG;
#endif
    reset_all();
    ebase.simtime = 0;
    init_sim();
    init_bpt(&sregs);
    reset_stat(&sregs);

    /* Fudge our descriptor for now.  */
    return (SIM_DESC) 1;
}

void
sim_close(SIM_DESC sd, int quitting)
{

    exit_sim();
#ifdef F_SETFL
    fcntl(0, F_SETFL, termsave);
#endif
}

SIM_RC
sim_load(SIM_DESC sd, const char *prog, bfd *abfd, int from_tty)
{
    bfd_load (prog);
    return SIM_RC_OK;
}

SIM_RC
sim_create_inferior(SIM_DESC sd, bfd *abfd, char * const *argv,
		    char * const *env)
{
    bfd_vma start_address = 0;
    if (abfd != NULL)
      start_address = bfd_get_start_address (abfd);

    ebase.simtime = 0;
    reset_all();
    reset_stat(&sregs);
    sregs.pc = start_address & ~3;
    sregs.npc = sregs.pc + 4;
    return SIM_RC_OK;
}

int
sim_store_register(SIM_DESC sd, int regno, const void *buf, int length)
{
    const unsigned char *value = buf;
    int regval;

    regval = (value[0] << 24) | (value[1] << 16)
		 | (value[2] << 8) | value[3];
    set_regi(&sregs, regno, regval);
    return length;
}


int
sim_fetch_register(SIM_DESC sd, int regno, void *buf, int length)
{
    get_regi(&sregs, regno, buf);
    return -1;
}

uint64_t
sim_write (SIM_DESC sd, uint64_t mem, const void *buffer, uint64_t length)
{
    int i;
    const unsigned char *data = buffer;

    for (i = 0; i < length; i++) {
	sis_memory_write ((mem + i) ^ EBT, &data[i], 1);
    }
    return length;
}

uint64_t
sim_read (SIM_DESC sd, uint64_t mem, void *buffer, uint64_t length)
{
    int i;
    unsigned char *data = buffer;

    for (i = 0; i < length; i++) {
	sis_memory_read ((mem + i) ^ EBT, &data[i], 1);
    }
    return length;
}

void
sim_info(SIM_DESC sd, bool verbose)
{
    show_stat(&sregs);
}

int             simstat = OK;

void
sim_stop_reason(SIM_DESC sd, enum sim_stop *reason, int *sigrc)
{

    switch (simstat) {
	case CTRL_C:
	*reason = sim_stopped;
	*sigrc = GDB_SIGNAL_INT;
	break;
    case OK:
    case TIME_OUT:
    case BPT_HIT:
	*reason = sim_stopped;
	*sigrc = GDB_SIGNAL_TRAP;
	break;
    case ERROR:
	*sigrc = 0;
	*reason = sim_exited;
    }
    ctrl_c = 0;
    simstat = OK;
}

/* Flush all register windows out to the stack.  Starting after the invalid
   window, flush all windows up to, and including the current window.  This
   allows GDB to do backtraces and look at local variables for frames that
   are still in the register windows.  Note that strictly speaking, this
   behavior is *wrong* for several reasons.  First, it doesn't use the window
   overflow handlers.  It therefore assumes standard frame layouts and window
   handling policies.  Second, it changes system state behind the back of the
   target program.  I expect this to mainly pose problems when debugging trap
   handlers.
*/

static void
flush_windows (void)
{
  int invwin;
  int cwp;
  int win;
  int ws;

  /* Keep current window handy */

  cwp = sregs.psr & PSR_CWP;

  /* Calculate the invalid window from the wim. */

  for (invwin = 0; invwin <= PSR_CWP; invwin++)
    if ((sregs.wim >> invwin) & 1)
      break;

  /* Start saving with the window after the invalid window. */

  invwin = (invwin - 1) & PSR_CWP;

  for (win = invwin; ; win = (win - 1) & PSR_CWP)
    {
      uint32_t sp;
      int i;

      sp = sregs.r[(win * 16 + 14) & 0x7f];
#if 1
      if (sis_verbose > 2) {
	uint32_t fp = sregs.r[(win * 16 + 30) & 0x7f];
	printf("flush_window: win %d, sp %x, fp %x\n", win, sp, fp);
      }
#endif

      for (i = 0; i < 16; i++)
	memory_write (11, sp + 4 * i, &sregs.r[(win * 16 + 16 + i) & 0x7f], 2,
		      &ws);

      if (win == cwp)
	break;
    }
}

void
sim_resume(SIM_DESC sd, int step, int siggnal)
{
    simstat = run_sim(&sregs, UINT64_MAX, 0);

    if (sis_gdb_break) flush_windows ();
}

void
sim_do_command(SIM_DESC sd, const char *cmd)
{
    exec_cmd(&sregs, cmd);
}

char **
sim_complete_command (SIM_DESC sd, const char *text, const char *word)
{
  return NULL;
}

char *
sim_memory_map (SIM_DESC sd)
{
  return NULL;
}

#if 0 /* FIXME: These shouldn't exist.  */

int
sim_insert_breakpoint(int addr)
{
    if (sregs.bptnum < BPT_MAX) {
	sregs.bpts[sregs.bptnum] = addr & ~0x3;
	sregs.bptnum++;
	if (sis_verbose)
	    (*sim_callback->printf_filtered) (sim_callback, "inserted HW BP at %x\n", addr);
	return 0;
    } else
	return 1;
}

int
sim_remove_breakpoint(int addr)
{
    int             i = 0;

    while ((i < sregs.bptnum) && (sregs.bpts[i] != addr))
	i++;
    if (addr == sregs.bpts[i]) {
	for (; i < sregs.bptnum - 1; i++)
	    sregs.bpts[i] = sregs.bpts[i + 1];
	sregs.bptnum -= 1;
	if (sis_verbose)
	    (*sim_callback->printf_filtered) (sim_callback, "removed HW BP at %x\n", addr);
	return 0;
    }
    return 1;
}

#endif
