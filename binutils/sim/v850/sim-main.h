#ifndef SIM_MAIN_H
#define SIM_MAIN_H

#include "sim-basics.h"
#include "sim-base.h"

/**
 * TODO: Move these includes to the igen files that need them.
 * This requires extending the igen syntax to support header includes.
 */
#if defined(SEMANTICS_C) || defined(SUPPORT_C)
#include "sim-signal.h"
#endif
#if defined(ENGINE_C) || defined(IDECODE_C) || defined(SEMANTICS_C)
#include "v850-sim.h"
#endif

#endif
