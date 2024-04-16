/* cpu.h --- declarations for the RL78 core.

   Copyright (C) 2005-2024 Free Software Foundation, Inc.
   Contributed by Red Hat, Inc.

   This file is part of the GNU simulators.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SIM_RL78_CPU_H_
#define SIM_RL78_CPU_H_

#include <stdint.h>
#include <setjmp.h>

#include "opcode/rl78.h"

extern int verbose;
extern int trace;

typedef uint8_t QI;
typedef uint16_t HI;
typedef uint32_t SI;

extern int rl78_in_gdb;

SI get_reg (RL78_Register);
SI set_reg (RL78_Register, SI);

extern SI pc;


extern const char * const reg_names[];

void init_cpu (void);
void set_flags (int mask, int newbits);
void set_c (int);
int  get_c (void);

const char *bits (int v, int b);

int condition_true (RL78_Condition cond_id, int val);

/* Instruction step return codes.
   Suppose one of the decode_* functions below returns a value R:
   - If RL78_STEPPED (R), then the single-step completed normally.
   - If RL78_HIT_BREAK (R), then the program hit a breakpoint.
   - If RL78_EXITED (R), then the program has done an 'exit' system
     call, and the exit code is RL78_EXIT_STATUS (R).
   - If RL78_STOPPED (R), then a signal (number RL78_STOP_SIG (R)) was
     generated.

   For building step return codes:
   - RL78_MAKE_STEPPED is the return code for finishing a normal step.
   - RL78_MAKE_HIT_BREAK is the return code for hitting a breakpoint.
   - RL78_MAKE_EXITED (C) is the return code for exiting with status C.
   - RL78_MAKE_STOPPED (S) is the return code for stopping on signal S.  */
#define RL78_MAKE_STEPPED()   (1)
#define RL78_MAKE_HIT_BREAK() (2)
#define RL78_MAKE_EXITED(c)   (((int) (c) << 8) + 3)
#define RL78_MAKE_STOPPED(s)  (((int) (s) << 8) + 4)

#define RL78_STEPPED(r)       ((r) == RL78_MAKE_STEPPED ())
#define RL78_HIT_BREAK(r)     ((r) == RL78_MAKE_HIT_BREAK ())
#define RL78_EXITED(r)        (((r) & 0xff) == 3)
#define RL78_EXIT_STATUS(r)   ((r) >> 8)
#define RL78_STOPPED(r)       (((r) & 0xff) == 4)
#define RL78_STOP_SIG(r)      ((r) >> 8)

/* The step result for the current step.  Global to allow
   communication between the stepping function and the system
   calls.  */
extern int step_result;

extern int decode_opcode (void);

extern int trace_register_words;
extern void trace_register_changes (void);
extern void generate_access_exception (void);
extern jmp_buf decode_jmp_buf;

extern long long total_clocks;
extern int pending_clocks;
extern int timer_enabled;
extern void dump_counts_per_insn (const char * filename);
extern unsigned int counts_per_insn[0x100000];

extern int rl78_g10_mode;
extern int g13_multiply;
extern int g14_multiply;

#endif
