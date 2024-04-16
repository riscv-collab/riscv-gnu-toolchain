/* Main header for the m32r.  */

#ifndef SIM_MAIN_H
#define SIM_MAIN_H

/* This is a global setting.  Different cpu families can't mix-n-match -scache
   and -pbb.  However some cpu families may use -simple while others use
   one of -scache/-pbb.  */
#define WITH_SCACHE_PBB 1

#include "sim-basics.h"
#include "opcodes/m32r-desc.h"
#include "opcodes/m32r-opc.h"
#include "arch.h"
#include "sim-base.h"
#include "cgen-sim.h"

/* TODO: Move this to the CGEN generated files instead.  */
#include "m32r-sim.h"

/* Misc.  */

/* Catch address exceptions.  */
extern SIM_CORE_SIGNAL_FN m32r_core_signal ATTRIBUTE_NORETURN;
#define SIM_CORE_SIGNAL(SD,CPU,CIA,MAP,NR_BYTES,ADDR,TRANSFER,ERROR) \
m32r_core_signal ((SD), (CPU), (CIA), (MAP), (NR_BYTES), (ADDR), \
		  (TRANSFER), (ERROR))

#endif /* SIM_MAIN_H */
