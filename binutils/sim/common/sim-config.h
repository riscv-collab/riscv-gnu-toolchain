/* The common simulator framework for GDB, the GNU Debugger.

   Copyright 2002-2024 Free Software Foundation, Inc.

   Contributed by Andrew Cagney and Red Hat.

   This file is part of GDB.

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


#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

#ifdef SIM_COMMON_BUILD
#error "This header is unusable in common builds due to reliance on SIM_AC_OPTION_BITSIZE"
#endif

/* Host dependant:

   The CPP below defines information about the compilation host.  In
   particular it defines the macro's:

   HOST_BYTE_ORDER	The byte order of the host. Could be BFD_ENDIAN_LITTLE
			or BFD_ENDIAN_BIG.

 */

#ifdef WORDS_BIGENDIAN
# define HOST_BYTE_ORDER BFD_ENDIAN_BIG
#else
# define HOST_BYTE_ORDER BFD_ENDIAN_LITTLE
#endif


/* Until devices and tree properties are sorted out, tell sim-config.c
   not to call the tree_find_foo fns.  */
#define WITH_TREE_PROPERTIES 0


/* Endianness of the target.

   Possible values are BFD_ENDIAN_UNKNOWN, BFD_ENDIAN_LITTLE, or BFD_ENDIAN_BIG.  */

extern enum bfd_endian current_target_byte_order;
#define CURRENT_TARGET_BYTE_ORDER \
  (WITH_TARGET_BYTE_ORDER != BFD_ENDIAN_UNKNOWN \
   ? WITH_TARGET_BYTE_ORDER : current_target_byte_order)



/* XOR endian.

   In addition to the above, the simulator can support the horrible
   XOR endian mode (as found in the PowerPC and MIPS ISA).  See
   sim-core for more information.

   If WITH_XOR_ENDIAN is non-zero, it specifies the number of bytes
   potentially involved in the XOR munge. A typical value is 8. */

#ifndef WITH_XOR_ENDIAN
#define WITH_XOR_ENDIAN		0
#endif



/* SMP support:

   Sets a limit on the number of processors that can be simulated.  If
   WITH_SMP is set to zero (0), the simulator is restricted to
   suporting only one processor (and as a consequence leaves the SMP
   code out of the build process).

   The actual number of processors is taken from the device
   /options/smp@<nr-cpu> */

#if defined (WITH_SMP) && (WITH_SMP > 0)
#define MAX_NR_PROCESSORS		WITH_SMP
#endif

#ifndef MAX_NR_PROCESSORS
#define MAX_NR_PROCESSORS		1
#endif


/* Size of target word, address and OpenFirmware Cell:

   The target word size is determined by the natural size of its
   reginsters.

   On most hosts, the address and cell are the same size as a target
   word.  */

#ifndef WITH_TARGET_WORD_BITSIZE
#define WITH_TARGET_WORD_BITSIZE        32
#endif

#ifndef WITH_TARGET_ADDRESS_BITSIZE
#define WITH_TARGET_ADDRESS_BITSIZE	WITH_TARGET_WORD_BITSIZE
#endif

#ifndef WITH_TARGET_CELL_BITSIZE
#define WITH_TARGET_CELL_BITSIZE	WITH_TARGET_WORD_BITSIZE
#endif

#ifndef WITH_TARGET_FLOATING_POINT_BITSIZE
#define WITH_TARGET_FLOATING_POINT_BITSIZE 64
#endif



/* Most significant bit of target:

   Set this according to your target's bit numbering convention.  For
   the PowerPC it is zero, for many other targets it is 31 or 63.

   For targets that can both have either 32 or 64 bit words and number
   MSB as 31, 63.  Define this to be (WITH_TARGET_WORD_BITSIZE - 1) */

#ifndef WITH_TARGET_WORD_MSB
#define WITH_TARGET_WORD_MSB            0
#endif



/* Program environment:

   Three environments are available - UEA (user), VEA (virtual) and
   OEA (perating).  The former two are environment that users would
   expect to see (VEA includes things like coherency and the time
   base) while OEA is what an operating system expects to see.  By
   setting these to specific values, the build process is able to
   eliminate non relevent environment code.

   STATE_ENVIRONMENT(sd) specifies which of vea or oea is required for
   the current runtime.

   ALL_ENVIRONMENT is used during configuration as a value for
   WITH_ENVIRONMENT to indicate the choice is runtime selectable.
   The default is then USER_ENVIRONMENT [since allowing the user to choose
   the default at configure time seems like featuritis and since people using
   OPERATING_ENVIRONMENT have more to worry about than selecting the
   default].
   ALL_ENVIRONMENT is also used to set STATE_ENVIRONMENT to the
   "uninitialized" state.  */

enum sim_environment {
  ALL_ENVIRONMENT,
  USER_ENVIRONMENT,
  VIRTUAL_ENVIRONMENT,
  OPERATING_ENVIRONMENT
};

/* To be prepended to simulator calls with absolute file paths and
   chdir:ed at startup.  */
extern char *simulator_sysroot;

/* Callback & Modulo Memory.

   Core includes a builtin memory type (raw_memory) that is
   implemented using an array.  raw_memory does not require any
   additional functions etc.

   Callback memory is where the core calls a core device for the data
   it requires.  Callback memory can be layered using priorities.

   Modulo memory is a variation on raw_memory where ADDRESS & (MODULO
   - 1) is used as the index into the memory array.

   The OEA model uses callback memory for devices.

   The VEA model uses callback memory to capture `page faults'.

   BTW, while raw_memory could have been implemented as a callback,
   profiling has shown that there is a biger win (at least for the
   x86) in eliminating a function call for the most common
   (raw_memory) case. */


/* Alignment:

   A processor architecture may or may not handle misaligned
   transfers.

   As alternatives: both little and big endian modes take an exception
   (STRICT_ALIGNMENT); big and little endian models handle misaligned
   transfers (NONSTRICT_ALIGNMENT); or the address is forced into
   alignment using a mask (FORCED_ALIGNMENT).

   Mixed alignment should be specified when the simulator needs to be
   able to change the alignment requirements on the fly (eg for
   bi-endian support). */

enum sim_alignments {
  MIXED_ALIGNMENT,
  NONSTRICT_ALIGNMENT,
  STRICT_ALIGNMENT,
  FORCED_ALIGNMENT,
};

extern enum sim_alignments current_alignment;

#if !defined (WITH_ALIGNMENT)
#define WITH_ALIGNMENT 0
#endif

#define CURRENT_ALIGNMENT (WITH_ALIGNMENT \
			   ? WITH_ALIGNMENT \
			   : current_alignment)



/* Floating point suport:

   Should the processor trap for all floating point instructions (as
   if the hardware wasn't implemented) or implement the floating point
   instructions directly. */

#if defined (WITH_FLOATING_POINT)

#define SOFT_FLOATING_POINT		1
#define HARD_FLOATING_POINT		2

extern int current_floating_point;
#define CURRENT_FLOATING_POINT (WITH_FLOATING_POINT \
				? WITH_FLOATING_POINT \
				: current_floating_point)

#endif


/* Whether to check instructions for reserved bits being set */

/* #define WITH_RESERVED_BITS		1 */



/* include monitoring code */

#define MONITOR_INSTRUCTION_ISSUE	1
#define MONITOR_LOAD_STORE_UNIT		2
/* do not define WITH_MON by default */
#define DEFAULT_WITH_MON		(MONITOR_LOAD_STORE_UNIT \
					 | MONITOR_INSTRUCTION_ISSUE)


/* Whether or not input/output just uses stdio, or uses printf_filtered for
   output, and polling input for input.  */

#define DONT_USE_STDIO			2
#define DO_USE_STDIO			1

extern int current_stdio;
#define CURRENT_STDIO (WITH_STDIO	\
		       ? WITH_STDIO     \
		       : current_stdio)



/* Set the default state configuration, before parsing argv.  */

extern void sim_config_default (SIM_DESC sd);

/* Complete and verify the simulator configuration.  */

extern SIM_RC sim_config (SIM_DESC sd);

/* Print the simulator configuration.  */

extern void sim_config_print (SIM_DESC sd);


#endif
