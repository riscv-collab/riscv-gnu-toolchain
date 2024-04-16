/* Simulator for the Renesas (formerly Hitachi) / SuperH Inc. SH architecture.

   Written by Steve Chamberlain of Cygnus Support.
   sac@cygnus.com

   This file is part of SH sim


		THIS SOFTWARE IS NOT COPYRIGHTED

   Cygnus offers the following for use in the public domain.  Cygnus
   makes no warranty with regard to the software or it's performance
   and the user accepts the software "AS IS" with all faults.

   CYGNUS DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
   THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

*/

/* This must come before any other includes.  */
#include "defs.h"

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#ifdef HAVE_MMAP
#include <sys/mman.h>
# ifndef MAP_FAILED
#  define MAP_FAILED -1
# endif
# if !defined (MAP_ANONYMOUS) && defined (MAP_ANON)
#  define MAP_ANONYMOUS MAP_ANON
# endif
#endif

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif
#ifndef _WIN32
#include <sys/wait.h>
#endif

#include "bfd.h"
#include "sim/callback.h"
#include "sim/sim.h"
#include "sim/sim-sh.h"

#include "sim-main.h"
#include "sim-base.h"
#include "sim-options.h"

#include "target-newlib-syscall.h"

#include "sh-sim.h"

#include <math.h>

#ifdef _WIN32
#include <float.h>		/* Needed for _isnan() */
#ifndef isnan
#define isnan _isnan
#endif
#endif

#ifndef SIGBUS
#define SIGBUS SIGSEGV
#endif

#ifndef SIGQUIT
#define SIGQUIT SIGTERM
#endif

#ifndef SIGTRAP
#define SIGTRAP 5
#endif

/* TODO: Stop using these names.  */
#undef SEXT
#undef SEXT32

extern unsigned short sh_jump_table[], sh_dsp_table[0x1000], ppi_table[];

#define O_RECOMPILE 85
#define DEFINE_TABLE
#define DISASSEMBLER_TABLE

/* Define the rate at which the simulator should poll the host
   for a quit. */
#define POLL_QUIT_INTERVAL 0x60000

/* TODO: Move into sim_cpu.  */
saved_state_type saved_state;

struct loop_bounds { unsigned char *start, *end; };

/* These variables are at file scope so that functions other than
   sim_resume can use the fetch/store macros */

#define target_little_endian (CURRENT_TARGET_BYTE_ORDER == BFD_ENDIAN_LITTLE)
static int global_endianw, endianb;
static int target_dsp;
#define host_little_endian (HOST_BYTE_ORDER == BFD_ENDIAN_LITTLE)

static int maskw = 0;
static int maskl = 0;

/* Short hand definitions of the registers */

#define SBIT(x) ((x)&sbit)
#define R0 	saved_state.asregs.regs[0]
#define Rn 	saved_state.asregs.regs[n]
#define Rm 	saved_state.asregs.regs[m]
#define UR0 	(unsigned int) (saved_state.asregs.regs[0])
#define UR 	(unsigned int) R
#define UR 	(unsigned int) R
#define SR0 	saved_state.asregs.regs[0]
#define CREG(n)	(saved_state.asregs.cregs[(n)])
#define GBR 	saved_state.asregs.gbr
#define VBR 	saved_state.asregs.vbr
#define DBR 	saved_state.asregs.dbr
#define TBR 	saved_state.asregs.tbr
#define IBCR	saved_state.asregs.ibcr
#define IBNR	saved_state.asregs.ibnr
#define BANKN	(saved_state.asregs.ibnr & 0x1ff)
#define ME	((saved_state.asregs.ibnr >> 14) & 0x3)
#define SSR	saved_state.asregs.ssr
#define SPC	saved_state.asregs.spc
#define SGR 	saved_state.asregs.sgr
#define SREG(n)	(saved_state.asregs.sregs[(n)])
#define MACH 	saved_state.asregs.mach
#define MACL 	saved_state.asregs.macl
#define PR	saved_state.asregs.pr
#define FPUL	saved_state.asregs.fpul

#define PC insn_ptr



/* Alternate bank of registers r0-r7 */

/* Note: code controling SR handles flips between BANK0 and BANK1 */
#define Rn_BANK(n) (saved_state.asregs.bank[(n)])
#define SET_Rn_BANK(n, EXP) do { saved_state.asregs.bank[(n)] = (EXP); } while (0)


/* Manipulate SR */

#define SR_MASK_BO  (1 << 14)
#define SR_MASK_CS  (1 << 13)
#define SR_MASK_DMY (1 << 11)
#define SR_MASK_DMX (1 << 10)
#define SR_MASK_M (1 << 9)
#define SR_MASK_Q (1 << 8)
#define SR_MASK_I (0xf << 4)
#define SR_MASK_S (1 << 1)
#define SR_MASK_T (1 << 0)

#define SR_MASK_BL (1 << 28)
#define SR_MASK_RB (1 << 29)
#define SR_MASK_MD (1 << 30)
#define SR_MASK_RC 0x0fff0000
#define SR_RC_INCREMENT -0x00010000

#define BO	((saved_state.asregs.sr & SR_MASK_BO) != 0)
#define CS	((saved_state.asregs.sr & SR_MASK_CS) != 0)
#define M 	((saved_state.asregs.sr & SR_MASK_M) != 0)
#define Q 	((saved_state.asregs.sr & SR_MASK_Q) != 0)
#define S 	((saved_state.asregs.sr & SR_MASK_S) != 0)
#define T 	((saved_state.asregs.sr & SR_MASK_T) != 0)
#define LDST	((saved_state.asregs.ldst) != 0)

#define SR_BL ((saved_state.asregs.sr & SR_MASK_BL) != 0)
#define SR_RB ((saved_state.asregs.sr & SR_MASK_RB) != 0)
#define SR_MD ((saved_state.asregs.sr & SR_MASK_MD) != 0)
#define SR_DMY ((saved_state.asregs.sr & SR_MASK_DMY) != 0)
#define SR_DMX ((saved_state.asregs.sr & SR_MASK_DMX) != 0)
#define SR_RC ((saved_state.asregs.sr & SR_MASK_RC))

/* Note: don't use this for privileged bits */
#define SET_SR_BIT(EXP, BIT) \
do { \
  if ((EXP) & 1) \
    saved_state.asregs.sr |= (BIT); \
  else \
    saved_state.asregs.sr &= ~(BIT); \
} while (0)

#define SET_SR_BO(EXP) SET_SR_BIT ((EXP), SR_MASK_BO)
#define SET_SR_CS(EXP) SET_SR_BIT ((EXP), SR_MASK_CS)
#define SET_BANKN(EXP) \
do { \
  IBNR = (IBNR & 0xfe00) | ((EXP) & 0x1f); \
} while (0)
#define SET_ME(EXP) \
do { \
  IBNR = (IBNR & 0x3fff) | (((EXP) & 0x3) << 14); \
} while (0)
#define SET_SR_M(EXP) SET_SR_BIT ((EXP), SR_MASK_M)
#define SET_SR_Q(EXP) SET_SR_BIT ((EXP), SR_MASK_Q)
#define SET_SR_S(EXP) SET_SR_BIT ((EXP), SR_MASK_S)
#define SET_SR_T(EXP) SET_SR_BIT ((EXP), SR_MASK_T)
#define SET_LDST(EXP) (saved_state.asregs.ldst = ((EXP) != 0))

/* stc currently relies on being able to read SR without modifications.  */
#define GET_SR() (saved_state.asregs.sr - 0)

#define SET_SR(x) set_sr (x)

#define SET_RC(x) \
  (saved_state.asregs.sr \
   = (saved_state.asregs.sr & 0xf000ffff) | ((x) & 0xfff) << 16)

/* Manipulate FPSCR */

#define FPSCR_MASK_FR (1 << 21)
#define FPSCR_MASK_SZ (1 << 20)
#define FPSCR_MASK_PR (1 << 19)

#define FPSCR_FR  ((GET_FPSCR () & FPSCR_MASK_FR) != 0)
#define FPSCR_SZ  ((GET_FPSCR () & FPSCR_MASK_SZ) != 0)
#define FPSCR_PR  ((GET_FPSCR () & FPSCR_MASK_PR) != 0)

static void
set_fpscr1 (int x)
{
  int old = saved_state.asregs.fpscr;
  saved_state.asregs.fpscr = (x);
  /* swap the floating point register banks */
  if ((saved_state.asregs.fpscr ^ old) & FPSCR_MASK_FR
      /* Ignore bit change if simulating sh-dsp.  */
      && ! target_dsp)
    {
      union fregs_u tmpf = saved_state.asregs.fregs[0];
      saved_state.asregs.fregs[0] = saved_state.asregs.fregs[1];
      saved_state.asregs.fregs[1] = tmpf;
    }
}

/* sts relies on being able to read fpscr directly.  */
#define GET_FPSCR()  (saved_state.asregs.fpscr)
#define SET_FPSCR(x) \
do { \
  set_fpscr1 (x); \
} while (0)

#define DSR  (saved_state.asregs.fpscr)

#define RAISE_EXCEPTION(x) \
  (saved_state.asregs.exception = x, saved_state.asregs.insn_end = 0)

#define RAISE_EXCEPTION_IF_IN_DELAY_SLOT() \
  if (in_delay_slot) RAISE_EXCEPTION (SIGILL)

/* This function exists mainly for the purpose of setting a breakpoint to
   catch simulated bus errors when running the simulator under GDB.  */

static void
raise_exception (int x)
{
  RAISE_EXCEPTION (x);
}

static void
raise_buserror (void)
{
  raise_exception (SIGBUS);
}

#define PROCESS_SPECIAL_ADDRESS(addr, endian, ptr, bits_written, \
				forbidden_addr_bits, data, retval) \
do { \
  if (addr & forbidden_addr_bits) \
    { \
      raise_buserror (); \
      return retval; \
    } \
  else if ((addr & saved_state.asregs.xyram_select) \
	   == saved_state.asregs.xram_start) \
    ptr = (void *) &saved_state.asregs.xmem_offset[addr ^ endian]; \
  else if ((addr & saved_state.asregs.xyram_select) \
	   == saved_state.asregs.yram_start) \
    ptr = (void *) &saved_state.asregs.ymem_offset[addr ^ endian]; \
  else if ((unsigned) addr >> 24 == 0xf0 \
	   && bits_written == 32 && (data & 1) == 0) \
    /* This invalidates (if not associative) or might invalidate \
       (if associative) an instruction cache line.  This is used for \
       trampolines.  Since we don't simulate the cache, this is a no-op \
       as far as the simulator is concerned.  */ \
    return retval; \
  else \
    { \
      if (bits_written == 8 && addr > 0x5000000) \
	IOMEM (addr, 1, data); \
      /* We can't do anything useful with the other stuff, so fail.  */ \
      raise_buserror (); \
      return retval; \
    } \
} while (0)

/* FIXME: sim_resume should be renamed to sim_engine_run.  sim_resume
   being implemented by ../common/sim_resume.c and the below should
   make a call to sim_engine_halt */

#define BUSERROR(addr, mask) ((addr) & (mask))

#define WRITE_BUSERROR(addr, mask, data, addr_func) \
  do \
    { \
      if (addr & mask) \
	{ \
	  addr_func (addr, data); \
	  return; \
	} \
    } \
  while (0)

#define READ_BUSERROR(addr, mask, addr_func) \
  do \
    { \
      if (addr & mask) \
	return addr_func (addr); \
    } \
  while (0)

/* Define this to enable register lifetime checking.
   The compiler generates "add #0,rn" insns to mark registers as invalid,
   the simulator uses this info to call fail if it finds a ref to an invalid
   register before a def

   #define PARANOID
*/

#ifdef PARANOID
int valid[16];
#define CREF(x)  if (!valid[x]) fail ();
#define CDEF(x)  valid[x] = 1;
#define UNDEF(x) valid[x] = 0;
#else
#define CREF(x)
#define CDEF(x)
#define UNDEF(x)
#endif

static void parse_and_set_memory_size (SIM_DESC sd, const char *str);
static int IOMEM (int addr, int write, int value);
static struct loop_bounds get_loop_bounds (int, int, unsigned char *,
					   unsigned char *, int, int);
static void process_wlat_addr (int, int);
static void process_wwat_addr (int, int);
static void process_wbat_addr (int, int);
static int process_rlat_addr (int);
static int process_rwat_addr (int);
static int process_rbat_addr (int);

/* Floating point registers */

#define DR(n) (get_dr (n))
static double
get_dr (int n)
{
  n = (n & ~1);
  if (host_little_endian)
    {
      union
      {
	int i[2];
	double d;
      } dr;
      dr.i[1] = saved_state.asregs.fregs[0].i[n + 0];
      dr.i[0] = saved_state.asregs.fregs[0].i[n + 1];
      return dr.d;
    }
  else
    return (saved_state.asregs.fregs[0].d[n >> 1]);
}

#define SET_DR(n, EXP) set_dr ((n), (EXP))
static void
set_dr (int n, double exp)
{
  n = (n & ~1);
  if (host_little_endian)
    {
      union
      {
	int i[2];
	double d;
      } dr;
      dr.d = exp;
      saved_state.asregs.fregs[0].i[n + 0] = dr.i[1];
      saved_state.asregs.fregs[0].i[n + 1] = dr.i[0];
    }
  else
    saved_state.asregs.fregs[0].d[n >> 1] = exp;
}

#define SET_FI(n,EXP) (saved_state.asregs.fregs[0].i[(n)] = (EXP))
#define FI(n) (saved_state.asregs.fregs[0].i[(n)])

#define FR(n) (saved_state.asregs.fregs[0].f[(n)])
#define SET_FR(n,EXP) (saved_state.asregs.fregs[0].f[(n)] = (EXP))

#define XD_TO_XF(n) ((((n) & 1) << 5) | ((n) & 0x1e))
#define XF(n) (saved_state.asregs.fregs[(n) >> 5].i[(n) & 0x1f])
#define SET_XF(n,EXP) (saved_state.asregs.fregs[(n) >> 5].i[(n) & 0x1f] = (EXP))

#define RS saved_state.asregs.rs
#define RE saved_state.asregs.re
#define MOD (saved_state.asregs.mod)
#define SET_MOD(i) \
(MOD = (i), \
 MOD_ME = (unsigned) MOD >> 16 | (SR_DMY ? ~0xffff : (SR_DMX ? 0 : 0x10000)), \
 MOD_DELTA = (MOD & 0xffff) - ((unsigned) MOD >> 16))

#define DSP_R(n) saved_state.asregs.sregs[(n)]
#define DSP_GRD(n) DSP_R ((n) + 8)
#define GET_DSP_GRD(n) ((n | 2) == 7 ? SEXT (DSP_GRD (n)) : SIGN32 (DSP_R (n)))
#define A1 DSP_R (5)
#define A0 DSP_R (7)
#define X0 DSP_R (8)
#define X1 DSP_R (9)
#define Y0 DSP_R (10)
#define Y1 DSP_R (11)
#define M0 DSP_R (12)
#define A1G DSP_R (13)
#define M1 DSP_R (14)
#define A0G DSP_R (15)
/* DSP_R (16) / DSP_GRD (16) are used as a fake destination for pcmp.  */
#define MOD_ME DSP_GRD (17)
#define MOD_DELTA DSP_GRD (18)

#define FP_OP(n, OP, m) \
{ \
  if (FPSCR_PR) \
    { \
      if (((n) & 1) || ((m) & 1)) \
	RAISE_EXCEPTION (SIGILL); \
      else \
	SET_DR (n, (DR (n) OP DR (m))); \
    } \
  else \
    SET_FR (n, (FR (n) OP FR (m))); \
} while (0)

#define FP_UNARY(n, OP) \
{ \
  if (FPSCR_PR) \
    { \
      if ((n) & 1) \
	RAISE_EXCEPTION (SIGILL); \
      else \
	SET_DR (n, (OP (DR (n)))); \
    } \
  else \
    SET_FR (n, (OP (FR (n)))); \
} while (0)

#define FP_CMP(n, OP, m) \
{ \
  if (FPSCR_PR) \
    { \
      if (((n) & 1) || ((m) & 1)) \
	RAISE_EXCEPTION (SIGILL); \
      else \
	SET_SR_T (DR (n) OP DR (m)); \
    } \
  else \
    SET_SR_T (FR (n) OP FR (m)); \
} while (0)

static void
set_sr (int new_sr)
{
  /* do we need to swap banks */
  int old_gpr = SR_MD && SR_RB;
  int new_gpr = (new_sr & SR_MASK_MD) && (new_sr & SR_MASK_RB);
  if (old_gpr != new_gpr)
    {
      int i, tmp;
      for (i = 0; i < 8; i++)
	{
	  tmp = saved_state.asregs.bank[i];
	  saved_state.asregs.bank[i] = saved_state.asregs.regs[i];
	  saved_state.asregs.regs[i] = tmp;
	}
    }
  saved_state.asregs.sr = new_sr;
  SET_MOD (MOD);
}

static INLINE void
wlat_fast (unsigned char *memory, int x, int value, int maskl)
{
  int v = value;
  unsigned int *p = (unsigned int *) (memory + x);
  WRITE_BUSERROR (x, maskl, v, process_wlat_addr);
  *p = v;
}

static INLINE void
wwat_fast (unsigned char *memory, int x, int value, int maskw, int endianw)
{
  int v = value;
  unsigned short *p = (unsigned short *) (memory + (x ^ endianw));
  WRITE_BUSERROR (x, maskw, v, process_wwat_addr);
  *p = v;
}

static INLINE void
wbat_fast (unsigned char *memory, int x, int value, int maskb)
{
  unsigned char *p = memory + (x ^ endianb);
  WRITE_BUSERROR (x, maskb, value, process_wbat_addr);

  p[0] = value;
}

/* Read functions */

static INLINE int
rlat_fast (unsigned char *memory, int x, int maskl)
{
  unsigned int *p = (unsigned int *) (memory + x);
  READ_BUSERROR (x, maskl, process_rlat_addr);

  return *p;
}

static INLINE int
rwat_fast (unsigned char *memory, int x, int maskw, int endianw)
{
  unsigned short *p = (unsigned short *) (memory + (x ^ endianw));
  READ_BUSERROR (x, maskw, process_rwat_addr);

  return *p;
}

static INLINE int
riat_fast (unsigned char *insn_ptr, int endianw)
{
  unsigned short *p = (unsigned short *) ((uintptr_t) insn_ptr ^ endianw);

  return *p;
}

static INLINE int
rbat_fast (unsigned char *memory, int x, int maskb)
{
  unsigned char *p = memory + (x ^ endianb);
  READ_BUSERROR (x, maskb, process_rbat_addr);

  return *p;
}

#define RWAT(x) 	(rwat_fast (memory, x, maskw, endianw))
#define RLAT(x) 	(rlat_fast (memory, x, maskl))
#define RBAT(x)         (rbat_fast (memory, x, maskb))
#define RIAT(p)		(riat_fast ((p), endianw))
#define WWAT(x,v) 	(wwat_fast (memory, x, v, maskw, endianw))
#define WLAT(x,v) 	(wlat_fast (memory, x, v, maskl))
#define WBAT(x,v)       (wbat_fast (memory, x, v, maskb))

#define RUWAT(x)  (RWAT (x) & 0xffff)
#define RSWAT(x)  ((short) (RWAT (x)))
#define RSLAT(x)  ((long) (RLAT (x)))
#define RSBAT(x)  (SEXT (RBAT (x)))

#define RDAT(x, n) (do_rdat (memory, (x), (n), (maskl)))
static int
do_rdat (unsigned char *memory, int x, int n, int maskl)
{
  int f0;
  int f1;
  int i = (n & 1);
  int j = (n & ~1);
  f0 = rlat_fast (memory, x + 0, maskl);
  f1 = rlat_fast (memory, x + 4, maskl);
  saved_state.asregs.fregs[i].i[(j + 0)] = f0;
  saved_state.asregs.fregs[i].i[(j + 1)] = f1;
  return 0;
}

#define WDAT(x, n) (do_wdat (memory, (x), (n), (maskl)))
static int
do_wdat (unsigned char *memory, int x, int n, int maskl)
{
  int f0;
  int f1;
  int i = (n & 1);
  int j = (n & ~1);
  f0 = saved_state.asregs.fregs[i].i[(j + 0)];
  f1 = saved_state.asregs.fregs[i].i[(j + 1)];
  wlat_fast (memory, (x + 0), f0, maskl);
  wlat_fast (memory, (x + 4), f1, maskl);
  return 0;
}

static void
process_wlat_addr (int addr, int value)
{
  unsigned int *ptr;

  PROCESS_SPECIAL_ADDRESS (addr, endianb, ptr, 32, 3, value, );
  *ptr = value;
}

static void
process_wwat_addr (int addr, int value)
{
  unsigned short *ptr;

  PROCESS_SPECIAL_ADDRESS (addr, endianb, ptr, 16, 1, value, );
  *ptr = value;
}

static void
process_wbat_addr (int addr, int value)
{
  unsigned char *ptr;

  PROCESS_SPECIAL_ADDRESS (addr, endianb, ptr, 8, 0, value, );
  *ptr = value;
}

static int
process_rlat_addr (int addr)
{
  unsigned char *ptr;

  PROCESS_SPECIAL_ADDRESS (addr, endianb, ptr, -32, 3, -1, 0);
  return *ptr;
}

static int
process_rwat_addr (int addr)
{
  unsigned char *ptr;

  PROCESS_SPECIAL_ADDRESS (addr, endianb, ptr, -16, 1, -1, 0);
  return *ptr;
}

static int
process_rbat_addr (int addr)
{
  unsigned char *ptr;

  PROCESS_SPECIAL_ADDRESS (addr, endianb, ptr, -8, 0, -1, 0);
  return *ptr;
}

#define SEXT(x)     	(((x &  0xff) ^ (~0x7f))+0x80)
#define SEXT12(x)	(((x & 0xfff) ^ 0x800) - 0x800)
#define SEXTW(y)    	((int) ((short) y))
#if 0
#define SEXT32(x)	((int) ((x & 0xffffffff) ^ 0x80000000U) - 0x7fffffff - 1)
#else
#define SEXT32(x)	((int) (x))
#endif
#define SIGN32(x)	(SEXT32 (x) >> 31)

/* convert pointer from target to host value.  */
#define PT2H(x) ((x) + memory)
/* convert pointer from host to target value.  */
#define PH2T(x) ((x) - memory)

#define SKIP_INSN(p) ((p) += ((RIAT (p) & 0xfc00) == 0xf800 ? 4 : 2))

#define SET_NIP(x) nip = (x); CHECK_INSN_PTR (nip);

static int in_delay_slot = 0;
#define Delay_Slot(TEMPPC)  	iword = RIAT (TEMPPC); in_delay_slot = 1; goto top;

#define CHECK_INSN_PTR(p) \
do { \
  if (saved_state.asregs.exception || PH2T (p) & maskw) \
    saved_state.asregs.insn_end = 0; \
  else if (p < loop.end) \
    saved_state.asregs.insn_end = loop.end; \
  else \
    saved_state.asregs.insn_end = mem_end; \
} while (0)

#ifdef ACE_FAST

#define MA(n)
#define L(x)
#define TL(x)
#define TB(x)

#else

#define MA(n) \
  do { memstalls += ((((uintptr_t) PC & 3) != 0) ? (n) : ((n) - 1)); } while (0)

#define L(x)   thislock = x;
#define TL(x)  if ((x) == prevlock) stalls++;
#define TB(x,y)  if ((x) == prevlock || (y) == prevlock) stalls++;

#endif

#if defined(__GO32__)
int sim_memory_size = 19;
#else
int sim_memory_size = 30;
#endif

static int sim_profile_size = 17;
static int nsamples;

#undef TB
#define TB(x,y)

#define SMR1 (0x05FFFEC8)	/* Channel 1  serial mode register */
#define BRR1 (0x05FFFEC9)	/* Channel 1  bit rate register */
#define SCR1 (0x05FFFECA)	/* Channel 1  serial control register */
#define TDR1 (0x05FFFECB)	/* Channel 1  transmit data register */
#define SSR1 (0x05FFFECC)	/* Channel 1  serial status register */
#define RDR1 (0x05FFFECD)	/* Channel 1  receive data register */

#define SCI_RDRF  	 0x40	/* Recieve data register full */
#define SCI_TDRE	0x80	/* Transmit data register empty */

static int
IOMEM (int addr, int write, int value)
{
  if (write)
    {
      switch (addr)
	{
	case TDR1:
	  if (value != '\r')
	    {
	      putchar (value);
	      fflush (stdout);
	    }
	  break;
	}
    }
  else
    {
      switch (addr)
	{
	case RDR1:
	  return getchar ();
	}
    }
  return 0;
}

static int
get_now (void)
{
  return time (NULL);
}

static int
now_persec (void)
{
  return 1;
}

static FILE *profile_file;

static INLINE unsigned
swap (unsigned n)
{
  if (endianb)
    n = (n << 24 | (n & 0xff00) << 8
	 | (n & 0xff0000) >> 8 | (n & 0xff000000) >> 24);
  return n;
}

static INLINE unsigned short
swap16 (unsigned short n)
{
  if (endianb)
    n = n << 8 | (n & 0xff00) >> 8;
  return n;
}

static void
swapout (int n)
{
  if (profile_file)
    {
      union { char b[4]; int n; } u;
      u.n = swap (n);
      fwrite (u.b, 4, 1, profile_file);
    }
}

static void
swapout16 (int n)
{
  union { char b[4]; int n; } u;
  u.n = swap16 (n);
  fwrite (u.b, 2, 1, profile_file);
}

/* Turn a pointer in a register into a pointer into real memory. */

static char *
ptr (int x)
{
  return (char *) (x + saved_state.asregs.memory);
}

/* STR points to a zero-terminated string in target byte order.  Return
   the number of bytes that need to be converted to host byte order in order
   to use this string as a zero-terminated string on the host.
   (Not counting the rounding up needed to operate on entire words.)  */
static int
strswaplen (int str)
{
  unsigned char *memory = saved_state.asregs.memory;
  int end;
  int endian = endianb;

  if (! endian)
    return 0;
  end = str;
  for (end = str; memory[end ^ endian]; end++) ;
  return end - str + 1;
}

static void
strnswap (int str, int len)
{
  int *start, *end;

  if (! endianb || ! len)
    return;
  start = (int *) ptr (str & ~3);
  end = (int *) ptr (str + len);
  do
    {
      int old = *start;
      *start = (old << 24 | (old & 0xff00) << 8
		| (old & 0xff0000) >> 8 | (old & 0xff000000) >> 24);
      start++;
    }
  while (start < end);
}

/* Simulate a monitor trap, put the result into r0 and errno into r1
   return offset by which to adjust pc.  */

static int
trap (SIM_DESC sd, int i, int *regs, unsigned char *insn_ptr,
      unsigned char *memory, int maskl, int maskw, int endianw)
{
  host_callback *callback = STATE_CALLBACK (sd);
  char **prog_argv = STATE_PROG_ARGV (sd);

  switch (i)
    {
    case 1:
      printf ("%c", regs[0]);
      break;
    case 2:
      raise_exception (SIGQUIT);
      break;
    case 3:			/* FIXME: for backwards compat, should be removed */
    case 33:
      {
	unsigned int countp = * (unsigned int *) (insn_ptr + 4);

	WLAT (countp, RLAT (countp) + 1);
	return 6;
      }
    case 34:
      {
	int perrno = errno;
	errno = 0;

	switch (regs[4])
	  {

#if !defined(__GO32__) && !defined(_WIN32)
	  case TARGET_NEWLIB_SH_SYS_fork:
	    regs[0] = fork ();
	    break;
/* This would work only if endianness matched between host and target.
   Besides, it's quite dangerous.  */
#if 0
	  case TARGET_NEWLIB_SH_SYS_execve:
	    regs[0] = execve (ptr (regs[5]), (char **) ptr (regs[6]), 
			      (char **) ptr (regs[7]));
	    break;
	  case TARGET_NEWLIB_SH_SYS_execv:
	    regs[0] = execve (ptr (regs[5]), (char **) ptr (regs[6]), 0);
	    break;
#endif
	  case TARGET_NEWLIB_SH_SYS_pipe:
	    {
	      regs[0] = (BUSERROR (regs[5], maskl)
			 ? -EINVAL
			 : pipe ((int *) ptr (regs[5])));
	    }
	    break;

	  case TARGET_NEWLIB_SH_SYS_wait:
	    regs[0] = wait ((int *) ptr (regs[5]));
	    break;
#endif /* !defined(__GO32__) && !defined(_WIN32) */

	  case TARGET_NEWLIB_SH_SYS_read:
	    strnswap (regs[6], regs[7]);
	    regs[0]
	      = callback->read (callback, regs[5], ptr (regs[6]), regs[7]);
	    strnswap (regs[6], regs[7]);
	    break;
	  case TARGET_NEWLIB_SH_SYS_write:
	    strnswap (regs[6], regs[7]);
	    if (regs[5] == 1)
	      regs[0] = (int) callback->write_stdout (callback, 
						      ptr (regs[6]), regs[7]);
	    else
	      regs[0] = (int) callback->write (callback, regs[5], 
					       ptr (regs[6]), regs[7]);
	    strnswap (regs[6], regs[7]);
	    break;
	  case TARGET_NEWLIB_SH_SYS_lseek:
	    regs[0] = callback->lseek (callback,regs[5], regs[6], regs[7]);
	    break;
	  case TARGET_NEWLIB_SH_SYS_close:
	    regs[0] = callback->close (callback,regs[5]);
	    break;
	  case TARGET_NEWLIB_SH_SYS_open:
	    {
	      int len = strswaplen (regs[5]);
	      strnswap (regs[5], len);
	      regs[0] = callback->open (callback, ptr (regs[5]), regs[6]);
	      strnswap (regs[5], len);
	      break;
	    }
	  case TARGET_NEWLIB_SH_SYS_exit:
	    /* EXIT - caller can look in r5 to work out the reason */
	    raise_exception (SIGQUIT);
	    regs[0] = regs[5];
	    break;

	  case TARGET_NEWLIB_SH_SYS_stat:	/* added at hmsi */
	    /* stat system call */
	    {
	      struct stat host_stat;
	      int buf;
	      int len = strswaplen (regs[5]);

	      strnswap (regs[5], len);
	      regs[0] = stat (ptr (regs[5]), &host_stat);
	      strnswap (regs[5], len);

	      buf = regs[6];

	      WWAT (buf, host_stat.st_dev);
	      buf += 2;
	      WWAT (buf, host_stat.st_ino);
	      buf += 2;
	      WLAT (buf, host_stat.st_mode);
	      buf += 4;
	      WWAT (buf, host_stat.st_nlink);
	      buf += 2;
	      WWAT (buf, host_stat.st_uid);
	      buf += 2;
	      WWAT (buf, host_stat.st_gid);
	      buf += 2;
	      WWAT (buf, host_stat.st_rdev);
	      buf += 2;
	      WLAT (buf, host_stat.st_size);
	      buf += 4;
	      WLAT (buf, host_stat.st_atime);
	      buf += 4;
	      WLAT (buf, 0);
	      buf += 4;
	      WLAT (buf, host_stat.st_mtime);
	      buf += 4;
	      WLAT (buf, 0);
	      buf += 4;
	      WLAT (buf, host_stat.st_ctime);
	      buf += 4;
	      WLAT (buf, 0);
	      buf += 4;
	      WLAT (buf, 0);
	      buf += 4;
	      WLAT (buf, 0);
	      buf += 4;
	    }
	    break;

#ifndef _WIN32
	  case TARGET_NEWLIB_SH_SYS_chown:
	    {
	      int len = strswaplen (regs[5]);

	      strnswap (regs[5], len);
	      regs[0] = chown (ptr (regs[5]), regs[6], regs[7]);
	      strnswap (regs[5], len);
	      break;
	    }
#endif /* _WIN32 */
	  case TARGET_NEWLIB_SH_SYS_chmod:
	    {
	      int len = strswaplen (regs[5]);

	      strnswap (regs[5], len);
	      regs[0] = chmod (ptr (regs[5]), regs[6]);
	      strnswap (regs[5], len);
	      break;
	    }
	  case TARGET_NEWLIB_SH_SYS_utime:
	    {
	      /* Cast the second argument to void *, to avoid type mismatch
		 if a prototype is present.  */
	      int len = strswaplen (regs[5]);

	      strnswap (regs[5], len);
#ifdef HAVE_UTIME_H
	      regs[0] = utime (ptr (regs[5]), (void *) ptr (regs[6]));
#else
	      errno = ENOSYS;
	      regs[0] = -1;
#endif
	      strnswap (regs[5], len);
	      break;
	    }
	  case TARGET_NEWLIB_SH_SYS_argc:
	    regs[0] = countargv (prog_argv);
	    break;
	  case TARGET_NEWLIB_SH_SYS_argnlen:
	    if (regs[5] < countargv (prog_argv))
	      regs[0] = strlen (prog_argv[regs[5]]);
	    else
	      regs[0] = -1;
	    break;
	  case TARGET_NEWLIB_SH_SYS_argn:
	    if (regs[5] < countargv (prog_argv))
	      {
		/* Include the termination byte.  */
		int len = strlen (prog_argv[regs[5]]) + 1;
		regs[0] = sim_write (0, regs[6], prog_argv[regs[5]], len);
	      }
	    else
	      regs[0] = -1;
	    break;
	  case TARGET_NEWLIB_SH_SYS_time:
	    regs[0] = get_now ();
	    break;
	  case TARGET_NEWLIB_SH_SYS_ftruncate:
	    regs[0] = callback->ftruncate (callback, regs[5], regs[6]);
	    break;
	  case TARGET_NEWLIB_SH_SYS_truncate:
	    {
	      int len = strswaplen (regs[5]);
	      strnswap (regs[5], len);
	      regs[0] = callback->truncate (callback, ptr (regs[5]), regs[6]);
	      strnswap (regs[5], len);
	      break;
	    }
	  default:
	    regs[0] = -1;
	    break;
	  }
	regs[1] = callback->get_errno (callback);
	errno = perrno;
      }
      break;

    case 13:	/* Set IBNR */
      IBNR = regs[0] & 0xffff;
      break;
    case 14:	/* Set IBCR */
      IBCR = regs[0] & 0xffff;
      break;
    case 0xc3:
    case 255:
      raise_exception (SIGTRAP);
      if (i == 0xc3)
	return -2;
      break;
    }
  return 0;
}

static void
div1 (int *R, int iRn2, int iRn1/*, int T*/)
{
  unsigned long tmp0;
  unsigned char old_q, tmp1;

  old_q = Q;
  SET_SR_Q ((unsigned char) ((0x80000000 & R[iRn1]) != 0));
  R[iRn1] <<= 1;
  R[iRn1] |= (unsigned long) T;

  if (!old_q)
    {
      if (!M)
	{
	  tmp0 = R[iRn1];
	  R[iRn1] -= R[iRn2];
	  tmp1 = (R[iRn1] > tmp0);
	  if (!Q)
	    SET_SR_Q (tmp1);
	  else
	    SET_SR_Q ((unsigned char) (tmp1 == 0));
	}
      else
	{
	  tmp0 = R[iRn1];
	  R[iRn1] += R[iRn2];
	  tmp1 = (R[iRn1] < tmp0);
	  if (!Q)
	    SET_SR_Q ((unsigned char) (tmp1 == 0));
	  else
	    SET_SR_Q (tmp1);
	}
    }
  else
    {
      if (!M)
	{
	  tmp0 = R[iRn1];
	  R[iRn1] += R[iRn2];
	  tmp1 = (R[iRn1] < tmp0);
	  if (!Q)
	    SET_SR_Q (tmp1);
	  else
	    SET_SR_Q ((unsigned char) (tmp1 == 0));
	}
      else
	{
	  tmp0 = R[iRn1];
	  R[iRn1] -= R[iRn2];
	  tmp1 = (R[iRn1] > tmp0);
	  if (!Q)
	    SET_SR_Q ((unsigned char) (tmp1 == 0));
	  else
	    SET_SR_Q (tmp1);
	}
    }
  /*T = (Q == M);*/
  SET_SR_T (Q == M);
  /*return T;*/
}

static void
dmul_s (uint32_t rm, uint32_t rn)
{
  int64_t res = (int64_t)(int32_t)rm * (int64_t)(int32_t)rn;
  MACH = (uint32_t)((uint64_t)res >> 32);
  MACL = (uint32_t)res;
}

static void
dmul_u (uint32_t rm, uint32_t rn)
{
  uint64_t res = (uint64_t)(uint32_t)rm * (uint64_t)(uint32_t)rn;
  MACH = (uint32_t)(res >> 32);
  MACL = (uint32_t)res;
}

static void
macw (int *regs, unsigned char *memory, int n, int m, int endianw)
{
  long tempm, tempn;
  long prod, macl, sum;

  tempm=RSWAT (regs[m]); regs[m]+=2;
  tempn=RSWAT (regs[n]); regs[n]+=2;

  macl = MACL;
  prod = (long) (short) tempm * (long) (short) tempn;
  sum = prod + macl;
  if (S)
    {
      if ((~(prod ^ macl) & (sum ^ prod)) < 0)
	{
	  /* MACH's lsb is a sticky overflow bit.  */
	  MACH |= 1;
	  /* Store the smallest negative number in MACL if prod is
	     negative, and the largest positive number otherwise.  */
	  sum = 0x7fffffff + (prod < 0);
	}
    }
  else
    {
      long mach;
      /* Add to MACH the sign extended product, and carry from low sum.  */
      mach = MACH + (-(prod < 0)) + ((unsigned long) sum < prod);
      /* Sign extend at 10:th bit in MACH.  */
      MACH = (mach & 0x1ff) | -(mach & 0x200);
    }
  MACL = sum;
}

static void
macl (int *regs, unsigned char *memory, int n, int m)
{
  long tempm, tempn;
  long macl, mach;
  long long ans;
  long long mac64;

  tempm = RSLAT (regs[m]);
  regs[m] += 4;

  tempn = RSLAT (regs[n]);
  regs[n] += 4;

  mach = MACH;
  macl = MACL;

  mac64 = ((long long) macl & 0xffffffff) |
          ((long long) mach & 0xffffffff) << 32;

  ans = (long long) tempm * (long long) tempn; /* Multiply 32bit * 32bit */

  mac64 += ans; /* Accumulate 64bit + 64 bit */

  macl = (long) (mac64 & 0xffffffff);
  mach = (long) ((mac64 >> 32) & 0xffffffff);

  if (S)  /* Store only 48 bits of the result */
    {
      if (mach < 0) /* Result is negative */
        {
          mach = mach & 0x0000ffff; /* Mask higher 16 bits */
          mach |= 0xffff8000; /* Sign extend higher 16 bits */
        }
      else
        mach = mach & 0x00007fff; /* Postive Result */
    }

  MACL = macl;
  MACH = mach;
}

enum {
  B_BCLR = 0,
  B_BSET = 1,
  B_BST  = 2,
  B_BLD  = 3,
  B_BAND = 4,
  B_BOR  = 5,
  B_BXOR = 6,
  B_BLDNOT = 11,
  B_BANDNOT = 12,
  B_BORNOT = 13,
  
  MOVB_RM = 0x0000,
  MOVW_RM = 0x1000,
  MOVL_RM = 0x2000,
  FMOV_RM = 0x3000,
  MOVB_MR = 0x4000,
  MOVW_MR = 0x5000,
  MOVL_MR = 0x6000,
  FMOV_MR = 0x7000,
  MOVU_BMR = 0x8000,
  MOVU_WMR = 0x9000,
};

/* Do extended displacement move instructions.  */
static void
do_long_move_insn (int op, int disp12, int m, int n, int *thatlock)
{
  int memstalls = 0;
  int thislock = *thatlock;
  int endianw = global_endianw;
  int *R = &(saved_state.asregs.regs[0]);
  unsigned char *memory = saved_state.asregs.memory;
  int maskb = ~((saved_state.asregs.msize - 1) & ~0);
  unsigned char *insn_ptr = PT2H (saved_state.asregs.pc);

  switch (op) {
  case MOVB_RM:		/* signed */
    WBAT (disp12 * 1 + R[n], R[m]); 
    break;
  case MOVW_RM:
    WWAT (disp12 * 2 + R[n], R[m]); 
    break;
  case MOVL_RM:
    WLAT (disp12 * 4 + R[n], R[m]); 
    break;
  case FMOV_RM:		/* floating point */
    if (FPSCR_SZ) 
      {
        MA (1);
        WDAT (R[n] + 8 * disp12, m);
      }
    else 
      WLAT (R[n] + 4 * disp12, FI (m));
    break;
  case MOVB_MR:
    R[n] = RSBAT (disp12 * 1 + R[m]);
    L (n); 
    break;
  case MOVW_MR:
    R[n] = RSWAT (disp12 * 2 + R[m]);
    L (n); 
    break;
  case MOVL_MR:
    R[n] = RLAT (disp12 * 4 + R[m]);
    L (n); 
    break;
  case FMOV_MR:
    if (FPSCR_SZ) {
      MA (1);
      RDAT (R[m] + 8 * disp12, n);
    }
    else 
      SET_FI (n, RLAT (R[m] + 4 * disp12));
    break;
  case MOVU_BMR:	/* unsigned */
    R[n] = RBAT (disp12 * 1 + R[m]);
    L (n);
    break;
  case MOVU_WMR:
    R[n] = RWAT (disp12 * 2 + R[m]);
    L (n);
    break;
  default:
    RAISE_EXCEPTION (SIGINT);
    exit (1);
  }
  saved_state.asregs.memstalls += memstalls;
  *thatlock = thislock;
}

/* Do binary logical bit-manipulation insns.  */
static void
do_blog_insn (int imm, int addr, int binop, 
	      unsigned char *memory, int maskb)
{
  int oldval = RBAT (addr);

  switch (binop) {
  case B_BCLR:	/* bclr.b */
    WBAT (addr, oldval & ~imm);
    break;
  case B_BSET:	/* bset.b */
    WBAT (addr, oldval | imm);
    break;
  case B_BST:	/* bst.b */
    if (T)
      WBAT (addr, oldval | imm);
    else
      WBAT (addr, oldval & ~imm);
    break;
  case B_BLD:	/* bld.b */
    SET_SR_T ((oldval & imm) != 0);
    break;
  case B_BAND:	/* band.b */
    SET_SR_T (T && ((oldval & imm) != 0));
    break;
  case B_BOR:	/* bor.b */
    SET_SR_T (T || ((oldval & imm) != 0));
    break;
  case B_BXOR:	/* bxor.b */
    SET_SR_T (T ^ ((oldval & imm) != 0));
    break;
  case B_BLDNOT:	/* bldnot.b */
    SET_SR_T ((oldval & imm) == 0);
    break;
  case B_BANDNOT:	/* bandnot.b */
    SET_SR_T (T && ((oldval & imm) == 0));
    break;
  case B_BORNOT:	/* bornot.b */
    SET_SR_T (T || ((oldval & imm) == 0));
    break;
  }
}

static float
fsca_s (int in, double (*f) (double))
{
  double rad = ldexp ((in & 0xffff), -15) * 3.141592653589793238462643383;
  double result = (*f) (rad);
  double error, upper, lower, frac;
  int exp;

  /* Search the value with the maximum error that is still within the
     architectural spec.  */
  error = ldexp (1., -21);
  /* compensate for calculation inaccuracy by reducing error.  */
  error = error - ldexp (1., -50);
  upper = result + error;
  frac = frexp (upper, &exp);
  upper = ldexp (floor (ldexp (frac, 24)), exp - 24);
  lower = result - error;
  frac = frexp (lower, &exp);
  lower = ldexp (ceil (ldexp (frac, 24)), exp - 24);
  return fabs (upper - result) >= fabs (lower - result) ? upper : lower;
}

static float
fsrra_s (float in)
{
  double result = 1. / sqrt (in);
  int exp;
  double frac, upper, lower, error, eps;

  /* refine result */
  result = result - (result * result * in - 1) * 0.5 * result;
  /* Search the value with the maximum error that is still within the
     architectural spec.  */
  frac = frexp (result, &exp);
  frac = ldexp (frac, 24);
  error = 4.0; /* 1 << 24-1-21 */
  /* use eps to compensate for possible 1 ulp error in our 'exact' result.  */
  eps = ldexp (1., -29);
  upper = floor (frac + error - eps);
  if (upper > 16777216.)
    upper = floor ((frac + error - eps) * 0.5) * 2.;
  lower = ceil ((frac - error + eps) * 2) * .5;
  if (lower > 8388608.)
    lower = ceil (frac - error + eps);
  upper = ldexp (upper, exp - 24);
  lower = ldexp (lower, exp - 24);
  return upper - result >= result - lower ? upper : lower;
}


/* GET_LOOP_BOUNDS {EXTENDED}
   These two functions compute the actual starting and ending point
   of the repeat loop, based on the RS and RE registers (repeat start, 
   repeat stop).  The extended version is called for LDRC, and the
   regular version is called for SETRC.  The difference is that for
   LDRC, the loop start and end instructions are literally the ones
   pointed to by RS and RE -- for SETRC, they're not (see docs).  */

static struct loop_bounds
get_loop_bounds_ext (int rs, int re, unsigned char *memory,
		     unsigned char *mem_end, int maskw, int endianw)
{
  struct loop_bounds loop;

  /* FIXME: should I verify RS < RE?  */
  loop.start = PT2H (RS);	/* FIXME not using the params?  */
  loop.end   = PT2H (RE & ~1);	/* Ignore bit 0 of RE.  */
  SKIP_INSN (loop.end);
  if (loop.end >= mem_end)
    loop.end = PT2H (0);
  return loop;
}

static struct loop_bounds
get_loop_bounds (int rs, int re, unsigned char *memory, unsigned char *mem_end,
		 int maskw, int endianw)
{
  struct loop_bounds loop;

  if (SR_RC)
    {
      if (RS >= RE)
	{
	  loop.start = PT2H (RE - 4);
	  SKIP_INSN (loop.start);
	  loop.end = loop.start;
	  if (RS - RE == 0)
	    SKIP_INSN (loop.end);
	  if (RS - RE <= 2)
	    SKIP_INSN (loop.end);
	  SKIP_INSN (loop.end);
	}
      else
	{
	  loop.start = PT2H (RS);
	  loop.end = PT2H (RE - 4);
	  SKIP_INSN (loop.end);
	  SKIP_INSN (loop.end);
	  SKIP_INSN (loop.end);
	  SKIP_INSN (loop.end);
	}
      if (loop.end >= mem_end)
	loop.end = PT2H (0);
    }
  else
    loop.end = PT2H (0);

  return loop;
}

#include "ppi.c"

/* Provide calloc / free versions that use an anonymous mmap.  This can
   significantly cut the start-up time when a large simulator memory is
   required, because pages are only zeroed on demand.  */
#ifdef MAP_ANONYMOUS
static void *
mcalloc (size_t nmemb, size_t size)
{
  if (nmemb != 1)
    size *= nmemb;
  return mmap (0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
	       -1, 0);
}

#define mfree(start,length) munmap ((start), (length))
#else
#define mcalloc calloc
#define mfree(start,length) free(start)
#endif

/* Set the memory size to the power of two provided. */

static void
sim_size (int power)
{
  sim_memory_size = power;

  if (saved_state.asregs.memory)
    {
      mfree (saved_state.asregs.memory, saved_state.asregs.msize);
    }

  saved_state.asregs.msize = 1 << power;

  saved_state.asregs.memory =
    (unsigned char *) mcalloc (1, saved_state.asregs.msize);

  if (!saved_state.asregs.memory)
    {
      fprintf (stderr,
	       "Not enough VM for simulation of %d bytes of RAM\n",
	       saved_state.asregs.msize);

      saved_state.asregs.msize = 1;
      saved_state.asregs.memory = (unsigned char *) mcalloc (1, 1);
    }
}

static void
init_dsp (struct bfd *abfd)
{
  int was_dsp = target_dsp;
  unsigned long mach = bfd_get_mach (abfd);

  if (mach == bfd_mach_sh_dsp  || 
      mach == bfd_mach_sh4al_dsp ||
      mach == bfd_mach_sh3_dsp)
    {
      int ram_area_size, xram_start, yram_start;
      int new_select;

      target_dsp = 1;
      if (mach == bfd_mach_sh_dsp)
	{
	  /* SH7410 (orig. sh-sdp):
	     4KB each for X & Y memory;
	     On-chip X RAM 0x0800f000-0x0800ffff
	     On-chip Y RAM 0x0801f000-0x0801ffff  */
	  xram_start = 0x0800f000;
	  ram_area_size = 0x1000;
	}
      if (mach == bfd_mach_sh3_dsp || mach == bfd_mach_sh4al_dsp)
	{
	  /* SH7612:
	     8KB each for X & Y memory;
	     On-chip X RAM 0x1000e000-0x1000ffff
	     On-chip Y RAM 0x1001e000-0x1001ffff  */
	  xram_start = 0x1000e000;
	  ram_area_size = 0x2000;
	}
      yram_start = xram_start + 0x10000;
      new_select = ~(ram_area_size - 1);
      if (saved_state.asregs.xyram_select != new_select)
	{
	  saved_state.asregs.xyram_select = new_select;
	  free (saved_state.asregs.xmem);
	  free (saved_state.asregs.ymem);
	  saved_state.asregs.xmem = 
	    (unsigned char *) calloc (1, ram_area_size);
	  saved_state.asregs.ymem = 
	    (unsigned char *) calloc (1, ram_area_size);

	  /* Disable use of X / Y mmeory if not allocated.  */
	  if (! saved_state.asregs.xmem || ! saved_state.asregs.ymem)
	    {
	      saved_state.asregs.xyram_select = 0;
	      if (saved_state.asregs.xmem)
		free (saved_state.asregs.xmem);
	      if (saved_state.asregs.ymem)
		free (saved_state.asregs.ymem);
	    }
	}
      saved_state.asregs.xram_start = xram_start;
      saved_state.asregs.yram_start = yram_start;
      saved_state.asregs.xmem_offset = saved_state.asregs.xmem - xram_start;
      saved_state.asregs.ymem_offset = saved_state.asregs.ymem - yram_start;
    }
  else
    {
      target_dsp = 0;
      if (saved_state.asregs.xyram_select)
	{
	  saved_state.asregs.xyram_select = 0;
	  free (saved_state.asregs.xmem);
	  free (saved_state.asregs.ymem);
	}
    }

  if (! saved_state.asregs.xyram_select)
    {
      saved_state.asregs.xram_start = 1;
      saved_state.asregs.yram_start = 1;
    }

  if (saved_state.asregs.regstack == NULL)
    saved_state.asregs.regstack = 
      calloc (512, sizeof *saved_state.asregs.regstack);

  if (target_dsp != was_dsp)
    {
      int i, tmp;

      for (i = ARRAY_SIZE (sh_dsp_table) - 1; i >= 0; i--)
	{
	  tmp = sh_jump_table[0xf000 + i];
	  sh_jump_table[0xf000 + i] = sh_dsp_table[i];
	  sh_dsp_table[i] = tmp;
	}
    }
}

static void
init_pointers (void)
{
  if (saved_state.asregs.msize != 1 << sim_memory_size)
    {
      sim_size (sim_memory_size);
    }

  if (saved_state.asregs.profile && !profile_file)
    {
      profile_file = fopen ("gmon.out", "wb");
      /* Seek to where to put the call arc data */
      nsamples = (1 << sim_profile_size);

      fseek (profile_file, nsamples * 2 + 12, 0);

      if (!profile_file)
	{
	  fprintf (stderr, "Can't open gmon.out\n");
	}
      else
	{
	  saved_state.asregs.profile_hist =
	    (unsigned short *) calloc (64, (nsamples * sizeof (short) / 64));
	}
    }
}

static void
dump_profile (void)
{
  unsigned int minpc;
  unsigned int maxpc;
  int i;

  minpc = 0;
  maxpc = (1 << sim_profile_size);

  fseek (profile_file, 0L, 0);
  swapout (minpc << PROFILE_SHIFT);
  swapout (maxpc << PROFILE_SHIFT);
  swapout (nsamples * 2 + 12);
  for (i = 0; i < nsamples; i++)
    swapout16 (saved_state.asregs.profile_hist[i]);

}

static void
gotcall (int from, int to)
{
  swapout (from);
  swapout (to);
  swapout (1);
}

#define MMASKB ((saved_state.asregs.msize -1) & ~0)

void
sim_resume (SIM_DESC sd, int step, int siggnal)
{
  register unsigned char *insn_ptr;
  unsigned char *mem_end;
  struct loop_bounds loop;
  register int cycles = 0;
  register int stalls = 0;
  register int memstalls = 0;
  register int insts = 0;
  register int prevlock;
#if 1
  int thislock;
#else
  register int thislock;
#endif
  register unsigned int doprofile;
  register int pollcount = 0;
  /* endianw is used for every insn fetch, hence it makes sense to cache it.
     endianb is used less often.  */
  register int endianw = global_endianw;

  int tick_start = get_now ();
  void (*prev_fpe) ();

  register unsigned short *jump_table = sh_jump_table;

  register int *R = &(saved_state.asregs.regs[0]);
  /*register int T;*/
#ifndef PR
  register int PR;
#endif

  register int maskb = ~((saved_state.asregs.msize - 1) & ~0);
  register int maskw = ~((saved_state.asregs.msize - 1) & ~1);
  register int maskl = ~((saved_state.asregs.msize - 1) & ~3);
  register unsigned char *memory;
  register unsigned int sbit = ((unsigned int) 1 << 31);

  prev_fpe = signal (SIGFPE, SIG_IGN);

  init_pointers ();
  saved_state.asregs.exception = 0;

  memory = saved_state.asregs.memory;
  mem_end = memory + saved_state.asregs.msize;

  if (RE & 1)
    loop = get_loop_bounds_ext (RS, RE, memory, mem_end, maskw, endianw);
  else
    loop = get_loop_bounds     (RS, RE, memory, mem_end, maskw, endianw);

  insn_ptr = PT2H (saved_state.asregs.pc);
  CHECK_INSN_PTR (insn_ptr);

#ifndef PR
  PR = saved_state.asregs.pr;
#endif
  /*T = GET_SR () & SR_MASK_T;*/
  prevlock = saved_state.asregs.prevlock;
  thislock = saved_state.asregs.thislock;
  doprofile = saved_state.asregs.profile;

  /* If profiling not enabled, disable it by asking for
     profiles infrequently. */
  if (doprofile == 0)
    doprofile = ~0;

 loop:
  if (step && insn_ptr < saved_state.asregs.insn_end)
    {
      if (saved_state.asregs.exception)
	/* This can happen if we've already been single-stepping and
	   encountered a loop end.  */
	saved_state.asregs.insn_end = insn_ptr;
      else
	{
	  saved_state.asregs.exception = SIGTRAP;
	  saved_state.asregs.insn_end = insn_ptr + 2;
	}
    }

  while (insn_ptr < saved_state.asregs.insn_end)
    {
      register unsigned int iword = RIAT (insn_ptr);
      register unsigned int ult;
      register unsigned char *nip = insn_ptr + 2;

#ifndef ACE_FAST
      insts++;
#endif
    top:

#include "code.c"


      in_delay_slot = 0;
      insn_ptr = nip;

      if (--pollcount < 0)
	{
	  host_callback *callback = STATE_CALLBACK (sd);

	  pollcount = POLL_QUIT_INTERVAL;
	  if ((*callback->poll_quit) != NULL
	      && (*callback->poll_quit) (callback))
	    {
	      sim_stop (sd);
	    }	    
	}

#ifndef ACE_FAST
      prevlock = thislock;
      thislock = 30;
      cycles++;

      if (cycles >= doprofile)
	{

	  saved_state.asregs.cycles += doprofile;
	  cycles -= doprofile;
	  if (saved_state.asregs.profile_hist)
	    {
	      int n = PH2T (insn_ptr) >> PROFILE_SHIFT;
	      if (n < nsamples)
		{
		  int i = saved_state.asregs.profile_hist[n];
		  if (i < 65000)
		    saved_state.asregs.profile_hist[n] = i + 1;
		}

	    }
	}
#endif
    }
  if (saved_state.asregs.insn_end == loop.end)
    {
      saved_state.asregs.sr += SR_RC_INCREMENT;
      if (SR_RC)
	insn_ptr = loop.start;
      else
	{
	  saved_state.asregs.insn_end = mem_end;
	  loop.end = PT2H (0);
	}
      goto loop;
    }

  if (saved_state.asregs.exception == SIGILL
      || saved_state.asregs.exception == SIGBUS)
    {
      insn_ptr -= 2;
    }
  /* Check for SIGBUS due to insn fetch.  */
  else if (! saved_state.asregs.exception)
    saved_state.asregs.exception = SIGBUS;

  saved_state.asregs.ticks += get_now () - tick_start;
  saved_state.asregs.cycles += cycles;
  saved_state.asregs.stalls += stalls;
  saved_state.asregs.memstalls += memstalls;
  saved_state.asregs.insts += insts;
  saved_state.asregs.pc = PH2T (insn_ptr);
#ifndef PR
  saved_state.asregs.pr = PR;
#endif

  saved_state.asregs.prevlock = prevlock;
  saved_state.asregs.thislock = thislock;

  if (profile_file)
    {
      dump_profile ();
    }

  signal (SIGFPE, prev_fpe);
}

uint64_t
sim_write (SIM_DESC sd, uint64_t addr, const void *buffer, uint64_t size)
{
  int i;
  const unsigned char *data = buffer;

  init_pointers ();

  for (i = 0; i < size; i++)
    {
      saved_state.asregs.memory[(MMASKB & (addr + i)) ^ endianb] = data[i];
    }
  return size;
}

uint64_t
sim_read (SIM_DESC sd, uint64_t addr, void *buffer, uint64_t size)
{
  int i;
  unsigned char *data = buffer;

  init_pointers ();

  for (i = 0; i < size; i++)
    {
      data[i] = saved_state.asregs.memory[(MMASKB & (addr + i)) ^ endianb];
    }
  return size;
}

static int gdb_bank_number;
enum {
  REGBANK_MACH = 15,
  REGBANK_IVN  = 16,
  REGBANK_PR   = 17,
  REGBANK_GBR  = 18,
  REGBANK_MACL = 19
};

static int
sh_reg_store (SIM_CPU *cpu, int rn, const void *memory, int length)
{
  unsigned val;

  init_pointers ();
  val = swap (* (int *) memory);
  switch (rn)
    {
    case SIM_SH_R0_REGNUM: case SIM_SH_R1_REGNUM: case SIM_SH_R2_REGNUM:
    case SIM_SH_R3_REGNUM: case SIM_SH_R4_REGNUM: case SIM_SH_R5_REGNUM:
    case SIM_SH_R6_REGNUM: case SIM_SH_R7_REGNUM: case SIM_SH_R8_REGNUM:
    case SIM_SH_R9_REGNUM: case SIM_SH_R10_REGNUM: case SIM_SH_R11_REGNUM:
    case SIM_SH_R12_REGNUM: case SIM_SH_R13_REGNUM: case SIM_SH_R14_REGNUM:
    case SIM_SH_R15_REGNUM:
      saved_state.asregs.regs[rn] = val;
      break;
    case SIM_SH_PC_REGNUM:
      saved_state.asregs.pc = val;
      break;
    case SIM_SH_PR_REGNUM:
      PR = val;
      break;
    case SIM_SH_GBR_REGNUM:
      GBR = val;
      break;
    case SIM_SH_VBR_REGNUM:
      VBR = val;
      break;
    case SIM_SH_MACH_REGNUM:
      MACH = val;
      break;
    case SIM_SH_MACL_REGNUM:
      MACL = val;
      break;
    case SIM_SH_SR_REGNUM:
      SET_SR (val);
      break;
    case SIM_SH_FPUL_REGNUM:
      FPUL = val;
      break;
    case SIM_SH_FPSCR_REGNUM:
      SET_FPSCR (val);
      break;
    case SIM_SH_FR0_REGNUM: case SIM_SH_FR1_REGNUM: case SIM_SH_FR2_REGNUM:
    case SIM_SH_FR3_REGNUM: case SIM_SH_FR4_REGNUM: case SIM_SH_FR5_REGNUM:
    case SIM_SH_FR6_REGNUM: case SIM_SH_FR7_REGNUM: case SIM_SH_FR8_REGNUM:
    case SIM_SH_FR9_REGNUM: case SIM_SH_FR10_REGNUM: case SIM_SH_FR11_REGNUM:
    case SIM_SH_FR12_REGNUM: case SIM_SH_FR13_REGNUM: case SIM_SH_FR14_REGNUM:
    case SIM_SH_FR15_REGNUM:
      SET_FI (rn - SIM_SH_FR0_REGNUM, val);
      break;
    case SIM_SH_DSR_REGNUM:
      DSR = val;
      break;
    case SIM_SH_A0G_REGNUM:
      A0G = val;
      break;
    case SIM_SH_A0_REGNUM:
      A0 = val;
      break;
    case SIM_SH_A1G_REGNUM:
      A1G = val;
      break;
    case SIM_SH_A1_REGNUM:
      A1 = val;
      break;
    case SIM_SH_M0_REGNUM:
      M0 = val;
      break;
    case SIM_SH_M1_REGNUM:
      M1 = val;
      break;
    case SIM_SH_X0_REGNUM:
      X0 = val;
      break;
    case SIM_SH_X1_REGNUM:
      X1 = val;
      break;
    case SIM_SH_Y0_REGNUM:
      Y0 = val;
      break;
    case SIM_SH_Y1_REGNUM:
      Y1 = val;
      break;
    case SIM_SH_MOD_REGNUM:
      SET_MOD (val);
      break;
    case SIM_SH_RS_REGNUM:
      RS = val;
      break;
    case SIM_SH_RE_REGNUM:
      RE = val;
      break;
    case SIM_SH_SSR_REGNUM:
      SSR = val;
      break;
    case SIM_SH_SPC_REGNUM:
      SPC = val;
      break;
    /* The rn_bank idiosyncracies are not due to hardware differences, but to
       a weird aliasing naming scheme for sh3 / sh3e / sh4.  */
    case SIM_SH_R0_BANK0_REGNUM: case SIM_SH_R1_BANK0_REGNUM:
    case SIM_SH_R2_BANK0_REGNUM: case SIM_SH_R3_BANK0_REGNUM:
    case SIM_SH_R4_BANK0_REGNUM: case SIM_SH_R5_BANK0_REGNUM:
    case SIM_SH_R6_BANK0_REGNUM: case SIM_SH_R7_BANK0_REGNUM:
      if (saved_state.asregs.bfd_mach == bfd_mach_sh2a)
	{
	  rn -= SIM_SH_R0_BANK0_REGNUM;
	  saved_state.asregs.regstack[gdb_bank_number].regs[rn] = val;
	}
      else
      if (SR_MD && SR_RB)
	Rn_BANK (rn - SIM_SH_R0_BANK0_REGNUM) = val;
      else
	saved_state.asregs.regs[rn - SIM_SH_R0_BANK0_REGNUM] = val;
      break;
    case SIM_SH_R0_BANK1_REGNUM: case SIM_SH_R1_BANK1_REGNUM:
    case SIM_SH_R2_BANK1_REGNUM: case SIM_SH_R3_BANK1_REGNUM:
    case SIM_SH_R4_BANK1_REGNUM: case SIM_SH_R5_BANK1_REGNUM:
    case SIM_SH_R6_BANK1_REGNUM: case SIM_SH_R7_BANK1_REGNUM:
      if (saved_state.asregs.bfd_mach == bfd_mach_sh2a)
	{
	  rn -= SIM_SH_R0_BANK1_REGNUM;
	  saved_state.asregs.regstack[gdb_bank_number].regs[rn + 8] = val;
	}
      else
      if (SR_MD && SR_RB)
	saved_state.asregs.regs[rn - SIM_SH_R0_BANK1_REGNUM] = val;
      else
	Rn_BANK (rn - SIM_SH_R0_BANK1_REGNUM) = val;
      break;
    case SIM_SH_R0_BANK_REGNUM: case SIM_SH_R1_BANK_REGNUM:
    case SIM_SH_R2_BANK_REGNUM: case SIM_SH_R3_BANK_REGNUM:
    case SIM_SH_R4_BANK_REGNUM: case SIM_SH_R5_BANK_REGNUM:
    case SIM_SH_R6_BANK_REGNUM: case SIM_SH_R7_BANK_REGNUM:
      SET_Rn_BANK (rn - SIM_SH_R0_BANK_REGNUM, val);
      break;
    case SIM_SH_TBR_REGNUM:
      TBR = val;
      break;
    case SIM_SH_IBNR_REGNUM:
      IBNR = val;
      break;
    case SIM_SH_IBCR_REGNUM:
      IBCR = val;
      break;
    case SIM_SH_BANK_REGNUM:
      /* This is a pseudo-register maintained just for gdb.
	 It tells us what register bank gdb would like to read/write.  */
      gdb_bank_number = val;
      break;
    case SIM_SH_BANK_MACL_REGNUM:
      saved_state.asregs.regstack[gdb_bank_number].regs[REGBANK_MACL] = val;
      break;
    case SIM_SH_BANK_GBR_REGNUM:
      saved_state.asregs.regstack[gdb_bank_number].regs[REGBANK_GBR] = val;
      break;
    case SIM_SH_BANK_PR_REGNUM:
      saved_state.asregs.regstack[gdb_bank_number].regs[REGBANK_PR] = val;
      break;
    case SIM_SH_BANK_IVN_REGNUM:
      saved_state.asregs.regstack[gdb_bank_number].regs[REGBANK_IVN] = val;
      break;
    case SIM_SH_BANK_MACH_REGNUM:
      saved_state.asregs.regstack[gdb_bank_number].regs[REGBANK_MACH] = val;
      break;
    default:
      return 0;
    }
  return length;
}

static int
sh_reg_fetch (SIM_CPU *cpu, int rn, void *memory, int length)
{
  int val;

  init_pointers ();
  switch (rn)
    {
    case SIM_SH_R0_REGNUM: case SIM_SH_R1_REGNUM: case SIM_SH_R2_REGNUM:
    case SIM_SH_R3_REGNUM: case SIM_SH_R4_REGNUM: case SIM_SH_R5_REGNUM:
    case SIM_SH_R6_REGNUM: case SIM_SH_R7_REGNUM: case SIM_SH_R8_REGNUM:
    case SIM_SH_R9_REGNUM: case SIM_SH_R10_REGNUM: case SIM_SH_R11_REGNUM:
    case SIM_SH_R12_REGNUM: case SIM_SH_R13_REGNUM: case SIM_SH_R14_REGNUM:
    case SIM_SH_R15_REGNUM:
      val = saved_state.asregs.regs[rn];
      break;
    case SIM_SH_PC_REGNUM:
      val = saved_state.asregs.pc;
      break;
    case SIM_SH_PR_REGNUM:
      val = PR;
      break;
    case SIM_SH_GBR_REGNUM:
      val = GBR;
      break;
    case SIM_SH_VBR_REGNUM:
      val = VBR;
      break;
    case SIM_SH_MACH_REGNUM:
      val = MACH;
      break;
    case SIM_SH_MACL_REGNUM:
      val = MACL;
      break;
    case SIM_SH_SR_REGNUM:
      val = GET_SR ();
      break;
    case SIM_SH_FPUL_REGNUM:
      val = FPUL;
      break;
    case SIM_SH_FPSCR_REGNUM:
      val = GET_FPSCR ();
      break;
    case SIM_SH_FR0_REGNUM: case SIM_SH_FR1_REGNUM: case SIM_SH_FR2_REGNUM:
    case SIM_SH_FR3_REGNUM: case SIM_SH_FR4_REGNUM: case SIM_SH_FR5_REGNUM:
    case SIM_SH_FR6_REGNUM: case SIM_SH_FR7_REGNUM: case SIM_SH_FR8_REGNUM:
    case SIM_SH_FR9_REGNUM: case SIM_SH_FR10_REGNUM: case SIM_SH_FR11_REGNUM:
    case SIM_SH_FR12_REGNUM: case SIM_SH_FR13_REGNUM: case SIM_SH_FR14_REGNUM:
    case SIM_SH_FR15_REGNUM:
      val = FI (rn - SIM_SH_FR0_REGNUM);
      break;
    case SIM_SH_DSR_REGNUM:
      val = DSR;
      break;
    case SIM_SH_A0G_REGNUM:
      val = SEXT (A0G);
      break;
    case SIM_SH_A0_REGNUM:
      val = A0;
      break;
    case SIM_SH_A1G_REGNUM:
      val = SEXT (A1G);
      break;
    case SIM_SH_A1_REGNUM:
      val = A1;
      break;
    case SIM_SH_M0_REGNUM:
      val = M0;
      break;
    case SIM_SH_M1_REGNUM:
      val = M1;
      break;
    case SIM_SH_X0_REGNUM:
      val = X0;
      break;
    case SIM_SH_X1_REGNUM:
      val = X1;
      break;
    case SIM_SH_Y0_REGNUM:
      val = Y0;
      break;
    case SIM_SH_Y1_REGNUM:
      val = Y1;
      break;
    case SIM_SH_MOD_REGNUM:
      val = MOD;
      break;
    case SIM_SH_RS_REGNUM:
      val = RS;
      break;
    case SIM_SH_RE_REGNUM:
      val = RE;
      break;
    case SIM_SH_SSR_REGNUM:
      val = SSR;
      break;
    case SIM_SH_SPC_REGNUM:
      val = SPC;
      break;
    /* The rn_bank idiosyncracies are not due to hardware differences, but to
       a weird aliasing naming scheme for sh3 / sh3e / sh4.  */
    case SIM_SH_R0_BANK0_REGNUM: case SIM_SH_R1_BANK0_REGNUM:
    case SIM_SH_R2_BANK0_REGNUM: case SIM_SH_R3_BANK0_REGNUM:
    case SIM_SH_R4_BANK0_REGNUM: case SIM_SH_R5_BANK0_REGNUM:
    case SIM_SH_R6_BANK0_REGNUM: case SIM_SH_R7_BANK0_REGNUM:
      if (saved_state.asregs.bfd_mach == bfd_mach_sh2a)
	{
	  rn -= SIM_SH_R0_BANK0_REGNUM;
	  val = saved_state.asregs.regstack[gdb_bank_number].regs[rn];
	}
      else
      val = (SR_MD && SR_RB
	     ? Rn_BANK (rn - SIM_SH_R0_BANK0_REGNUM)
	     : saved_state.asregs.regs[rn - SIM_SH_R0_BANK0_REGNUM]);
      break;
    case SIM_SH_R0_BANK1_REGNUM: case SIM_SH_R1_BANK1_REGNUM:
    case SIM_SH_R2_BANK1_REGNUM: case SIM_SH_R3_BANK1_REGNUM:
    case SIM_SH_R4_BANK1_REGNUM: case SIM_SH_R5_BANK1_REGNUM:
    case SIM_SH_R6_BANK1_REGNUM: case SIM_SH_R7_BANK1_REGNUM:
      if (saved_state.asregs.bfd_mach == bfd_mach_sh2a)
	{
	  rn -= SIM_SH_R0_BANK1_REGNUM;
	  val = saved_state.asregs.regstack[gdb_bank_number].regs[rn + 8];
	}
      else
      val = (! SR_MD || ! SR_RB
	     ? Rn_BANK (rn - SIM_SH_R0_BANK1_REGNUM)
	     : saved_state.asregs.regs[rn - SIM_SH_R0_BANK1_REGNUM]);
      break;
    case SIM_SH_R0_BANK_REGNUM: case SIM_SH_R1_BANK_REGNUM:
    case SIM_SH_R2_BANK_REGNUM: case SIM_SH_R3_BANK_REGNUM:
    case SIM_SH_R4_BANK_REGNUM: case SIM_SH_R5_BANK_REGNUM:
    case SIM_SH_R6_BANK_REGNUM: case SIM_SH_R7_BANK_REGNUM:
      val = Rn_BANK (rn - SIM_SH_R0_BANK_REGNUM);
      break;
    case SIM_SH_TBR_REGNUM:
      val = TBR;
      break;
    case SIM_SH_IBNR_REGNUM:
      val = IBNR;
      break;
    case SIM_SH_IBCR_REGNUM:
      val = IBCR;
      break;
    case SIM_SH_BANK_REGNUM:
      /* This is a pseudo-register maintained just for gdb.
	 It tells us what register bank gdb would like to read/write.  */
      val = gdb_bank_number;
      break;
    case SIM_SH_BANK_MACL_REGNUM:
      val = saved_state.asregs.regstack[gdb_bank_number].regs[REGBANK_MACL];
      break;
    case SIM_SH_BANK_GBR_REGNUM:
      val = saved_state.asregs.regstack[gdb_bank_number].regs[REGBANK_GBR];
      break;
    case SIM_SH_BANK_PR_REGNUM:
      val = saved_state.asregs.regstack[gdb_bank_number].regs[REGBANK_PR];
      break;
    case SIM_SH_BANK_IVN_REGNUM:
      val = saved_state.asregs.regstack[gdb_bank_number].regs[REGBANK_IVN];
      break;
    case SIM_SH_BANK_MACH_REGNUM:
      val = saved_state.asregs.regstack[gdb_bank_number].regs[REGBANK_MACH];
      break;
    default:
      return 0;
    }
  * (int *) memory = swap (val);
  return length;
}

void
sim_stop_reason (SIM_DESC sd, enum sim_stop *reason, int *sigrc)
{
  /* The SH simulator uses SIGQUIT to indicate that the program has
     exited, so we must check for it here and translate it to exit.  */
  if (saved_state.asregs.exception == SIGQUIT)
    {
      *reason = sim_exited;
      *sigrc = saved_state.asregs.regs[5];
    }
  else
    {
      *reason = sim_stopped;
      *sigrc = saved_state.asregs.exception;
    }
}

void
sim_info (SIM_DESC sd, bool verbose)
{
  double timetaken = 
    (double) saved_state.asregs.ticks / (double) now_persec ();
  double virttime = saved_state.asregs.cycles / 36.0e6;

  sim_io_printf (sd, "\n\n# instructions executed  %10d\n",
		 saved_state.asregs.insts);
  sim_io_printf (sd, "# cycles                 %10d\n",
		 saved_state.asregs.cycles);
  sim_io_printf (sd, "# pipeline stalls        %10d\n",
		 saved_state.asregs.stalls);
  sim_io_printf (sd, "# misaligned load/store  %10d\n",
		 saved_state.asregs.memstalls);
  sim_io_printf (sd, "# real time taken        %10.4f\n", timetaken);
  sim_io_printf (sd, "# virtual time taken     %10.4f\n", virttime);
  sim_io_printf (sd, "# profiling size         %10d\n", sim_profile_size);
  sim_io_printf (sd, "# profiling frequency    %10d\n",
		 saved_state.asregs.profile);
  sim_io_printf (sd, "# profile maxpc          %10x\n",
		 (1 << sim_profile_size) << PROFILE_SHIFT);

  if (timetaken != 0)
    {
      sim_io_printf (sd, "# cycles/second          %10d\n",
		     (int) (saved_state.asregs.cycles / timetaken));
      sim_io_printf (sd, "# simulation ratio       %10.4f\n",
		     virttime / timetaken);
    }
}

static sim_cia
sh_pc_get (sim_cpu *cpu)
{
  return saved_state.asregs.pc;
}

static void
sh_pc_set (sim_cpu *cpu, sim_cia pc)
{
  saved_state.asregs.pc = pc;
}

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *cb,
	  struct bfd *abfd, char * const *argv)
{
  char * const *p;
  int i;
  union
    {
      int i;
      short s[2];
      char c[4];
    }
  mem_word;

  SIM_DESC sd = sim_state_alloc (kind, cb);
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* Set default options before parsing user options.  */
  current_alignment = STRICT_ALIGNMENT;
  cb->syscall_map = cb_sh_syscall_map;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all (sd, 0) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* The parser will print an error message for us, so we silently return.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Check for/establish the a reference program image.  */
  if (sim_analyze_program (sd, STATE_PROG_FILE (sd), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Configure/verify the target byte order and other runtime
     configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      /* Uninstall the modules to avoid memory leaks,
	 file descriptor leaks, etc.  */
      sim_module_uninstall (sd);
      return 0;
    }

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU *cpu = STATE_CPU (sd, i);

      CPU_REG_FETCH (cpu) = sh_reg_fetch;
      CPU_REG_STORE (cpu) = sh_reg_store;
      CPU_PC_FETCH (cpu) = sh_pc_get;
      CPU_PC_STORE (cpu) = sh_pc_set;
    }

  for (p = argv + 1; *p != NULL; ++p)
    {
      if (isdigit (**p))
	parse_and_set_memory_size (sd, *p);
    }

  if (abfd)
    init_dsp (abfd);

  for (i = 4; (i -= 2) >= 0; )
    mem_word.s[i >> 1] = i;
  global_endianw = mem_word.i >> (target_little_endian ? 0 : 16) & 0xffff;

  for (i = 4; --i >= 0; )
    mem_word.c[i] = i;
  endianb = mem_word.i >> (target_little_endian ? 0 : 24) & 0xff;

  return sd;
}

static void
parse_and_set_memory_size (SIM_DESC sd, const char *str)
{
  int n;

  n = strtol (str, NULL, 10);
  if (n > 0 && n <= 31)
    sim_memory_size = n;
  else
    sim_io_printf (sd, "Bad memory size %d; must be 1 to 31, inclusive\n", n);
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *prog_bfd,
		     char * const *argv, char * const *env)
{
  /* Clear the registers. */
  memset (&saved_state, 0,
	  (char*) &saved_state.asregs.end_of_registers - (char*) &saved_state);

  /* Set the PC.  */
  if (prog_bfd != NULL)
    saved_state.asregs.pc = bfd_get_start_address (prog_bfd);

  /* Set the bfd machine type.  */
  if (prog_bfd != NULL)
    saved_state.asregs.bfd_mach = bfd_get_mach (prog_bfd);

  if (prog_bfd != NULL)
    init_dsp (prog_bfd);

  return SIM_RC_OK;
}

void
sim_do_command (SIM_DESC sd, const char *cmd)
{
  const char *sms_cmd = "set-memory-size";
  int cmdsize;

  if (cmd == NULL || *cmd == '\0')
    {
      cmd = "help";
    }

  cmdsize = strlen (sms_cmd);
  if (strncmp (cmd, sms_cmd, cmdsize) == 0 
      && strchr (" \t", cmd[cmdsize]) != NULL)
    {
      parse_and_set_memory_size (sd, cmd + cmdsize + 1);
    }
  else if (strcmp (cmd, "help") == 0)
    {
      sim_io_printf (sd, "List of SH simulator commands:\n\n");
      sim_io_printf (sd, "set-memory-size <n> -- Set the number of address bits to use\n");
      sim_io_printf (sd, "\n");
    }
  else
    {
      sim_io_printf (sd, "Error: \"%s\" is not a valid SH simulator command.\n", cmd);
    }
}
