/* OS ABI variant handling for GDB.
   Copyright (C) 2001-2024 Free Software Foundation, Inc.
   
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

#ifndef OSABI_H
#define OSABI_H

/* * List of known OS ABIs.  If you change this, make sure to update the
   table in osabi.c.  */
enum gdb_osabi
{
  GDB_OSABI_UNKNOWN = 0,	/* keep this zero */
  GDB_OSABI_NONE,

  GDB_OSABI_SVR4,
  GDB_OSABI_HURD,
  GDB_OSABI_SOLARIS,
  GDB_OSABI_LINUX,
  GDB_OSABI_FREEBSD,
  GDB_OSABI_NETBSD,
  GDB_OSABI_OPENBSD,
  GDB_OSABI_WINCE,
  GDB_OSABI_GO32,
  GDB_OSABI_QNXNTO,
  GDB_OSABI_CYGWIN,
  GDB_OSABI_WINDOWS,
  GDB_OSABI_AIX,
  GDB_OSABI_DICOS,
  GDB_OSABI_DARWIN,
  GDB_OSABI_OPENVMS,
  GDB_OSABI_LYNXOS178,
  GDB_OSABI_NEWLIB,
  GDB_OSABI_SDE,
  GDB_OSABI_PIKEOS,

  GDB_OSABI_INVALID		/* keep this last */
};

/* Register an OS ABI sniffer.  Each arch/flavour may have more than
   one sniffer.  This is used to e.g. differentiate one OS's a.out from
   another.  The first sniffer to return something other than
   GDB_OSABI_UNKNOWN wins, so a sniffer should be careful to claim a file
   only if it knows for sure what it is.  */
void gdbarch_register_osabi_sniffer (enum bfd_architecture,
				     enum bfd_flavour,
				     enum gdb_osabi (*)(bfd *));

/* Register a handler for an OS ABI variant for a given architecture
   and machine type.  There should be only one handler for a given OS
   ABI for each architecture and machine type combination.  */
void gdbarch_register_osabi (enum bfd_architecture, unsigned long,
			     enum gdb_osabi,
			     void (*)(struct gdbarch_info,
				      struct gdbarch *));

/* Lookup the OS ABI corresponding to the specified BFD.  */
enum gdb_osabi gdbarch_lookup_osabi (bfd *);

/* Lookup the OS ABI corresponding to the specified target description
   string.  */
enum gdb_osabi osabi_from_tdesc_string (const char *text);

/* Return true if there's an OS ABI handler for INFO.  */
bool has_gdb_osabi_handler (struct gdbarch_info info);

/* Initialize the gdbarch for the specified OS ABI variant.  */
void gdbarch_init_osabi (struct gdbarch_info, struct gdbarch *);

/* Return the name of the specified OS ABI.  */
const char *gdbarch_osabi_name (enum gdb_osabi);

/* Return a regular expression that matches the OS part of a GNU
   configury triplet for the given OSABI.  */
const char *osabi_triplet_regexp (enum gdb_osabi osabi);

/* Helper routine for ELF file sniffers.  This looks at ABI tag note
   sections to determine the OS ABI from the note.  */
void generic_elf_osabi_sniff_abi_tag_sections (bfd *, asection *,
					       enum gdb_osabi *);

#endif /* OSABI_H */
