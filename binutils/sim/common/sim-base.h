/* Simulator pseudo baseclass.

   Copyright 1997-2024 Free Software Foundation, Inc.

   Contributed by Cygnus Support.

This file is part of GDB, the GNU debugger.

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


/* Simulator state pseudo baseclass.

   Each simulator is required to have the file ``sim-main.h''.  That
   file includes ``sim-basics.h'', defines the base type ``sim_cia''
   (the data type that contains complete current instruction address
   information), include ``sim-base.h'':

     #include "sim-basics.h"
     /-* If `sim_cia' is not an integral value (e.g. a struct), define
         CIA_ADDR to return the integral value.  *-/
     /-* typedef struct {...} sim_cia; *-/
     /-* #define CIA_ADDR(cia) (...) *-/
     #include "sim-base.h"

   finally, two data types `struct _sim_cpu' and `struct sim_state'
   are defined:

     struct _sim_cpu {
        ... simulator specific members ...
        sim_cpu_base base;
     };

   If your sim needs to allocate sim-wide state, use STATE_ARCH_DATA.  */


#ifndef SIM_BASE_H
#define SIM_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Pre-declare certain types. */

/* typedef <target-dependant> sim_cia; */
#ifndef NULL_CIA
#define NULL_CIA ((sim_cia) 0)
#endif
/* Return the current instruction address as a number.
   Some targets treat the current instruction address as a struct
   (e.g. for delay slot handling).  */
#ifndef CIA_ADDR
#define CIA_ADDR(cia) (cia)
typedef address_word sim_cia;
#endif
#ifndef INVALID_INSTRUCTION_ADDRESS
#define INVALID_INSTRUCTION_ADDRESS ((address_word)0 - 1)
#endif

/* TODO: Probably should just delete SIM_CPU.  */
typedef struct _sim_cpu SIM_CPU;
typedef struct _sim_cpu sim_cpu;

#include "bfd.h"

#include "sim-module.h"

#include "sim-arange.h"
#include "sim-trace.h"
#include "sim-core.h"
#include "sim-events.h"
#include "sim-profile.h"
#include "sim-model.h"
#include "sim-io.h"
#include "sim-engine.h"
#include "sim-watch.h"
#include "sim-memopt.h"
#include "sim-cpu.h"
#include "sim-assert.h"

struct sim_state {
  /* All the cpus for this instance.  */
  sim_cpu *cpu[MAX_NR_PROCESSORS];
#if (WITH_SMP)
# define STATE_CPU(sd, n) ((sd)->cpu[n])
#else
# define STATE_CPU(sd, n) ((sd)->cpu[0])
#endif

  /* Simulator's argv[0].  */
  const char *my_name;
#define STATE_MY_NAME(sd) ((sd)->my_name)

  /* Who opened the simulator.  */
  SIM_OPEN_KIND open_kind;
#define STATE_OPEN_KIND(sd) ((sd)->open_kind)

  /* The host callbacks.  */
  struct host_callback_struct *callback;
#define STATE_CALLBACK(sd) ((sd)->callback)

  /* The type of simulation environment (user/operating).  */
  enum sim_environment environment;
#define STATE_ENVIRONMENT(sd) ((sd)->environment)

#if 0 /* FIXME: Not ready yet.  */
  /* Stuff defined in sim-config.h.  */
  struct sim_config config;
#define STATE_CONFIG(sd) ((sd)->config)
#endif

  /* List of installed module `init' handlers.  */
  struct module_list *modules;
#define STATE_MODULES(sd) ((sd)->modules)

  /* Supported options.  */
  struct option_list *options;
#define STATE_OPTIONS(sd) ((sd)->options)

  /* Non-zero if -v specified.  */
  int verbose_p;
#define STATE_VERBOSE_P(sd) ((sd)->verbose_p)

  /* Non cpu-specific trace data.  See sim-trace.h.  */
  TRACE_DATA trace_data;
#define STATE_TRACE_DATA(sd) (& (sd)->trace_data)

  /* If non NULL, the BFD architecture specified on the command line */
  const struct bfd_arch_info *architecture;
#define STATE_ARCHITECTURE(sd) ((sd)->architecture)

  /* If non NULL, the bfd target specified on the command line */
  const char *target;
#define STATE_TARGET(sd) ((sd)->target)

  /* List of machs available.  */
  const SIM_MACH * const *machs;
#define STATE_MACHS(sd) ((sd)->machs)

  /* If non-NULL, the model to select for CPUs.  */
  const char *model_name;
#define STATE_MODEL_NAME(sd) ((sd)->model_name)

  /* In standalone simulator, this is the program to run.  Not to be confused
     with argv which are the strings passed to the program itself.  */
  char *prog_file;
#define STATE_PROG_FILE(sd) ((sd)->prog_file)

  /* In standalone simulator, this is the program's arguments passed
     on the command line.  */
  char **prog_argv;
#define STATE_PROG_ARGV(sd) ((sd)->prog_argv)

  /* Thie is the program's argv[0] override.  */
  char *prog_argv0;
#define STATE_PROG_ARGV0(sd) ((sd)->prog_argv0)

  /* The program's environment.  */
  char **prog_envp;
#define STATE_PROG_ENVP(sd) ((sd)->prog_envp)

  /* The program's bfd.  */
  struct bfd *prog_bfd;
#define STATE_PROG_BFD(sd) ((sd)->prog_bfd)

  /* Symbol table for prog_bfd */
  struct bfd_symbol **prog_syms;
#define STATE_PROG_SYMS(sd) ((sd)->prog_syms)

  /* Number of prog_syms symbols.  */
  long prog_syms_count;
#define STATE_PROG_SYMS_COUNT(sd) ((sd)->prog_syms_count)

  /* The program's text section.  */
  struct bfd_section *text_section;
  /* Starting and ending text section addresses from the bfd.  */
  bfd_vma text_start, text_end;
#define STATE_TEXT_SECTION(sd) ((sd)->text_section)
#define STATE_TEXT_START(sd) ((sd)->text_start)
#define STATE_TEXT_END(sd) ((sd)->text_end)

  /* Start address, set when the program is loaded from the bfd.  */
  bfd_vma start_addr;
#define STATE_START_ADDR(sd) ((sd)->start_addr)

  /* Size of the simulator's cache, if any.
     This is not the target's cache.  It is the cache the simulator uses
     to process instructions.  */
  unsigned int scache_size;
#define STATE_SCACHE_SIZE(sd) ((sd)->scache_size)

  /* core memory bus */
#define STATE_CORE(sd) (&(sd)->core)
  sim_core core;

  /* Record of memory sections added via the memory-options interface.  */
#define STATE_MEMOPT(sd) ((sd)->memopt)
  sim_memopt *memopt;

  /* event handler */
#define STATE_EVENTS(sd) (&(sd)->events)
  sim_events events;

  /* generic halt/resume engine */
  sim_engine engine;
#define STATE_ENGINE(sd) (&(sd)->engine)

  /* generic watchpoint support */
  sim_watchpoints watchpoints;
#define STATE_WATCHPOINTS(sd) (&(sd)->watchpoints)

#if WITH_HW
  struct sim_hw *hw;
#define STATE_HW(sd) ((sd)->hw)
#endif

  /* Should image loads be performed using the LMA or VMA?  Older
     simulators use the VMA while newer simulators prefer the LMA. */
  int load_at_lma_p;
#define STATE_LOAD_AT_LMA_P(SD) ((SD)->load_at_lma_p)

  /* Pointer for sim target to store arbitrary state data.  Normally the
     target should define a struct and use it here.  */
  void *arch_data;
#define STATE_ARCH_DATA(sd) ((sd)->arch_data)

  /* Marker for those wanting to do sanity checks.
     This should remain the last member of this struct to help catch
     miscompilation errors.  */
  int magic;
#define SIM_MAGIC_NUMBER 0x4242
#define STATE_MAGIC(sd) ((sd)->magic)
};

/* Functions for allocating/freeing a sim_state.  */
SIM_DESC sim_state_alloc_extra (SIM_OPEN_KIND kind, host_callback *callback,
				size_t extra_bytes);
#define sim_state_alloc(kind, callback) sim_state_alloc_extra(kind, callback, 0)

void sim_state_free (SIM_DESC);

#ifdef __cplusplus
}
#endif

#endif /* SIM_BASE_H */
