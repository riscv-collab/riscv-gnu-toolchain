/* nto-tdep.h - QNX Neutrino target header.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

   Contributed by QNX Software Systems Ltd.

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

#ifndef NTO_TDEP_H
#define NTO_TDEP_H

#include "solist.h"
#include "osabi.h"
#include "regset.h"
#include "gdbthread.h"
#include "gdbsupport/gdb-checked-static-cast.h"

/* Target operations defined for Neutrino targets (<target>-nto-tdep.c).  */

struct nto_target_ops
{
/* The CPUINFO flags from the remote.  Currently used by
   i386 for fxsave but future proofing other hosts.
   This is initialized in procfs_attach or nto_start_remote
   depending on our host/target.  It would only be invalid
   if we were talking to an older pdebug which didn't support
   the cpuinfo message.  */
  unsigned cpuinfo_flags;

/* True if successfully retrieved cpuinfo from remote.  */
  int cpuinfo_valid;

/* Given a register, return an id that represents the Neutrino
   regset it came from.  If reg == -1 update all regsets.  */
  int (*regset_id) (int);

  void (*supply_gregset) (struct regcache *, char *);

  void (*supply_fpregset) (struct regcache *, char *);

  void (*supply_altregset) (struct regcache *, char *);

/* Given a regset, tell gdb about registers stored in data.  */
  void (*supply_regset) (struct regcache *, int, char *);

/* Given a register and regset, calculate the offset into the regset
   and stuff it into the last argument.  If regno is -1, calculate the
   size of the entire regset.  Returns length of data, -1 if unknown
   regset, 0 if unknown register.  */
  int (*register_area) (struct gdbarch *, int, int, unsigned *);

/* Build the Neutrino register set info into the data buffer.
   Return -1 if unknown regset, 0 otherwise.  */
  int (*regset_fill) (const struct regcache *, int, char *);

/* Gives the fetch_link_map_offsets function exposure outside of
   solib-svr4.c so that we can override relocate_section_addresses().  */
  struct link_map_offsets *(*fetch_link_map_offsets) (void);

/* Used by nto_elf_osabi_sniffer to determine if we're connected to an
   Neutrino target.  */
  enum gdb_osabi (*is_nto_target) (bfd *abfd);
};

extern struct nto_target_ops current_nto_target;

#define nto_cpuinfo_flags (current_nto_target.cpuinfo_flags)

#define nto_cpuinfo_valid (current_nto_target.cpuinfo_valid)

#define nto_regset_id (current_nto_target.regset_id)

#define nto_supply_gregset (current_nto_target.supply_gregset)

#define nto_supply_fpregset (current_nto_target.supply_fpregset)

#define nto_supply_altregset (current_nto_target.supply_altregset)

#define nto_supply_regset (current_nto_target.supply_regset)

#define nto_register_area (current_nto_target.register_area)

#define nto_regset_fill (current_nto_target.regset_fill)

#define nto_fetch_link_map_offsets \
(current_nto_target.fetch_link_map_offsets)

#define nto_is_nto_target (current_nto_target.is_nto_target)

/* Keep this consistant with neutrino syspage.h.  */
enum
{
  CPUTYPE_X86,
  CPUTYPE_PPC,
  CPUTYPE_MIPS,
  CPUTYPE_SPARE,
  CPUTYPE_ARM,
  CPUTYPE_SH,
  CPUTYPE_UNKNOWN
};

enum
{
  OSTYPE_QNX4,
  OSTYPE_NTO
};

/* These correspond to the DSMSG_* versions in dsmsgs.h.  */
enum
{
  NTO_REG_GENERAL,
  NTO_REG_FLOAT,
  NTO_REG_SYSTEM,
  NTO_REG_ALT,
  NTO_REG_END
};

typedef char qnx_reg64[8];

typedef struct _debug_regs
{
  qnx_reg64 padding[1024];
} nto_regset_t;

struct nto_thread_info : public private_thread_info
{
  short tid = 0;
  unsigned char state = 0;
  unsigned char flags = 0;
  std::string name;
};

static inline nto_thread_info *
get_nto_thread_info (thread_info *thread)
{
  return gdb::checked_static_cast<nto_thread_info *> (thread->priv.get ());
}

/* Per-inferior data, common for both procfs and remote.  */
struct nto_inferior_data
{
  /* Last stopped flags result from wait function */
  unsigned int stopped_flags = 0;

  /* Last known stopped PC */
  CORE_ADDR stopped_pc = 0;
};

/* Generic functions in nto-tdep.c.  */

void nto_init_solib_absolute_prefix (void);

char **nto_parse_redirection (char *start_argv[], const char **in,
			      const char **out, const char **err);

void nto_relocate_section_addresses (shobj &, target_section *);

int nto_map_arch_to_cputype (const char *);

int nto_find_and_open_solib (const char *, unsigned,
			     gdb::unique_xmalloc_ptr<char> *);

enum gdb_osabi nto_elf_osabi_sniffer (bfd *abfd);

void nto_initialize_signals (void);

/* Dummy function for initializing nto_target_ops on targets which do
   not define a particular regset.  */
void nto_dummy_supply_regset (struct regcache *regcache, char *regs);

int nto_in_dynsym_resolve_code (CORE_ADDR pc);

const char *nto_extra_thread_info (struct target_ops *self, struct thread_info *);

LONGEST nto_read_auxv_from_initial_stack (CORE_ADDR initial_stack,
					  gdb_byte *readbuf,
					  LONGEST len, size_t sizeof_auxv_t);

struct nto_inferior_data *nto_inferior_data (struct inferior *inf);

#endif /* NTO_TDEP_H */
