/* Target-dependent code for GNU/Linux, architecture independent.

   Copyright (C) 2009-2024 Free Software Foundation, Inc.

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

#ifndef LINUX_TDEP_H
#define LINUX_TDEP_H

#include "bfd.h"
#include "displaced-stepping.h"

struct inferior;
struct regcache;

/* Enum used to define the extra fields of the siginfo type used by an
   architecture.  */
enum linux_siginfo_extra_field_values
{
  /* Add bound fields into the segmentation fault field.  */
  LINUX_SIGINFO_FIELD_ADDR_BND = 1
};

/* Defines a type for the values defined in linux_siginfo_extra_field_values.  */
DEF_ENUM_FLAGS_TYPE (enum linux_siginfo_extra_field_values,
		     linux_siginfo_extra_fields);

/* This function is suitable for architectures that
   extend/override the standard siginfo in a specific way.  */
struct type *linux_get_siginfo_type_with_fields (struct gdbarch *gdbarch,
						 linux_siginfo_extra_fields);

/* Return true if ADDRESS is within the boundaries of a page mapped with
   memory tagging protection.  */
bool linux_address_in_memtag_page (CORE_ADDR address);

typedef char *(*linux_collect_thread_registers_ftype) (const struct regcache *,
						       ptid_t,
						       bfd *, char *, int *,
						       enum gdb_signal);

extern enum gdb_signal linux_gdb_signal_from_target (struct gdbarch *gdbarch,
						     int signal);

extern int linux_gdb_signal_to_target (struct gdbarch *gdbarch,
				       enum gdb_signal signal);

/* Default GNU/Linux implementation of `displaced_step_location', as
   defined in gdbarch.h.  Determines the entry point from AT_ENTRY in
   the target auxiliary vector.  */
extern CORE_ADDR linux_displaced_step_location (struct gdbarch *gdbarch);


/* Implementation of gdbarch_displaced_step_prepare.  */

extern displaced_step_prepare_status linux_displaced_step_prepare
  (gdbarch *arch, thread_info *thread, CORE_ADDR &displaced_pc);

/* Implementation of gdbarch_displaced_step_finish.  */

extern displaced_step_finish_status linux_displaced_step_finish
  (gdbarch *arch, thread_info *thread, const target_waitstatus &status);

/* Implementation of gdbarch_displaced_step_copy_insn_closure_by_addr.  */

extern const displaced_step_copy_insn_closure *
  linux_displaced_step_copy_insn_closure_by_addr
    (inferior *inf, CORE_ADDR addr);

/* Implementation of gdbarch_displaced_step_restore_all_in_ptid.  */

extern void linux_displaced_step_restore_all_in_ptid (inferior *parent_inf,
						      ptid_t ptid);

extern void linux_init_abi (struct gdbarch_info info, struct gdbarch *gdbarch,
			    int num_disp_step_buffers);

extern int linux_is_uclinux (void);

/* Fetch the AT_HWCAP entry from auxv data AUXV.  Use TARGET and GDBARCH to
   parse auxv entries.

   On error, 0 is returned.  */
extern CORE_ADDR linux_get_hwcap (const std::optional<gdb::byte_vector> &auxv,
				  struct target_ops *target, gdbarch *gdbarch);

/* Same as the above, but obtain all the inputs from the current inferior.  */

extern CORE_ADDR linux_get_hwcap ();

/* Fetch the AT_HWCAP2 entry from auxv data AUXV.  Use TARGET and GDBARCH to
   parse auxv entries.

   On error, 0 is returned.  */
extern CORE_ADDR linux_get_hwcap2 (const std::optional<gdb::byte_vector> &auxv,
				   struct target_ops *target, gdbarch *gdbarch);

/* Same as the above, but obtain all the inputs from the current inferior.  */

extern CORE_ADDR linux_get_hwcap2 ();

/* Fetch (and possibly build) an appropriate `struct link_map_offsets'
   for ILP32 and LP64 Linux systems.  */
extern struct link_map_offsets *linux_ilp32_fetch_link_map_offsets ();
extern struct link_map_offsets *linux_lp64_fetch_link_map_offsets ();

#endif /* linux-tdep.h */
