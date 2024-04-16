
/* Main header for the Vitesse IQ2000.  */

#ifndef SIM_MAIN_H
#define SIM_MAIN_H

/* This is a global setting.  Different cpu families can't mix-n-match -scache
   and -pbb.  However some cpu families may use -simple while others use
   one of -scache/-pbb. ???? */
#define WITH_SCACHE_PBB 1

#include "sim-basics.h"
#include "opcodes/iq2000-desc.h"
#include "opcodes/iq2000-opc.h"
#include "arch.h"

/* Pull in IQ2000_{DATA,INSN}_{MASK,VALUE}.  */
#include "elf/iq2000.h"

#include "sim-base.h"
#include "cgen-sim.h"

struct iq2000_sim_cpu {
  /* CPU specific parts go here.
     Note that in files that don't need to access these pieces WANT_CPU_FOO
     won't be defined and thus these parts won't appear.  This is ok in the
     sense that things work.  It is a source of bugs though.
     One has to of course be careful to not take the size of this
     struct and no structure members accessed in non-cpu specific files can
     go after here.  Oh for a better language.  */
#if defined (WANT_CPU_IQ2000BF)
  IQ2000BF_CPU_DATA cpu_data;
#endif
};
#define IQ2000_SIM_CPU(cpu) ((struct iq2000_sim_cpu *) CPU_ARCH_DATA (cpu))

/* Misc.  */

/* Catch address exceptions.  */
extern SIM_CORE_SIGNAL_FN iq2000_core_signal ATTRIBUTE_NORETURN;
#define SIM_CORE_SIGNAL(SD,CPU,CIA,MAP,NR_BYTES,ADDR,TRANSFER,ERROR) \
iq2000_core_signal ((SD), (CPU), (CIA), (MAP), (NR_BYTES), (ADDR), \
		  (TRANSFER), (ERROR))

/* Convert between CPU-internal addresses and sim_core addresses.  */
#define CPU2DATA(addr) (IQ2000_DATA_VALUE + (addr))
#define DATA2CPU(addr) ((addr) - IQ2000_DATA_VALUE)
#define CPU2INSN(addr) (IQ2000_INSN_VALUE + ((addr) & ~IQ2000_INSN_MASK))
#define INSN2CPU(addr) ((addr) - IQ2000_INSN_VALUE)
#define IQ2000_INSN_MEM_SIZE (CPU2INSN(0x800000) - CPU2INSN(0x0000))
#define IQ2000_DATA_MEM_SIZE (CPU2DATA(0x800000) - CPU2DATA(0x0000))

PCADDR get_h_pc (SIM_CPU *);
void set_h_pc (SIM_CPU *, PCADDR);
void do_syscall (SIM_CPU *, PCADDR);
void do_break (SIM_CPU *, PCADDR);

#endif /* SIM_MAIN_H */
