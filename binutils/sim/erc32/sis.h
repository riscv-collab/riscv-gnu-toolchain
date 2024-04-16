/* Copyright (C) 1995-2024 Free Software Foundation, Inc.

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

#include "ansidecl.h"
#include "sim/callback.h"
#include "sim/sim.h"
#include <sim-config.h>
#include <stdint.h>
#include "dis-asm.h"

#if HOST_BYTE_ORDER == BIG_ENDIAN
#define HOST_BIG_ENDIAN
#define EBT 0
#else
#define HOST_LITTLE_ENDIAN
#define EBT 3
#endif

#define I_ACC_EXC 1

/* Maximum events in event queue */
#define EVENT_MAX	256

/* Maximum # of floating point queue */
#define FPUQN	1

/* Maximum # of breakpoints */
#define BPT_MAX	256

struct histype {
    unsigned        addr;
    unsigned        time;
};

/* type definitions */

typedef float   float32;	/* 32-bit float */
typedef double  float64;	/* 64-bit float */

struct pstate {

    float64         fd[16];	/* FPU registers */
#ifdef HOST_LITTLE_ENDIAN
    float32         fs[32];
    float32        *fdp;
#else
    float32        *fs;
#endif
    int32_t          *fsi;
    uint32_t          fsr;
    int32_t           fpstate;
    uint32_t          fpq[FPUQN * 2];
    uint32_t          fpqn;
    uint32_t          ftime;
    uint32_t          flrd;
    uint32_t          frd;
    uint32_t          frs1;
    uint32_t          frs2;
    uint32_t          fpu_pres;	/* FPU present (0 = No, 1 = Yes) */

    uint32_t          psr;	/* IU registers */
    uint32_t          tbr;
    uint32_t          wim;
    uint32_t          g[8];
    uint32_t          r[128];
    uint32_t          y;
    uint32_t          asr17;      /* Single vector trapping */
    uint32_t          pc, npc;


    uint32_t          trap;	/* Current trap type */
    uint32_t          annul;	/* Instruction annul */
    uint32_t          data;	/* Loaded data	     */
    uint32_t          inst;	/* Current instruction */
    uint32_t          asi;	/* Current ASI */
    uint32_t          err_mode;	/* IU error mode */
    uint32_t          breakpoint;
    uint32_t          bptnum;
    uint32_t          bphit;
    uint32_t          bpts[BPT_MAX];	/* Breakpoints */

    uint32_t          ltime;	/* Load interlock time */
    uint32_t          hold;	/* IU hold cycles in current inst */
    uint32_t          fhold;	/* FPU hold cycles in current inst */
    uint32_t          icnt;	/* Instruction cycles in curr inst */

    uint32_t          histlen;	/* Trace history management */
    uint32_t          histind;
    struct histype *histbuf;
    float32         freq;	/* Simulated processor frequency */


    double          tottime;
    uint64_t          ninst;
    uint64_t          fholdt;
    uint64_t          holdt;
    uint64_t          icntt;
    uint64_t          finst;
    uint64_t          simstart;
    double          starttime;
    uint64_t          tlimit;	/* Simulation time limit */
    uint64_t          pwdtime;	/* Cycles in power-down mode */
    uint64_t          nstore;	/* Number of load instructions */
    uint64_t          nload;	/* Number of store instructions */
    uint64_t          nannul;	/* Number of annuled instructions */
    uint64_t          nbranch;	/* Number of branch instructions */
    uint32_t          ildreg;	/* Destination of last load instruction */
    uint64_t          ildtime;	/* Last time point for load dependency */

    int             rett_err;	/* IU in jmpl/restore error state (Rev.0) */
    int             jmpltime;
};

struct evcell {
    void            (*cfunc) (int32_t);
    int32_t           arg;
    uint64_t          time;
    struct evcell  *nxt;
};

struct estate {
    struct evcell   eq;
    struct evcell  *freeq;
    uint64_t          simtime;
};

struct irqcell {
    void            (*callback) (int32_t);
    int32_t           arg;
};


#define OK 0
#define TIME_OUT 1
#define BPT_HIT 2
#define ERROR 3
#define CTRL_C 4

/* Prototypes  */

/* erc32.c */
extern void	init_sim (void);
extern void	reset (void);
extern void	error_mode (uint32_t pc);
extern void	sim_halt (void);
extern void	exit_sim (void);
extern void	init_stdio (void);
extern void	restore_stdio (void);
extern int	memory_iread (uint32_t addr, uint32_t *data, uint32_t *ws);
extern int	memory_read (int32_t asi, uint32_t addr, void *data,
			     int32_t sz, int32_t *ws);
extern int	memory_write (int32_t asi, uint32_t addr, uint32_t *data,
			      int32_t sz, int32_t *ws);
extern int	sis_memory_write (uint32_t addr,
				  const void *data, uint32_t length);
extern int	sis_memory_read (uint32_t addr, void *data,
				 uint32_t length);
extern void	boot_init (void);

/* func.c */
extern struct pstate  sregs;
extern void	set_regi (struct pstate *sregs, int32_t reg,
			  uint32_t rval);
extern void	get_regi (struct pstate *sregs, int32_t reg, unsigned char *buf);
extern int	exec_cmd (struct pstate *sregs, const char *cmd);
extern void	reset_stat (struct pstate  *sregs);
extern void	show_stat (struct pstate  *sregs);
extern void	init_bpt (struct pstate  *sregs);
extern void	init_signals (void);

struct disassemble_info;
extern void	dis_mem (uint32_t addr, uint32_t len,
			 struct disassemble_info *info);
extern void	event (void (*cfunc) (int32_t), int32_t arg, uint64_t delta);
extern void	set_int (int32_t level, void (*callback) (int32_t), int32_t arg);
extern void	advance_time (struct pstate  *sregs);
extern uint32_t	now (void);
extern int	wait_for_irq (void);
extern int	check_bpt (struct pstate *sregs);
extern void	reset_all (void);
extern void	sys_reset (void);
extern void	sys_halt (void);
extern int	bfd_load (const char *fname);
extern double	get_time (void);

/* exec.c */
extern int	dispatch_instruction (struct pstate *sregs);
extern int	execute_trap (struct pstate *sregs);
extern int	check_interrupts (struct pstate  *sregs);
extern void	init_regs (struct pstate *sregs);

/* interf.c */
extern int	run_sim (struct pstate *sregs,
			 uint64_t icount, int dis);

/* float.c */
extern int	get_accex (void);
extern void	clear_accex (void);
extern void	set_fsr (uint32_t fsr);

/* help.c */
extern void	usage (void);
extern void	gen_help (void);
