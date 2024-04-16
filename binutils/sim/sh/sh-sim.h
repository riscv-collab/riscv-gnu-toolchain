/* Moxie Simulator definition.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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
along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef SH_SIM_H
#define SH_SIM_H

typedef struct
{
  int regs[20];
} regstacktype;

typedef union
{

  struct
  {
    int regs[16];
    int pc;

    /* System registers.  For sh-dsp this also includes A0 / X0 / X1 / Y0 / Y1
       which are located in fregs.  Probably should include pc too - to avoid
       alignment repercussions.  */
    union {
      struct {
	int mach;
	int macl;
	int pr;
	int dummy3, dummy4;
	int fpul; /* A1 for sh-dsp -  but only for movs etc.  */
	int fpscr; /* dsr for sh-dsp */

	/* sh3e / sh-dsp */
	union fregs_u {
	  float f[16];
	  double d[8];
	  int i[16];
	} fregs[2];
      };
      int sregs[39];
    };

    /* Control registers; on the SH4, ldc / stc is privileged, except when
       accessing gbr.  */
    union
      {
	struct
	  {
	    int sr;
	    int gbr;
	    int vbr;
	    int ssr;
	    int spc;
	    int mod;
	    /* sh-dsp */
	    int rs;
	    int re;
	    /* sh3 */
	    int bank[8];
	    int dbr;		/* debug base register */
	    int sgr;		/* saved gr15 */
	    int ldst;		/* load/store flag (boolean) */
	    int tbr;
	    int ibcr;		/* sh2a bank control register */
	    int ibnr;		/* sh2a bank number register */
	  };
	int cregs[16];
      };

    unsigned char *insn_end;

    int ticks;
    int stalls;
    int memstalls;
    int cycles;
    int insts;

    int prevlock;
    int thislock;
    int exception;

    int end_of_registers;

    int msize;
#define PROFILE_FREQ 1
#define PROFILE_SHIFT 2
    int profile;
    unsigned short *profile_hist;
    unsigned char *memory;
    int xyram_select, xram_start, yram_start;
    unsigned char *xmem;
    unsigned char *ymem;
    unsigned char *xmem_offset;
    unsigned char *ymem_offset;
    unsigned long bfd_mach;
    regstacktype *regstack;
  } asregs;
  int asints[40];
} saved_state_type;

/* TODO: Move into sim_cpu.  */
extern saved_state_type saved_state;

#endif
