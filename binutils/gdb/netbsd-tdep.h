/* Common target-dependent definitions for NetBSD systems.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.
   Contributed by Wasabi Systems, Inc.

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

#ifndef NBSD_TDEP_H
#define NBSD_TDEP_H

int nbsd_pc_in_sigtramp (CORE_ADDR, const char *);

/* NetBSD specific set of ABI-related routines.  */

void nbsd_init_abi (struct gdbarch_info, struct gdbarch *);

/* Output the header for "info proc mappings".  ADDR_BIT is the size
   of a virtual address in bits.  */

extern void nbsd_info_proc_mappings_header (int addr_bit);

/* Output description of a single memory range for "info proc
   mappings".  ADDR_BIT is the size of a virtual address in bits.  The
   KVE_START, KVE_END, KVE_OFFSET, KVE_FLAGS, and KVE_PROTECTION
   parameters should contain the value of the corresponding fields in
   a 'struct kinfo_vmentry'.  The KVE_PATH parameter should contain a
   pointer to the 'kve_path' field in a 'struct kinfo_vmentry'. */

extern void nbsd_info_proc_mappings_entry (int addr_bit, ULONGEST kve_start,
					   ULONGEST kve_end,
					   ULONGEST kve_offset,
					   int kve_flags, int kve_protection,
					   const char *kve_path);

#endif /* NBSD_TDEP_H */
