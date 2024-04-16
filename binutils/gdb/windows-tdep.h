/* Copyright (C) 2008-2024 Free Software Foundation, Inc.

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

#ifndef WINDOWS_TDEP_H
#define WINDOWS_TDEP_H

struct gdbarch;

extern struct cmd_list_element *info_w32_cmdlist;

extern void init_w32_command_list (void);

extern void windows_xfer_shared_library (const char* so_name,
					 CORE_ADDR load_addr,
					 CORE_ADDR *text_offset_cached,
					 struct gdbarch *gdbarch,
					 std::string &xml);

extern ULONGEST windows_core_xfer_shared_libraries (struct gdbarch *gdbarch,
						    gdb_byte *readbuf,
						    ULONGEST offset,
						    ULONGEST len);

extern std::string windows_core_pid_to_str (struct gdbarch *gdbarch,
					    ptid_t ptid);

/* To be called from the various GDB_OSABI_WINDOWS handlers for the
   various Windows architectures and machine types.  */

extern void windows_init_abi (struct gdbarch_info info,
			      struct gdbarch *gdbarch);

/* To be called from the various GDB_OSABI_CYGWIN handlers for the
   various Windows architectures and machine types.  */

extern void cygwin_init_abi (struct gdbarch_info info,
			     struct gdbarch *gdbarch);

/* Return true if the Portable Executable behind ABFD uses the Cygwin dll
   (cygwin1.dll).  */

extern bool is_linked_with_cygwin_dll (bfd *abfd);

#endif
