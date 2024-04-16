/* Dynamic architecture support for GDB, the GNU debugger.

   Copyright (C) 1998-2024 Free Software Foundation, Inc.

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

#ifndef ARCH_UTILS_H
#define ARCH_UTILS_H

#include "gdbarch.h"

class frame_info_ptr;
struct minimal_symbol;
struct type;
struct gdbarch_info;
struct dwarf2_frame_state;

template <size_t bp_size, const gdb_byte *break_insn>
struct bp_manipulation
{
  static int
  kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
  {
    return bp_size;
  }

  static const gdb_byte *
  bp_from_kind (struct gdbarch *gdbarch, int kind, int *size)
  {
    *size = kind;
    return break_insn;
  }
};

template <size_t bp_size,
	  const gdb_byte *break_insn_little,
	  const gdb_byte *break_insn_big>
struct bp_manipulation_endian
{
  static int
  kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr)
  {
    return bp_size;
  }

  static const gdb_byte *
  bp_from_kind (struct gdbarch *gdbarch, int kind, int *size)
  {
    *size = kind;
    if (gdbarch_byte_order (gdbarch) == BFD_ENDIAN_BIG)
      return break_insn_big;
    else
      return break_insn_little;
  }
};

#define BP_MANIPULATION(BREAK_INSN) \
  bp_manipulation<sizeof (BREAK_INSN), BREAK_INSN>

#define BP_MANIPULATION_ENDIAN(BREAK_INSN_LITTLE, BREAK_INSN_BIG) \
  bp_manipulation_endian<sizeof (BREAK_INSN_LITTLE),		  \
  BREAK_INSN_LITTLE, BREAK_INSN_BIG>

/* Default implementation of gdbarch_displaced_hw_singlestep.  */
extern bool default_displaced_step_hw_singlestep (struct gdbarch *);

/* Possible value for gdbarch_displaced_step_location:
   Place displaced instructions at the program's entry point,
   leaving space for inferior function call return breakpoints.  */
extern CORE_ADDR displaced_step_at_entry_point (struct gdbarch *gdbarch);

/* The only possible cases for inner_than.  */
extern int core_addr_lessthan (CORE_ADDR lhs, CORE_ADDR rhs);
extern int core_addr_greaterthan (CORE_ADDR lhs, CORE_ADDR rhs);

/* Identity functions on a CORE_ADDR.  Just return the "addr".  */

extern CORE_ADDR core_addr_identity (struct gdbarch *gdbarch, CORE_ADDR addr);
extern gdbarch_convert_from_func_ptr_addr_ftype convert_from_func_ptr_addr_identity;

/* No-op conversion of reg to regnum.  */

extern int no_op_reg_to_regnum (struct gdbarch *gdbarch, int reg);

/* Do nothing version of coff_make_msymbol_special.  */

void default_coff_make_msymbol_special (int val, struct minimal_symbol *msym);

/* Do nothing default implementation of gdbarch_make_symbol_special.  */

void default_make_symbol_special (struct symbol *sym, struct objfile *objfile);

/* Do nothing default implementation of gdbarch_adjust_dwarf2_addr.  */

CORE_ADDR default_adjust_dwarf2_addr (CORE_ADDR pc);

/* Do nothing default implementation of gdbarch_adjust_dwarf2_line.  */

CORE_ADDR default_adjust_dwarf2_line (CORE_ADDR addr, int rel);

/* Default DWARF vendor CFI handler.  */

bool default_execute_dwarf_cfa_vendor_op (struct gdbarch *gdbarch, gdb_byte op,
					  struct dwarf2_frame_state *fs);

/* Version of cannot_fetch_register() / cannot_store_register() that
   always fails.  */

int cannot_register_not (struct gdbarch *gdbarch, int regnum);

/* Legacy version of target_virtual_frame_pointer().  Assumes that
   there is an gdbarch_deprecated_fp_regnum and that it is the same, cooked or
   raw.  */

extern gdbarch_virtual_frame_pointer_ftype legacy_virtual_frame_pointer;

/* Default implementation of gdbarch_floatformat_for_type.  */
extern const struct floatformat **
  default_floatformat_for_type (struct gdbarch *gdbarch,
				const char *name, int len);

/* Default implementation of gdbarch_remove_non_address_bits.  */
CORE_ADDR default_remove_non_address_bits (struct gdbarch *gdbarch,
					   CORE_ADDR pointer);

/* Default implementation of gdbarch_memtag_to_string.  */
extern std::string default_memtag_to_string (struct gdbarch *gdbarch,
					     struct value *tag);

/* Default implementation of gdbarch_tagged_address_p.  */
bool default_tagged_address_p (struct gdbarch *gdbarch, struct value *address);

/* Default implementation of gdbarch_memtag_matches_p.  */
extern bool default_memtag_matches_p (struct gdbarch *gdbarch,
				       struct value *address);

/* Default implementation of gdbarch_set_memtags.  */
bool default_set_memtags (struct gdbarch *gdbarch,
			  struct value *address, size_t length,
			  const gdb::byte_vector &tags,
			  memtag_type tag_type);

/* Default implementation of gdbarch_get_memtag.  */
struct value *default_get_memtag (struct gdbarch *gdbarch,
				  struct value *address,
				  memtag_type tag_type);

extern CORE_ADDR generic_skip_trampoline_code (frame_info_ptr frame,
					       CORE_ADDR pc);

extern CORE_ADDR generic_skip_solib_resolver (struct gdbarch *gdbarch,
					      CORE_ADDR pc);

extern int generic_in_solib_return_trampoline (struct gdbarch *gdbarch,
					       CORE_ADDR pc, const char *name);

extern int generic_stack_frame_destroyed_p (struct gdbarch *gdbarch,
					    CORE_ADDR pc);

extern int default_code_of_frame_writable (struct gdbarch *gdbarch,
					   frame_info_ptr frame);

/* By default, registers are not convertible.  */
extern int generic_convert_register_p (struct gdbarch *gdbarch, int regnum,
				       struct type *type);

extern int default_stabs_argument_has_addr (struct gdbarch *gdbarch,
					    struct type *type);

extern int generic_instruction_nullified (struct gdbarch *gdbarch,
					  struct regcache *regcache);

int default_remote_register_number (struct gdbarch *gdbarch,
				    int regno);

/* For compatibility with older architectures, returns
   (LEGACY_SIM_REGNO_IGNORE) when the register doesn't have a valid
   name.  */

extern int legacy_register_sim_regno (struct gdbarch *gdbarch, int regnum);

/* Return the selected byte order, or BFD_ENDIAN_UNKNOWN if no byte
   order was explicitly selected.  */
extern enum bfd_endian selected_byte_order (void);

/* Return the selected architecture's name, or NULL if no architecture
   was explicitly selected.  */
extern const char *selected_architecture_name (void);

/* Similar to init, but this time fill in the blanks.  Information is
   obtained from the global "set ..." options and explicitly
   initialized INFO fields.  */
extern void gdbarch_info_fill (struct gdbarch_info *info);

/* Return the architecture for ABFD.  If no suitable architecture
   could be find, return NULL.  */

extern struct gdbarch *gdbarch_from_bfd (bfd *abfd);

/* Return "current" architecture.  If the target is running, this is the
   architecture of the selected frame.  Otherwise, the "current" architecture
   defaults to the target architecture.

   This function should normally be called solely by the command interpreter
   routines to determine the architecture to execute a command in.  */
extern struct gdbarch *get_current_arch (void);

extern int default_has_shared_address_space (struct gdbarch *);

extern int default_fast_tracepoint_valid_at (struct gdbarch *gdbarch,
					     CORE_ADDR addr, std::string *msg);

extern const gdb_byte *default_breakpoint_from_pc (struct gdbarch *gdbarch,
						   CORE_ADDR *pcptr,
						   int *lenptr);

extern int default_breakpoint_kind_from_current_state (struct gdbarch *gdbarch,
						       struct regcache *regcache,
						       CORE_ADDR *pcptr);

extern void default_gen_return_address (struct gdbarch *gdbarch,
					struct agent_expr *ax,
					struct axs_value *value,
					CORE_ADDR scope);

extern const char *default_auto_charset (void);
extern const char *default_auto_wide_charset (void);

extern int default_return_in_first_hidden_param_p (struct gdbarch *,
						   struct type *);

extern int default_insn_is_call (struct gdbarch *, CORE_ADDR);
extern int default_insn_is_ret (struct gdbarch *, CORE_ADDR);
extern int default_insn_is_jump (struct gdbarch *, CORE_ADDR);

/* Default implementation of gdbarch_program_breakpoint_here_p.  */
extern bool default_program_breakpoint_here_p (struct gdbarch *gdbarch,
					       CORE_ADDR addr);

/* Do-nothing version of vsyscall_range.  Returns false.  */

extern int default_vsyscall_range (struct gdbarch *gdbarch, struct mem_range *range);

/* Default way to advance the PC to the next instruction in order to
   skip a permanent breakpoint.  Increments the PC by the size of a
   software breakpoint instruction, as determined with
   gdbarch_breakpoint_from_pc.  This matches how the breakpoints
   module determines whether a breakpoint is permanent.  */
extern void default_skip_permanent_breakpoint (struct regcache *regcache);

/* Symbols for gdbarch_infcall_mmap; their Linux PROT_* system
   definitions would be dependent on compilation host.  */
#define GDB_MMAP_PROT_READ	0x1	/* Page can be read.  */
#define GDB_MMAP_PROT_WRITE	0x2	/* Page can be written.  */
#define GDB_MMAP_PROT_EXEC	0x4	/* Page can be executed.  */

extern CORE_ADDR default_infcall_mmap (CORE_ADDR size, unsigned prot);
extern void default_infcall_munmap (CORE_ADDR addr, CORE_ADDR size);
extern std::string default_gcc_target_options (struct gdbarch *gdbarch);
extern const char *default_gnu_triplet_regexp (struct gdbarch *gdbarch);
extern int default_addressable_memory_unit_size (struct gdbarch *gdbarch);

extern void default_guess_tracepoint_registers (struct gdbarch *gdbarch,
						struct regcache *regcache,
						CORE_ADDR addr);

extern int default_print_insn (bfd_vma memaddr, disassemble_info *info);

/* Wrapper to gdbarch_skip_prologue, but doesn't throw exception.  Catch
   exception thrown from gdbarch_skip_prologue, and return PC.  */

extern CORE_ADDR gdbarch_skip_prologue_noexcept (gdbarch *gdbarch,
						 CORE_ADDR pc) noexcept;

/* Default implementation of gdbarch_in_indirect_branch_thunk that returns
   false.  */
extern bool default_in_indirect_branch_thunk (gdbarch *gdbarch,
					      CORE_ADDR pc);

/* Default implementation of gdbarch type_align method.  */
extern ULONGEST default_type_align (struct gdbarch *gdbarch,
				    struct type *type);

/* Default implementation of gdbarch get_pc_address_flags method.  */
extern std::string default_get_pc_address_flags (frame_info_ptr frame,
						 CORE_ADDR pc);

/* Default implementation of gdbarch read_core_file_mappings method.  */
extern void default_read_core_file_mappings
  (struct gdbarch *gdbarch,
   struct bfd *cbfd,
   read_core_file_mappings_pre_loop_ftype pre_loop_cb,
   read_core_file_mappings_loop_ftype loop_cb);

/* Default implementation of gdbarch
   use_target_description_from_corefile_notes.  */
extern bool default_use_target_description_from_corefile_notes
  (struct gdbarch *gdbarch,
  struct bfd *corefile_bfd);

/* Default implementation of gdbarch default_get_return_buf_addr method.  */
extern CORE_ADDR default_get_return_buf_addr (struct type *val_typegdbarch,
					      frame_info_ptr cur_frame);

/* Default implementation of gdbarch default_dwarf2_omit_typedef_p method.  */
extern bool default_dwarf2_omit_typedef_p (struct type *target_type,
					   const char *producer,
					   const char *name);

extern enum return_value_convention default_gdbarch_return_value
     (struct gdbarch *gdbarch, struct value *function, struct type *valtype,
      struct regcache *regcache, struct value **read_value,
      const gdb_byte *writebuf);

#endif /* ARCH_UTILS_H */
