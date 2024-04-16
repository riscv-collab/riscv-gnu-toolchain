/* This must come before any other includes.  */
#include "defs.h"

#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "sim/callback.h"

#include "sim-main.h"
#include "sim-fpu.h"
#include "sim-signal.h"
#include "sim-syscall.h"

#include "mn10300-sim.h"

#define REG0(X) ((X) & 0x3)
#define REG1(X) (((X) & 0xc) >> 2)
#define REG0_4(X) (((X) & 0x30) >> 4)
#define REG0_8(X) (((X) & 0x300) >> 8)
#define REG1_8(X) (((X) & 0xc00) >> 10)
#define REG0_16(X) (((X) & 0x30000) >> 16)
#define REG1_16(X) (((X) & 0xc0000) >> 18)


INLINE_SIM_MAIN (void)
genericAdd(uint32_t source, uint32_t destReg)
{
  int z, c, n, v;
  uint32_t dest, sum;

  dest = State.regs[destReg];
  sum = source + dest;
  State.regs[destReg] = sum;

  z = (sum == 0);
  n = (sum & 0x80000000);
  c = (sum < source) || (sum < dest);
  v = ((dest & 0x80000000) == (source & 0x80000000)
       && (dest & 0x80000000) != (sum & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}




INLINE_SIM_MAIN (void)
genericSub(uint32_t source, uint32_t destReg)
{
  int z, c, n, v;
  uint32_t dest, difference;

  dest = State.regs[destReg];
  difference = dest - source;
  State.regs[destReg] = difference;

  z = (difference == 0);
  n = (difference & 0x80000000);
  c = (source > dest);
  v = ((dest & 0x80000000) != (source & 0x80000000)
       && (dest & 0x80000000) != (difference & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}

INLINE_SIM_MAIN (void)
genericCmp(uint32_t leftOpnd, uint32_t rightOpnd)
{
  int z, c, n, v;
  uint32_t value;

  value = rightOpnd - leftOpnd;

  z = (value == 0);
  n = (value & 0x80000000);
  c = (leftOpnd > rightOpnd);
  v = ((rightOpnd & 0x80000000) != (leftOpnd & 0x80000000)
       && (rightOpnd & 0x80000000) != (value & 0x80000000));

  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | ( n ? PSW_N : 0)
	  | (c ? PSW_C : 0) | (v ? PSW_V : 0));
}


INLINE_SIM_MAIN (void)
genericOr(uint32_t source, uint32_t destReg)
{
  int n, z;

  State.regs[destReg] |= source;
  z = (State.regs[destReg] == 0);
  n = (State.regs[destReg] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}


INLINE_SIM_MAIN (void)
genericXor(uint32_t source, uint32_t destReg)
{
  int n, z;

  State.regs[destReg] ^= source;
  z = (State.regs[destReg] == 0);
  n = (State.regs[destReg] & 0x80000000) != 0;
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= ((z ? PSW_Z : 0) | (n ? PSW_N : 0));
}


INLINE_SIM_MAIN (void)
genericBtst(uint32_t leftOpnd, uint32_t rightOpnd)
{
  uint32_t temp;
  int z, n;

  temp = rightOpnd;
  temp &= leftOpnd;
  n = (temp & 0x80000000) != 0;
  z = (temp == 0);
  PSW &= ~(PSW_Z | PSW_N | PSW_C | PSW_V);
  PSW |= (z ? PSW_Z : 0) | (n ? PSW_N : 0);
}

/* syscall */
INLINE_SIM_MAIN (void)
do_syscall (SIM_DESC sd)
{
  /* Registers passed to trap 0.  */

  /* Function number.  */
  reg_t func = State.regs[0];
  /* Parameters.  */
  reg_t parm1 = State.regs[1];
  reg_t parm2 = load_word (State.regs[REG_SP] + 12);
  reg_t parm3 = load_word (State.regs[REG_SP] + 16);
  reg_t parm4 = load_word (State.regs[REG_SP] + 20);

  /* We use this for simulated system calls; we may need to change
     it to a reserved instruction if we conflict with uses at
     Matsushita.  */
  int save_errno = errno;	
  errno = 0;

  if (cb_target_to_host_syscall (STATE_CALLBACK (sd), func) == CB_SYS_exit)
    {
      /* EXIT - caller can look in parm1 to work out the reason */
      sim_engine_halt (simulator, STATE_CPU (simulator, 0), NULL, PC,
		       parm1 == 0xdead ? sim_stopped : sim_exited,
		       parm1 == 0xdead ? SIM_SIGABRT : parm1);
    }
  else
    {
      long result, result2;
      int errcode;

      sim_syscall_multi (STATE_CPU (simulator, 0), func, parm1, parm2,
			 parm3, parm4, &result, &result2, &errcode);

      /* Registers set by trap 0.  */
      State.regs[0] = errcode;
      State.regs[1] = result;
    }

  errno = save_errno;
}
