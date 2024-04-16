/* Handle shared libraries for GDB, the GNU Debugger.

   Copyright (C) 2000-2024 Free Software Foundation, Inc.

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

#ifndef SOLIB_SVR4_H
#define SOLIB_SVR4_H

#include "solist.h"

struct objfile;
struct target_so_ops;

extern const struct target_so_ops svr4_so_ops;

/* Link map info to include in an allocated so_list entry.  */

struct lm_info_svr4 final : public lm_info
{
  /* Amount by which addresses in the binary should be relocated to
     match the inferior.  The direct inferior value is L_ADDR_INFERIOR.
     When prelinking is involved and the prelink base address changes,
     we may need a different offset - the recomputed offset is in L_ADDR.
     It is commonly the same value.  It is cached as we want to warn about
     the difference and compute it only once.  L_ADDR is valid
     iff L_ADDR_P.  */
  CORE_ADDR l_addr = 0, l_addr_inferior = 0;
  bool l_addr_p = false;

  /* The target location of lm.  */
  CORE_ADDR lm_addr = 0;

  /* Values read in from inferior's fields of the same name.  */
  CORE_ADDR l_ld = 0, l_next = 0, l_prev = 0, l_name = 0;
};

using lm_info_svr4_up = std::unique_ptr<lm_info_svr4>;

/* Critical offsets and sizes which describe struct r_debug and
   struct link_map on SVR4-like targets.  All offsets and sizes are
   in bytes unless otherwise specified.  */

struct link_map_offsets
  {
    /* Offset and size of r_debug.r_version.  */
    int r_version_offset, r_version_size;

    /* Offset of r_debug.r_map.  */
    int r_map_offset;

    /* Offset of r_debug.r_brk.  */
    int r_brk_offset;

    /* Offset of r_debug.r_ldsomap.  */
    int r_ldsomap_offset;

    /* Offset of r_debug_extended.r_next.  */
    int r_next_offset;

    /* Size of struct link_map (or equivalent), or at least enough of it
       to be able to obtain the fields below.  */
    int link_map_size;

    /* Offset to l_addr field in struct link_map.  */
    int l_addr_offset;

    /* Offset to l_ld field in struct link_map.  */
    int l_ld_offset;

    /* Offset to l_next field in struct link_map.  */
    int l_next_offset;

    /* Offset to l_prev field in struct link_map.  */
    int l_prev_offset;

    /* Offset to l_name field in struct link_map.  */
    int l_name_offset;
  };

/* set_solib_svr4_fetch_link_map_offsets() is intended to be called by
   a <arch>_gdbarch_init() function.  It is used to establish an
   architecture specific link_map_offsets fetcher for the architecture
   being defined.  */

extern void set_solib_svr4_fetch_link_map_offsets
  (struct gdbarch *gdbarch, struct link_map_offsets *(*func) (void));

/* This function is called by thread_db.c.  Return the address of the
   link map for the given objfile.  */
extern CORE_ADDR svr4_fetch_objfile_link_map (struct objfile *objfile);

/* Fetch (and possibly build) an appropriate `struct link_map_offsets'
   for ILP32 and LP64 SVR4 systems.  */
extern struct link_map_offsets *svr4_ilp32_fetch_link_map_offsets (void);
extern struct link_map_offsets *svr4_lp64_fetch_link_map_offsets (void);

/* Return 1 if PC lies in the dynamic symbol resolution code of the
   SVR4 run time loader.  */
int svr4_in_dynsym_resolve_code (CORE_ADDR pc);

#endif /* solib-svr4.h */
