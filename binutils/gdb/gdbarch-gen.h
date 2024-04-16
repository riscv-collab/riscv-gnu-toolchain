/* THIS FILE IS GENERATED -*- buffer-read-only: t -*- */
/* vi:set ro: */

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

/* To regenerate this file, run:
   ./gdbarch.py
*/



/* The following are pre-initialized by GDBARCH.  */

extern const struct bfd_arch_info * gdbarch_bfd_arch_info (struct gdbarch *gdbarch);
/* set_gdbarch_bfd_arch_info() - not applicable - pre-initialized.  */

extern enum bfd_endian gdbarch_byte_order (struct gdbarch *gdbarch);
/* set_gdbarch_byte_order() - not applicable - pre-initialized.  */

extern enum bfd_endian gdbarch_byte_order_for_code (struct gdbarch *gdbarch);
/* set_gdbarch_byte_order_for_code() - not applicable - pre-initialized.  */

extern enum gdb_osabi gdbarch_osabi (struct gdbarch *gdbarch);
/* set_gdbarch_osabi() - not applicable - pre-initialized.  */

extern const struct target_desc * gdbarch_target_desc (struct gdbarch *gdbarch);
/* set_gdbarch_target_desc() - not applicable - pre-initialized.  */


/* The following are initialized by the target dependent code.  */

/* Number of bits in a short or unsigned short for the target machine. */

extern int gdbarch_short_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_short_bit (struct gdbarch *gdbarch, int short_bit);

/* Number of bits in an int or unsigned int for the target machine. */

extern int gdbarch_int_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_int_bit (struct gdbarch *gdbarch, int int_bit);

/* Number of bits in a long or unsigned long for the target machine. */

extern int gdbarch_long_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_long_bit (struct gdbarch *gdbarch, int long_bit);

/* Number of bits in a long long or unsigned long long for the target
   machine. */

extern int gdbarch_long_long_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_long_long_bit (struct gdbarch *gdbarch, int long_long_bit);

/* The ABI default bit-size and format for "bfloat16", "half", "float", "double", and
   "long double".  These bit/format pairs should eventually be combined
   into a single object.  For the moment, just initialize them as a pair.
   Each format describes both the big and little endian layouts (if
   useful). */

extern int gdbarch_bfloat16_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_bfloat16_bit (struct gdbarch *gdbarch, int bfloat16_bit);

extern const struct floatformat ** gdbarch_bfloat16_format (struct gdbarch *gdbarch);
extern void set_gdbarch_bfloat16_format (struct gdbarch *gdbarch, const struct floatformat ** bfloat16_format);

extern int gdbarch_half_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_half_bit (struct gdbarch *gdbarch, int half_bit);

extern const struct floatformat ** gdbarch_half_format (struct gdbarch *gdbarch);
extern void set_gdbarch_half_format (struct gdbarch *gdbarch, const struct floatformat ** half_format);

extern int gdbarch_float_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_float_bit (struct gdbarch *gdbarch, int float_bit);

extern const struct floatformat ** gdbarch_float_format (struct gdbarch *gdbarch);
extern void set_gdbarch_float_format (struct gdbarch *gdbarch, const struct floatformat ** float_format);

extern int gdbarch_double_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_double_bit (struct gdbarch *gdbarch, int double_bit);

extern const struct floatformat ** gdbarch_double_format (struct gdbarch *gdbarch);
extern void set_gdbarch_double_format (struct gdbarch *gdbarch, const struct floatformat ** double_format);

extern int gdbarch_long_double_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_long_double_bit (struct gdbarch *gdbarch, int long_double_bit);

extern const struct floatformat ** gdbarch_long_double_format (struct gdbarch *gdbarch);
extern void set_gdbarch_long_double_format (struct gdbarch *gdbarch, const struct floatformat ** long_double_format);

/* The ABI default bit-size for "wchar_t".  wchar_t is a built-in type
   starting with C++11. */

extern int gdbarch_wchar_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_wchar_bit (struct gdbarch *gdbarch, int wchar_bit);

/* One if `wchar_t' is signed, zero if unsigned. */

extern int gdbarch_wchar_signed (struct gdbarch *gdbarch);
extern void set_gdbarch_wchar_signed (struct gdbarch *gdbarch, int wchar_signed);

/* Returns the floating-point format to be used for values of length LENGTH.
   NAME, if non-NULL, is the type name, which may be used to distinguish
   different target formats of the same length. */

typedef const struct floatformat ** (gdbarch_floatformat_for_type_ftype) (struct gdbarch *gdbarch, const char *name, int length);
extern const struct floatformat ** gdbarch_floatformat_for_type (struct gdbarch *gdbarch, const char *name, int length);
extern void set_gdbarch_floatformat_for_type (struct gdbarch *gdbarch, gdbarch_floatformat_for_type_ftype *floatformat_for_type);

/* For most targets, a pointer on the target and its representation as an
   address in GDB have the same size and "look the same".  For such a
   target, you need only set gdbarch_ptr_bit and gdbarch_addr_bit
   / addr_bit will be set from it.

   If gdbarch_ptr_bit and gdbarch_addr_bit are different, you'll probably
   also need to set gdbarch_dwarf2_addr_size, gdbarch_pointer_to_address and
   gdbarch_address_to_pointer as well.

   ptr_bit is the size of a pointer on the target */

extern int gdbarch_ptr_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_ptr_bit (struct gdbarch *gdbarch, int ptr_bit);

/* addr_bit is the size of a target address as represented in gdb */

extern int gdbarch_addr_bit (struct gdbarch *gdbarch);
extern void set_gdbarch_addr_bit (struct gdbarch *gdbarch, int addr_bit);

/* dwarf2_addr_size is the target address size as used in the Dwarf debug
   info.  For .debug_frame FDEs, this is supposed to be the target address
   size from the associated CU header, and which is equivalent to the
   DWARF2_ADDR_SIZE as defined by the target specific GCC back-end.
   Unfortunately there is no good way to determine this value.  Therefore
   dwarf2_addr_size simply defaults to the target pointer size.

   dwarf2_addr_size is not used for .eh_frame FDEs, which are generally
   defined using the target's pointer size so far.

   Note that dwarf2_addr_size only needs to be redefined by a target if the
   GCC back-end defines a DWARF2_ADDR_SIZE other than the target pointer size,
   and if Dwarf versions < 4 need to be supported. */

extern int gdbarch_dwarf2_addr_size (struct gdbarch *gdbarch);
extern void set_gdbarch_dwarf2_addr_size (struct gdbarch *gdbarch, int dwarf2_addr_size);

/* One if `char' acts like `signed char', zero if `unsigned char'. */

extern int gdbarch_char_signed (struct gdbarch *gdbarch);
extern void set_gdbarch_char_signed (struct gdbarch *gdbarch, int char_signed);

extern bool gdbarch_read_pc_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_read_pc_ftype) (readable_regcache *regcache);
extern CORE_ADDR gdbarch_read_pc (struct gdbarch *gdbarch, readable_regcache *regcache);
extern void set_gdbarch_read_pc (struct gdbarch *gdbarch, gdbarch_read_pc_ftype *read_pc);

extern bool gdbarch_write_pc_p (struct gdbarch *gdbarch);

typedef void (gdbarch_write_pc_ftype) (struct regcache *regcache, CORE_ADDR val);
extern void gdbarch_write_pc (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR val);
extern void set_gdbarch_write_pc (struct gdbarch *gdbarch, gdbarch_write_pc_ftype *write_pc);

/* Function for getting target's idea of a frame pointer.  FIXME: GDB's
   whole scheme for dealing with "frames" and "frame pointers" needs a
   serious shakedown. */

typedef void (gdbarch_virtual_frame_pointer_ftype) (struct gdbarch *gdbarch, CORE_ADDR pc, int *frame_regnum, LONGEST *frame_offset);
extern void gdbarch_virtual_frame_pointer (struct gdbarch *gdbarch, CORE_ADDR pc, int *frame_regnum, LONGEST *frame_offset);
extern void set_gdbarch_virtual_frame_pointer (struct gdbarch *gdbarch, gdbarch_virtual_frame_pointer_ftype *virtual_frame_pointer);

extern bool gdbarch_pseudo_register_read_p (struct gdbarch *gdbarch);

typedef enum register_status (gdbarch_pseudo_register_read_ftype) (struct gdbarch *gdbarch, readable_regcache *regcache, int cookednum, gdb_byte *buf);
extern enum register_status gdbarch_pseudo_register_read (struct gdbarch *gdbarch, readable_regcache *regcache, int cookednum, gdb_byte *buf);
extern void set_gdbarch_pseudo_register_read (struct gdbarch *gdbarch, gdbarch_pseudo_register_read_ftype *pseudo_register_read);

/* Read a register into a new struct value.  If the register is wholly
   or partly unavailable, this should call mark_value_bytes_unavailable
   as appropriate.  If this is defined, then pseudo_register_read will
   never be called. */

extern bool gdbarch_pseudo_register_read_value_p (struct gdbarch *gdbarch);

typedef struct value * (gdbarch_pseudo_register_read_value_ftype) (struct gdbarch *gdbarch, frame_info_ptr next_frame, int cookednum);
extern struct value * gdbarch_pseudo_register_read_value (struct gdbarch *gdbarch, frame_info_ptr next_frame, int cookednum);
extern void set_gdbarch_pseudo_register_read_value (struct gdbarch *gdbarch, gdbarch_pseudo_register_read_value_ftype *pseudo_register_read_value);

/* Write bytes in BUF to pseudo register with number PSEUDO_REG_NUM.

   Raw registers backing the pseudo register should be written to using
   NEXT_FRAME. */

extern bool gdbarch_pseudo_register_write_p (struct gdbarch *gdbarch);

typedef void (gdbarch_pseudo_register_write_ftype) (struct gdbarch *gdbarch, frame_info_ptr next_frame, int pseudo_reg_num, gdb::array_view<const gdb_byte> buf);
extern void gdbarch_pseudo_register_write (struct gdbarch *gdbarch, frame_info_ptr next_frame, int pseudo_reg_num, gdb::array_view<const gdb_byte> buf);
extern void set_gdbarch_pseudo_register_write (struct gdbarch *gdbarch, gdbarch_pseudo_register_write_ftype *pseudo_register_write);

/* Write bytes to a pseudo register.

   This is marked as deprecated because it gets passed a regcache for
   implementations to write raw registers in.  This doesn't work for unwound
   frames, where the raw registers backing the pseudo registers may have been
   saved elsewhere.

   Implementations should be migrated to implement pseudo_register_write instead. */

extern bool gdbarch_deprecated_pseudo_register_write_p (struct gdbarch *gdbarch);

typedef void (gdbarch_deprecated_pseudo_register_write_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, int cookednum, const gdb_byte *buf);
extern void gdbarch_deprecated_pseudo_register_write (struct gdbarch *gdbarch, struct regcache *regcache, int cookednum, const gdb_byte *buf);
extern void set_gdbarch_deprecated_pseudo_register_write (struct gdbarch *gdbarch, gdbarch_deprecated_pseudo_register_write_ftype *deprecated_pseudo_register_write);

extern int gdbarch_num_regs (struct gdbarch *gdbarch);
extern void set_gdbarch_num_regs (struct gdbarch *gdbarch, int num_regs);

/* This macro gives the number of pseudo-registers that live in the
   register namespace but do not get fetched or stored on the target.
   These pseudo-registers may be aliases for other registers,
   combinations of other registers, or they may be computed by GDB. */

extern int gdbarch_num_pseudo_regs (struct gdbarch *gdbarch);
extern void set_gdbarch_num_pseudo_regs (struct gdbarch *gdbarch, int num_pseudo_regs);

/* Assemble agent expression bytecode to collect pseudo-register REG.
   Return -1 if something goes wrong, 0 otherwise. */

extern bool gdbarch_ax_pseudo_register_collect_p (struct gdbarch *gdbarch);

typedef int (gdbarch_ax_pseudo_register_collect_ftype) (struct gdbarch *gdbarch, struct agent_expr *ax, int reg);
extern int gdbarch_ax_pseudo_register_collect (struct gdbarch *gdbarch, struct agent_expr *ax, int reg);
extern void set_gdbarch_ax_pseudo_register_collect (struct gdbarch *gdbarch, gdbarch_ax_pseudo_register_collect_ftype *ax_pseudo_register_collect);

/* Assemble agent expression bytecode to push the value of pseudo-register
   REG on the interpreter stack.
   Return -1 if something goes wrong, 0 otherwise. */

extern bool gdbarch_ax_pseudo_register_push_stack_p (struct gdbarch *gdbarch);

typedef int (gdbarch_ax_pseudo_register_push_stack_ftype) (struct gdbarch *gdbarch, struct agent_expr *ax, int reg);
extern int gdbarch_ax_pseudo_register_push_stack (struct gdbarch *gdbarch, struct agent_expr *ax, int reg);
extern void set_gdbarch_ax_pseudo_register_push_stack (struct gdbarch *gdbarch, gdbarch_ax_pseudo_register_push_stack_ftype *ax_pseudo_register_push_stack);

/* Some architectures can display additional information for specific
   signals.
   UIOUT is the output stream where the handler will place information. */

extern bool gdbarch_report_signal_info_p (struct gdbarch *gdbarch);

typedef void (gdbarch_report_signal_info_ftype) (struct gdbarch *gdbarch, struct ui_out *uiout, enum gdb_signal siggnal);
extern void gdbarch_report_signal_info (struct gdbarch *gdbarch, struct ui_out *uiout, enum gdb_signal siggnal);
extern void set_gdbarch_report_signal_info (struct gdbarch *gdbarch, gdbarch_report_signal_info_ftype *report_signal_info);

/* GDB's standard (or well known) register numbers.  These can map onto
   a real register or a pseudo (computed) register or not be defined at
   all (-1).
   gdbarch_sp_regnum will hopefully be replaced by UNWIND_SP. */

extern int gdbarch_sp_regnum (struct gdbarch *gdbarch);
extern void set_gdbarch_sp_regnum (struct gdbarch *gdbarch, int sp_regnum);

extern int gdbarch_pc_regnum (struct gdbarch *gdbarch);
extern void set_gdbarch_pc_regnum (struct gdbarch *gdbarch, int pc_regnum);

extern int gdbarch_ps_regnum (struct gdbarch *gdbarch);
extern void set_gdbarch_ps_regnum (struct gdbarch *gdbarch, int ps_regnum);

extern int gdbarch_fp0_regnum (struct gdbarch *gdbarch);
extern void set_gdbarch_fp0_regnum (struct gdbarch *gdbarch, int fp0_regnum);

/* Convert stab register number (from `r' declaration) to a gdb REGNUM. */

typedef int (gdbarch_stab_reg_to_regnum_ftype) (struct gdbarch *gdbarch, int stab_regnr);
extern int gdbarch_stab_reg_to_regnum (struct gdbarch *gdbarch, int stab_regnr);
extern void set_gdbarch_stab_reg_to_regnum (struct gdbarch *gdbarch, gdbarch_stab_reg_to_regnum_ftype *stab_reg_to_regnum);

/* Provide a default mapping from a ecoff register number to a gdb REGNUM. */

typedef int (gdbarch_ecoff_reg_to_regnum_ftype) (struct gdbarch *gdbarch, int ecoff_regnr);
extern int gdbarch_ecoff_reg_to_regnum (struct gdbarch *gdbarch, int ecoff_regnr);
extern void set_gdbarch_ecoff_reg_to_regnum (struct gdbarch *gdbarch, gdbarch_ecoff_reg_to_regnum_ftype *ecoff_reg_to_regnum);

/* Convert from an sdb register number to an internal gdb register number. */

typedef int (gdbarch_sdb_reg_to_regnum_ftype) (struct gdbarch *gdbarch, int sdb_regnr);
extern int gdbarch_sdb_reg_to_regnum (struct gdbarch *gdbarch, int sdb_regnr);
extern void set_gdbarch_sdb_reg_to_regnum (struct gdbarch *gdbarch, gdbarch_sdb_reg_to_regnum_ftype *sdb_reg_to_regnum);

/* Provide a default mapping from a DWARF2 register number to a gdb REGNUM.
   Return -1 for bad REGNUM.  Note: Several targets get this wrong. */

typedef int (gdbarch_dwarf2_reg_to_regnum_ftype) (struct gdbarch *gdbarch, int dwarf2_regnr);
extern int gdbarch_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, int dwarf2_regnr);
extern void set_gdbarch_dwarf2_reg_to_regnum (struct gdbarch *gdbarch, gdbarch_dwarf2_reg_to_regnum_ftype *dwarf2_reg_to_regnum);

/* Return the name of register REGNR for the specified architecture.
   REGNR can be any value greater than, or equal to zero, and less than
   'gdbarch_num_cooked_regs (GDBARCH)'.  If REGNR is not supported for
   GDBARCH, then this function will return an empty string, this function
   should never return nullptr. */

typedef const char * (gdbarch_register_name_ftype) (struct gdbarch *gdbarch, int regnr);
extern const char * gdbarch_register_name (struct gdbarch *gdbarch, int regnr);
extern void set_gdbarch_register_name (struct gdbarch *gdbarch, gdbarch_register_name_ftype *register_name);

/* Return the type of a register specified by the architecture.  Only
   the register cache should call this function directly; others should
   use "register_type". */

typedef struct type * (gdbarch_register_type_ftype) (struct gdbarch *gdbarch, int reg_nr);
extern struct type * gdbarch_register_type (struct gdbarch *gdbarch, int reg_nr);
extern void set_gdbarch_register_type (struct gdbarch *gdbarch, gdbarch_register_type_ftype *register_type);

/* Generate a dummy frame_id for THIS_FRAME assuming that the frame is
   a dummy frame.  A dummy frame is created before an inferior call,
   the frame_id returned here must match the frame_id that was built
   for the inferior call.  Usually this means the returned frame_id's
   stack address should match the address returned by
   gdbarch_push_dummy_call, and the returned frame_id's code address
   should match the address at which the breakpoint was set in the dummy
   frame. */

typedef struct frame_id (gdbarch_dummy_id_ftype) (struct gdbarch *gdbarch, frame_info_ptr this_frame);
extern struct frame_id gdbarch_dummy_id (struct gdbarch *gdbarch, frame_info_ptr this_frame);
extern void set_gdbarch_dummy_id (struct gdbarch *gdbarch, gdbarch_dummy_id_ftype *dummy_id);

/* Implement DUMMY_ID and PUSH_DUMMY_CALL, then delete
   deprecated_fp_regnum. */

extern int gdbarch_deprecated_fp_regnum (struct gdbarch *gdbarch);
extern void set_gdbarch_deprecated_fp_regnum (struct gdbarch *gdbarch, int deprecated_fp_regnum);

extern bool gdbarch_push_dummy_call_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_push_dummy_call_ftype) (struct gdbarch *gdbarch, struct value *function, struct regcache *regcache, CORE_ADDR bp_addr, int nargs, struct value **args, CORE_ADDR sp, function_call_return_method return_method, CORE_ADDR struct_addr);
extern CORE_ADDR gdbarch_push_dummy_call (struct gdbarch *gdbarch, struct value *function, struct regcache *regcache, CORE_ADDR bp_addr, int nargs, struct value **args, CORE_ADDR sp, function_call_return_method return_method, CORE_ADDR struct_addr);
extern void set_gdbarch_push_dummy_call (struct gdbarch *gdbarch, gdbarch_push_dummy_call_ftype *push_dummy_call);

extern enum call_dummy_location_type gdbarch_call_dummy_location (struct gdbarch *gdbarch);
extern void set_gdbarch_call_dummy_location (struct gdbarch *gdbarch, enum call_dummy_location_type call_dummy_location);

extern bool gdbarch_push_dummy_code_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_push_dummy_code_ftype) (struct gdbarch *gdbarch, CORE_ADDR sp, CORE_ADDR funaddr, struct value **args, int nargs, struct type *value_type, CORE_ADDR *real_pc, CORE_ADDR *bp_addr, struct regcache *regcache);
extern CORE_ADDR gdbarch_push_dummy_code (struct gdbarch *gdbarch, CORE_ADDR sp, CORE_ADDR funaddr, struct value **args, int nargs, struct type *value_type, CORE_ADDR *real_pc, CORE_ADDR *bp_addr, struct regcache *regcache);
extern void set_gdbarch_push_dummy_code (struct gdbarch *gdbarch, gdbarch_push_dummy_code_ftype *push_dummy_code);

/* Return true if the code of FRAME is writable. */

typedef int (gdbarch_code_of_frame_writable_ftype) (struct gdbarch *gdbarch, frame_info_ptr frame);
extern int gdbarch_code_of_frame_writable (struct gdbarch *gdbarch, frame_info_ptr frame);
extern void set_gdbarch_code_of_frame_writable (struct gdbarch *gdbarch, gdbarch_code_of_frame_writable_ftype *code_of_frame_writable);

typedef void (gdbarch_print_registers_info_ftype) (struct gdbarch *gdbarch, struct ui_file *file, frame_info_ptr frame, int regnum, int all);
extern void gdbarch_print_registers_info (struct gdbarch *gdbarch, struct ui_file *file, frame_info_ptr frame, int regnum, int all);
extern void set_gdbarch_print_registers_info (struct gdbarch *gdbarch, gdbarch_print_registers_info_ftype *print_registers_info);

typedef void (gdbarch_print_float_info_ftype) (struct gdbarch *gdbarch, struct ui_file *file, frame_info_ptr frame, const char *args);
extern void gdbarch_print_float_info (struct gdbarch *gdbarch, struct ui_file *file, frame_info_ptr frame, const char *args);
extern void set_gdbarch_print_float_info (struct gdbarch *gdbarch, gdbarch_print_float_info_ftype *print_float_info);

extern bool gdbarch_print_vector_info_p (struct gdbarch *gdbarch);

typedef void (gdbarch_print_vector_info_ftype) (struct gdbarch *gdbarch, struct ui_file *file, frame_info_ptr frame, const char *args);
extern void gdbarch_print_vector_info (struct gdbarch *gdbarch, struct ui_file *file, frame_info_ptr frame, const char *args);
extern void set_gdbarch_print_vector_info (struct gdbarch *gdbarch, gdbarch_print_vector_info_ftype *print_vector_info);

/* MAP a GDB RAW register number onto a simulator register number.  See
   also include/...-sim.h. */

typedef int (gdbarch_register_sim_regno_ftype) (struct gdbarch *gdbarch, int reg_nr);
extern int gdbarch_register_sim_regno (struct gdbarch *gdbarch, int reg_nr);
extern void set_gdbarch_register_sim_regno (struct gdbarch *gdbarch, gdbarch_register_sim_regno_ftype *register_sim_regno);

typedef int (gdbarch_cannot_fetch_register_ftype) (struct gdbarch *gdbarch, int regnum);
extern int gdbarch_cannot_fetch_register (struct gdbarch *gdbarch, int regnum);
extern void set_gdbarch_cannot_fetch_register (struct gdbarch *gdbarch, gdbarch_cannot_fetch_register_ftype *cannot_fetch_register);

typedef int (gdbarch_cannot_store_register_ftype) (struct gdbarch *gdbarch, int regnum);
extern int gdbarch_cannot_store_register (struct gdbarch *gdbarch, int regnum);
extern void set_gdbarch_cannot_store_register (struct gdbarch *gdbarch, gdbarch_cannot_store_register_ftype *cannot_store_register);

/* Determine the address where a longjmp will land and save this address
   in PC.  Return nonzero on success.

   FRAME corresponds to the longjmp frame. */

extern bool gdbarch_get_longjmp_target_p (struct gdbarch *gdbarch);

typedef int (gdbarch_get_longjmp_target_ftype) (frame_info_ptr frame, CORE_ADDR *pc);
extern int gdbarch_get_longjmp_target (struct gdbarch *gdbarch, frame_info_ptr frame, CORE_ADDR *pc);
extern void set_gdbarch_get_longjmp_target (struct gdbarch *gdbarch, gdbarch_get_longjmp_target_ftype *get_longjmp_target);

extern int gdbarch_believe_pcc_promotion (struct gdbarch *gdbarch);
extern void set_gdbarch_believe_pcc_promotion (struct gdbarch *gdbarch, int believe_pcc_promotion);

typedef int (gdbarch_convert_register_p_ftype) (struct gdbarch *gdbarch, int regnum, struct type *type);
extern int gdbarch_convert_register_p (struct gdbarch *gdbarch, int regnum, struct type *type);
extern void set_gdbarch_convert_register_p (struct gdbarch *gdbarch, gdbarch_convert_register_p_ftype *convert_register_p);

typedef int (gdbarch_register_to_value_ftype) (frame_info_ptr frame, int regnum, struct type *type, gdb_byte *buf, int *optimizedp, int *unavailablep);
extern int gdbarch_register_to_value (struct gdbarch *gdbarch, frame_info_ptr frame, int regnum, struct type *type, gdb_byte *buf, int *optimizedp, int *unavailablep);
extern void set_gdbarch_register_to_value (struct gdbarch *gdbarch, gdbarch_register_to_value_ftype *register_to_value);

typedef void (gdbarch_value_to_register_ftype) (frame_info_ptr frame, int regnum, struct type *type, const gdb_byte *buf);
extern void gdbarch_value_to_register (struct gdbarch *gdbarch, frame_info_ptr frame, int regnum, struct type *type, const gdb_byte *buf);
extern void set_gdbarch_value_to_register (struct gdbarch *gdbarch, gdbarch_value_to_register_ftype *value_to_register);

/* Construct a value representing the contents of register REGNUM in
   frame THIS_FRAME, interpreted as type TYPE.  The routine needs to
   allocate and return a struct value with all value attributes
   (but not the value contents) filled in. */

typedef struct value * (gdbarch_value_from_register_ftype) (struct gdbarch *gdbarch, struct type *type, int regnum, const frame_info_ptr &this_frame);
extern struct value * gdbarch_value_from_register (struct gdbarch *gdbarch, struct type *type, int regnum, const frame_info_ptr &this_frame);
extern void set_gdbarch_value_from_register (struct gdbarch *gdbarch, gdbarch_value_from_register_ftype *value_from_register);

typedef CORE_ADDR (gdbarch_pointer_to_address_ftype) (struct gdbarch *gdbarch, struct type *type, const gdb_byte *buf);
extern CORE_ADDR gdbarch_pointer_to_address (struct gdbarch *gdbarch, struct type *type, const gdb_byte *buf);
extern void set_gdbarch_pointer_to_address (struct gdbarch *gdbarch, gdbarch_pointer_to_address_ftype *pointer_to_address);

typedef void (gdbarch_address_to_pointer_ftype) (struct gdbarch *gdbarch, struct type *type, gdb_byte *buf, CORE_ADDR addr);
extern void gdbarch_address_to_pointer (struct gdbarch *gdbarch, struct type *type, gdb_byte *buf, CORE_ADDR addr);
extern void set_gdbarch_address_to_pointer (struct gdbarch *gdbarch, gdbarch_address_to_pointer_ftype *address_to_pointer);

extern bool gdbarch_integer_to_address_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_integer_to_address_ftype) (struct gdbarch *gdbarch, struct type *type, const gdb_byte *buf);
extern CORE_ADDR gdbarch_integer_to_address (struct gdbarch *gdbarch, struct type *type, const gdb_byte *buf);
extern void set_gdbarch_integer_to_address (struct gdbarch *gdbarch, gdbarch_integer_to_address_ftype *integer_to_address);

/* Return the return-value convention that will be used by FUNCTION
   to return a value of type VALTYPE.  FUNCTION may be NULL in which
   case the return convention is computed based only on VALTYPE.

   If READBUF is not NULL, extract the return value and save it in this buffer.

   If WRITEBUF is not NULL, it contains a return value which will be
   stored into the appropriate register.  This can be used when we want
   to force the value returned by a function (see the "return" command
   for instance).

   NOTE: it is better to implement return_value_as_value instead, as that
   method can properly handle variably-sized types. */

typedef enum return_value_convention (gdbarch_return_value_ftype) (struct gdbarch *gdbarch, struct value *function, struct type *valtype, struct regcache *regcache, gdb_byte *readbuf, const gdb_byte *writebuf);
extern void set_gdbarch_return_value (struct gdbarch *gdbarch, gdbarch_return_value_ftype *return_value);

/* Return the return-value convention that will be used by FUNCTION
   to return a value of type VALTYPE.  FUNCTION may be NULL in which
   case the return convention is computed based only on VALTYPE.

   If READ_VALUE is not NULL, extract the return value and save it in
   this pointer.

   If WRITEBUF is not NULL, it contains a return value which will be
   stored into the appropriate register.  This can be used when we want
   to force the value returned by a function (see the "return" command
   for instance). */

typedef enum return_value_convention (gdbarch_return_value_as_value_ftype) (struct gdbarch *gdbarch, struct value *function, struct type *valtype, struct regcache *regcache, struct value **read_value, const gdb_byte *writebuf);
extern enum return_value_convention gdbarch_return_value_as_value (struct gdbarch *gdbarch, struct value *function, struct type *valtype, struct regcache *regcache, struct value **read_value, const gdb_byte *writebuf);
extern void set_gdbarch_return_value_as_value (struct gdbarch *gdbarch, gdbarch_return_value_as_value_ftype *return_value_as_value);

/* Return the address at which the value being returned from
   the current function will be stored.  This routine is only
   called if the current function uses the the "struct return
   convention".

   May return 0 when unable to determine that address. */

typedef CORE_ADDR (gdbarch_get_return_buf_addr_ftype) (struct type *val_type, frame_info_ptr cur_frame);
extern CORE_ADDR gdbarch_get_return_buf_addr (struct gdbarch *gdbarch, struct type *val_type, frame_info_ptr cur_frame);
extern void set_gdbarch_get_return_buf_addr (struct gdbarch *gdbarch, gdbarch_get_return_buf_addr_ftype *get_return_buf_addr);

/* Return true if the typedef record needs to be replaced.".

   Return 0 by default */

typedef bool (gdbarch_dwarf2_omit_typedef_p_ftype) (struct type *target_type, const char *producer, const char *name);
extern bool gdbarch_dwarf2_omit_typedef_p (struct gdbarch *gdbarch, struct type *target_type, const char *producer, const char *name);
extern void set_gdbarch_dwarf2_omit_typedef_p (struct gdbarch *gdbarch, gdbarch_dwarf2_omit_typedef_p_ftype *dwarf2_omit_typedef_p);

/* Update PC when trying to find a call site.  This is useful on
   architectures where the call site PC, as reported in the DWARF, can be
   incorrect for some reason.

   The passed-in PC will be an address in the inferior.  GDB will have
   already failed to find a call site at this PC.  This function may
   simply return its parameter if it thinks that should be the correct
   address. */

typedef CORE_ADDR (gdbarch_update_call_site_pc_ftype) (struct gdbarch *gdbarch, CORE_ADDR pc);
extern CORE_ADDR gdbarch_update_call_site_pc (struct gdbarch *gdbarch, CORE_ADDR pc);
extern void set_gdbarch_update_call_site_pc (struct gdbarch *gdbarch, gdbarch_update_call_site_pc_ftype *update_call_site_pc);

/* Return true if the return value of function is stored in the first hidden
   parameter.  In theory, this feature should be language-dependent, specified
   by language and its ABI, such as C++.  Unfortunately, compiler may
   implement it to a target-dependent feature.  So that we need such hook here
   to be aware of this in GDB. */

typedef int (gdbarch_return_in_first_hidden_param_p_ftype) (struct gdbarch *gdbarch, struct type *type);
extern int gdbarch_return_in_first_hidden_param_p (struct gdbarch *gdbarch, struct type *type);
extern void set_gdbarch_return_in_first_hidden_param_p (struct gdbarch *gdbarch, gdbarch_return_in_first_hidden_param_p_ftype *return_in_first_hidden_param_p);

typedef CORE_ADDR (gdbarch_skip_prologue_ftype) (struct gdbarch *gdbarch, CORE_ADDR ip);
extern CORE_ADDR gdbarch_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR ip);
extern void set_gdbarch_skip_prologue (struct gdbarch *gdbarch, gdbarch_skip_prologue_ftype *skip_prologue);

extern bool gdbarch_skip_main_prologue_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_skip_main_prologue_ftype) (struct gdbarch *gdbarch, CORE_ADDR ip);
extern CORE_ADDR gdbarch_skip_main_prologue (struct gdbarch *gdbarch, CORE_ADDR ip);
extern void set_gdbarch_skip_main_prologue (struct gdbarch *gdbarch, gdbarch_skip_main_prologue_ftype *skip_main_prologue);

/* On some platforms, a single function may provide multiple entry points,
   e.g. one that is used for function-pointer calls and a different one
   that is used for direct function calls.
   In order to ensure that breakpoints set on the function will trigger
   no matter via which entry point the function is entered, a platform
   may provide the skip_entrypoint callback.  It is called with IP set
   to the main entry point of a function (as determined by the symbol table),
   and should return the address of the innermost entry point, where the
   actual breakpoint needs to be set.  Note that skip_entrypoint is used
   by GDB common code even when debugging optimized code, where skip_prologue
   is not used. */

extern bool gdbarch_skip_entrypoint_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_skip_entrypoint_ftype) (struct gdbarch *gdbarch, CORE_ADDR ip);
extern CORE_ADDR gdbarch_skip_entrypoint (struct gdbarch *gdbarch, CORE_ADDR ip);
extern void set_gdbarch_skip_entrypoint (struct gdbarch *gdbarch, gdbarch_skip_entrypoint_ftype *skip_entrypoint);

typedef int (gdbarch_inner_than_ftype) (CORE_ADDR lhs, CORE_ADDR rhs);
extern int gdbarch_inner_than (struct gdbarch *gdbarch, CORE_ADDR lhs, CORE_ADDR rhs);
extern void set_gdbarch_inner_than (struct gdbarch *gdbarch, gdbarch_inner_than_ftype *inner_than);

typedef const gdb_byte * (gdbarch_breakpoint_from_pc_ftype) (struct gdbarch *gdbarch, CORE_ADDR *pcptr, int *lenptr);
extern const gdb_byte * gdbarch_breakpoint_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr, int *lenptr);
extern void set_gdbarch_breakpoint_from_pc (struct gdbarch *gdbarch, gdbarch_breakpoint_from_pc_ftype *breakpoint_from_pc);

/* Return the breakpoint kind for this target based on *PCPTR. */

typedef int (gdbarch_breakpoint_kind_from_pc_ftype) (struct gdbarch *gdbarch, CORE_ADDR *pcptr);
extern int gdbarch_breakpoint_kind_from_pc (struct gdbarch *gdbarch, CORE_ADDR *pcptr);
extern void set_gdbarch_breakpoint_kind_from_pc (struct gdbarch *gdbarch, gdbarch_breakpoint_kind_from_pc_ftype *breakpoint_kind_from_pc);

/* Return the software breakpoint from KIND.  KIND can have target
   specific meaning like the Z0 kind parameter.
   SIZE is set to the software breakpoint's length in memory. */

typedef const gdb_byte * (gdbarch_sw_breakpoint_from_kind_ftype) (struct gdbarch *gdbarch, int kind, int *size);
extern const gdb_byte * gdbarch_sw_breakpoint_from_kind (struct gdbarch *gdbarch, int kind, int *size);
extern void set_gdbarch_sw_breakpoint_from_kind (struct gdbarch *gdbarch, gdbarch_sw_breakpoint_from_kind_ftype *sw_breakpoint_from_kind);

/* Return the breakpoint kind for this target based on the current
   processor state (e.g. the current instruction mode on ARM) and the
   *PCPTR.  In default, it is gdbarch->breakpoint_kind_from_pc. */

typedef int (gdbarch_breakpoint_kind_from_current_state_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR *pcptr);
extern int gdbarch_breakpoint_kind_from_current_state (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR *pcptr);
extern void set_gdbarch_breakpoint_kind_from_current_state (struct gdbarch *gdbarch, gdbarch_breakpoint_kind_from_current_state_ftype *breakpoint_kind_from_current_state);

extern bool gdbarch_adjust_breakpoint_address_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_adjust_breakpoint_address_ftype) (struct gdbarch *gdbarch, CORE_ADDR bpaddr);
extern CORE_ADDR gdbarch_adjust_breakpoint_address (struct gdbarch *gdbarch, CORE_ADDR bpaddr);
extern void set_gdbarch_adjust_breakpoint_address (struct gdbarch *gdbarch, gdbarch_adjust_breakpoint_address_ftype *adjust_breakpoint_address);

typedef int (gdbarch_memory_insert_breakpoint_ftype) (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt);
extern int gdbarch_memory_insert_breakpoint (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt);
extern void set_gdbarch_memory_insert_breakpoint (struct gdbarch *gdbarch, gdbarch_memory_insert_breakpoint_ftype *memory_insert_breakpoint);

typedef int (gdbarch_memory_remove_breakpoint_ftype) (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt);
extern int gdbarch_memory_remove_breakpoint (struct gdbarch *gdbarch, struct bp_target_info *bp_tgt);
extern void set_gdbarch_memory_remove_breakpoint (struct gdbarch *gdbarch, gdbarch_memory_remove_breakpoint_ftype *memory_remove_breakpoint);

extern CORE_ADDR gdbarch_decr_pc_after_break (struct gdbarch *gdbarch);
extern void set_gdbarch_decr_pc_after_break (struct gdbarch *gdbarch, CORE_ADDR decr_pc_after_break);

/* A function can be addressed by either its "pointer" (possibly a
   descriptor address) or "entry point" (first executable instruction).
   The method "convert_from_func_ptr_addr" converting the former to the
   latter.  gdbarch_deprecated_function_start_offset is being used to implement
   a simplified subset of that functionality - the function's address
   corresponds to the "function pointer" and the function's start
   corresponds to the "function entry point" - and hence is redundant. */

extern CORE_ADDR gdbarch_deprecated_function_start_offset (struct gdbarch *gdbarch);
extern void set_gdbarch_deprecated_function_start_offset (struct gdbarch *gdbarch, CORE_ADDR deprecated_function_start_offset);

/* Return the remote protocol register number associated with this
   register.  Normally the identity mapping. */

typedef int (gdbarch_remote_register_number_ftype) (struct gdbarch *gdbarch, int regno);
extern int gdbarch_remote_register_number (struct gdbarch *gdbarch, int regno);
extern void set_gdbarch_remote_register_number (struct gdbarch *gdbarch, gdbarch_remote_register_number_ftype *remote_register_number);

/* Fetch the target specific address used to represent a load module. */

extern bool gdbarch_fetch_tls_load_module_address_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_fetch_tls_load_module_address_ftype) (struct objfile *objfile);
extern CORE_ADDR gdbarch_fetch_tls_load_module_address (struct gdbarch *gdbarch, struct objfile *objfile);
extern void set_gdbarch_fetch_tls_load_module_address (struct gdbarch *gdbarch, gdbarch_fetch_tls_load_module_address_ftype *fetch_tls_load_module_address);

/* Return the thread-local address at OFFSET in the thread-local
   storage for the thread PTID and the shared library or executable
   file given by LM_ADDR.  If that block of thread-local storage hasn't
   been allocated yet, this function may throw an error.  LM_ADDR may
   be zero for statically linked multithreaded inferiors. */

extern bool gdbarch_get_thread_local_address_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_get_thread_local_address_ftype) (struct gdbarch *gdbarch, ptid_t ptid, CORE_ADDR lm_addr, CORE_ADDR offset);
extern CORE_ADDR gdbarch_get_thread_local_address (struct gdbarch *gdbarch, ptid_t ptid, CORE_ADDR lm_addr, CORE_ADDR offset);
extern void set_gdbarch_get_thread_local_address (struct gdbarch *gdbarch, gdbarch_get_thread_local_address_ftype *get_thread_local_address);

extern CORE_ADDR gdbarch_frame_args_skip (struct gdbarch *gdbarch);
extern void set_gdbarch_frame_args_skip (struct gdbarch *gdbarch, CORE_ADDR frame_args_skip);

typedef CORE_ADDR (gdbarch_unwind_pc_ftype) (struct gdbarch *gdbarch, frame_info_ptr next_frame);
extern CORE_ADDR gdbarch_unwind_pc (struct gdbarch *gdbarch, frame_info_ptr next_frame);
extern void set_gdbarch_unwind_pc (struct gdbarch *gdbarch, gdbarch_unwind_pc_ftype *unwind_pc);

typedef CORE_ADDR (gdbarch_unwind_sp_ftype) (struct gdbarch *gdbarch, frame_info_ptr next_frame);
extern CORE_ADDR gdbarch_unwind_sp (struct gdbarch *gdbarch, frame_info_ptr next_frame);
extern void set_gdbarch_unwind_sp (struct gdbarch *gdbarch, gdbarch_unwind_sp_ftype *unwind_sp);

/* DEPRECATED_FRAME_LOCALS_ADDRESS as been replaced by the per-frame
   frame-base.  Enable frame-base before frame-unwind. */

extern bool gdbarch_frame_num_args_p (struct gdbarch *gdbarch);

typedef int (gdbarch_frame_num_args_ftype) (frame_info_ptr frame);
extern int gdbarch_frame_num_args (struct gdbarch *gdbarch, frame_info_ptr frame);
extern void set_gdbarch_frame_num_args (struct gdbarch *gdbarch, gdbarch_frame_num_args_ftype *frame_num_args);

extern bool gdbarch_frame_align_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_frame_align_ftype) (struct gdbarch *gdbarch, CORE_ADDR address);
extern CORE_ADDR gdbarch_frame_align (struct gdbarch *gdbarch, CORE_ADDR address);
extern void set_gdbarch_frame_align (struct gdbarch *gdbarch, gdbarch_frame_align_ftype *frame_align);

typedef int (gdbarch_stabs_argument_has_addr_ftype) (struct gdbarch *gdbarch, struct type *type);
extern int gdbarch_stabs_argument_has_addr (struct gdbarch *gdbarch, struct type *type);
extern void set_gdbarch_stabs_argument_has_addr (struct gdbarch *gdbarch, gdbarch_stabs_argument_has_addr_ftype *stabs_argument_has_addr);

extern int gdbarch_frame_red_zone_size (struct gdbarch *gdbarch);
extern void set_gdbarch_frame_red_zone_size (struct gdbarch *gdbarch, int frame_red_zone_size);

typedef CORE_ADDR (gdbarch_convert_from_func_ptr_addr_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr, struct target_ops *targ);
extern CORE_ADDR gdbarch_convert_from_func_ptr_addr (struct gdbarch *gdbarch, CORE_ADDR addr, struct target_ops *targ);
extern void set_gdbarch_convert_from_func_ptr_addr (struct gdbarch *gdbarch, gdbarch_convert_from_func_ptr_addr_ftype *convert_from_func_ptr_addr);

/* On some machines there are bits in addresses which are not really
   part of the address, but are used by the kernel, the hardware, etc.
   for special purposes.  gdbarch_addr_bits_remove takes out any such bits so
   we get a "real" address such as one would find in a symbol table.
   This is used only for addresses of instructions, and even then I'm
   not sure it's used in all contexts.  It exists to deal with there
   being a few stray bits in the PC which would mislead us, not as some
   sort of generic thing to handle alignment or segmentation (it's
   possible it should be in TARGET_READ_PC instead). */

typedef CORE_ADDR (gdbarch_addr_bits_remove_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern CORE_ADDR gdbarch_addr_bits_remove (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_addr_bits_remove (struct gdbarch *gdbarch, gdbarch_addr_bits_remove_ftype *addr_bits_remove);

/* On some architectures, not all bits of a pointer are significant.
   On AArch64, for example, the top bits of a pointer may carry a "tag", which
   can be ignored by the kernel and the hardware.  The "tag" can be regarded as
   additional data associated with the pointer, but it is not part of the address.

   Given a pointer for the architecture, this hook removes all the
   non-significant bits and sign-extends things as needed.  It gets used to remove
   non-address bits from data pointers (for example, removing the AArch64 MTE tag
   bits from a pointer) and from code pointers (removing the AArch64 PAC signature
   from a pointer containing the return address). */

typedef CORE_ADDR (gdbarch_remove_non_address_bits_ftype) (struct gdbarch *gdbarch, CORE_ADDR pointer);
extern CORE_ADDR gdbarch_remove_non_address_bits (struct gdbarch *gdbarch, CORE_ADDR pointer);
extern void set_gdbarch_remove_non_address_bits (struct gdbarch *gdbarch, gdbarch_remove_non_address_bits_ftype *remove_non_address_bits);

/* Return a string representation of the memory tag TAG. */

typedef std::string (gdbarch_memtag_to_string_ftype) (struct gdbarch *gdbarch, struct value *tag);
extern std::string gdbarch_memtag_to_string (struct gdbarch *gdbarch, struct value *tag);
extern void set_gdbarch_memtag_to_string (struct gdbarch *gdbarch, gdbarch_memtag_to_string_ftype *memtag_to_string);

/* Return true if ADDRESS contains a tag and false otherwise.  ADDRESS
   must be either a pointer or a reference type. */

typedef bool (gdbarch_tagged_address_p_ftype) (struct gdbarch *gdbarch, struct value *address);
extern bool gdbarch_tagged_address_p (struct gdbarch *gdbarch, struct value *address);
extern void set_gdbarch_tagged_address_p (struct gdbarch *gdbarch, gdbarch_tagged_address_p_ftype *tagged_address_p);

/* Return true if the tag from ADDRESS matches the memory tag for that
   particular address.  Return false otherwise. */

typedef bool (gdbarch_memtag_matches_p_ftype) (struct gdbarch *gdbarch, struct value *address);
extern bool gdbarch_memtag_matches_p (struct gdbarch *gdbarch, struct value *address);
extern void set_gdbarch_memtag_matches_p (struct gdbarch *gdbarch, gdbarch_memtag_matches_p_ftype *memtag_matches_p);

/* Set the tags of type TAG_TYPE, for the memory address range
   [ADDRESS, ADDRESS + LENGTH) to TAGS.
   Return true if successful and false otherwise. */

typedef bool (gdbarch_set_memtags_ftype) (struct gdbarch *gdbarch, struct value *address, size_t length, const gdb::byte_vector &tags, memtag_type tag_type);
extern bool gdbarch_set_memtags (struct gdbarch *gdbarch, struct value *address, size_t length, const gdb::byte_vector &tags, memtag_type tag_type);
extern void set_gdbarch_set_memtags (struct gdbarch *gdbarch, gdbarch_set_memtags_ftype *set_memtags);

/* Return the tag of type TAG_TYPE associated with the memory address ADDRESS,
   assuming ADDRESS is tagged. */

typedef struct value * (gdbarch_get_memtag_ftype) (struct gdbarch *gdbarch, struct value *address, memtag_type tag_type);
extern struct value * gdbarch_get_memtag (struct gdbarch *gdbarch, struct value *address, memtag_type tag_type);
extern void set_gdbarch_get_memtag (struct gdbarch *gdbarch, gdbarch_get_memtag_ftype *get_memtag);

/* memtag_granule_size is the size of the allocation tag granule, for
   architectures that support memory tagging.
   This is 0 for architectures that do not support memory tagging.
   For a non-zero value, this represents the number of bytes of memory per tag. */

extern CORE_ADDR gdbarch_memtag_granule_size (struct gdbarch *gdbarch);
extern void set_gdbarch_memtag_granule_size (struct gdbarch *gdbarch, CORE_ADDR memtag_granule_size);

/* FIXME/cagney/2001-01-18: This should be split in two.  A target method that
   indicates if the target needs software single step.  An ISA method to
   implement it.

   FIXME/cagney/2001-01-18: The logic is backwards.  It should be asking if the
   target can single step.  If not, then implement single step using breakpoints.

   Return a vector of addresses on which the software single step
   breakpoints should be inserted.  NULL means software single step is
   not used.
   Multiple breakpoints may be inserted for some instructions such as
   conditional branch.  However, each implementation must always evaluate
   the condition and only put the breakpoint at the branch destination if
   the condition is true, so that we ensure forward progress when stepping
   past a conditional branch to self. */

extern bool gdbarch_software_single_step_p (struct gdbarch *gdbarch);

typedef std::vector<CORE_ADDR> (gdbarch_software_single_step_ftype) (struct regcache *regcache);
extern std::vector<CORE_ADDR> gdbarch_software_single_step (struct gdbarch *gdbarch, struct regcache *regcache);
extern void set_gdbarch_software_single_step (struct gdbarch *gdbarch, gdbarch_software_single_step_ftype *software_single_step);

/* Return non-zero if the processor is executing a delay slot and a
   further single-step is needed before the instruction finishes. */

extern bool gdbarch_single_step_through_delay_p (struct gdbarch *gdbarch);

typedef int (gdbarch_single_step_through_delay_ftype) (struct gdbarch *gdbarch, frame_info_ptr frame);
extern int gdbarch_single_step_through_delay (struct gdbarch *gdbarch, frame_info_ptr frame);
extern void set_gdbarch_single_step_through_delay (struct gdbarch *gdbarch, gdbarch_single_step_through_delay_ftype *single_step_through_delay);

/* FIXME: cagney/2003-08-28: Need to find a better way of selecting the
   disassembler.  Perhaps objdump can handle it? */

typedef int (gdbarch_print_insn_ftype) (bfd_vma vma, struct disassemble_info *info);
extern int gdbarch_print_insn (struct gdbarch *gdbarch, bfd_vma vma, struct disassemble_info *info);
extern void set_gdbarch_print_insn (struct gdbarch *gdbarch, gdbarch_print_insn_ftype *print_insn);

typedef CORE_ADDR (gdbarch_skip_trampoline_code_ftype) (frame_info_ptr frame, CORE_ADDR pc);
extern CORE_ADDR gdbarch_skip_trampoline_code (struct gdbarch *gdbarch, frame_info_ptr frame, CORE_ADDR pc);
extern void set_gdbarch_skip_trampoline_code (struct gdbarch *gdbarch, gdbarch_skip_trampoline_code_ftype *skip_trampoline_code);

/* Vtable of solib operations functions. */

extern const struct target_so_ops * gdbarch_so_ops (struct gdbarch *gdbarch);
extern void set_gdbarch_so_ops (struct gdbarch *gdbarch, const struct target_so_ops * so_ops);

/* If in_solib_dynsym_resolve_code() returns true, and SKIP_SOLIB_RESOLVER
   evaluates non-zero, this is the address where the debugger will place
   a step-resume breakpoint to get us past the dynamic linker. */

typedef CORE_ADDR (gdbarch_skip_solib_resolver_ftype) (struct gdbarch *gdbarch, CORE_ADDR pc);
extern CORE_ADDR gdbarch_skip_solib_resolver (struct gdbarch *gdbarch, CORE_ADDR pc);
extern void set_gdbarch_skip_solib_resolver (struct gdbarch *gdbarch, gdbarch_skip_solib_resolver_ftype *skip_solib_resolver);

/* Some systems also have trampoline code for returning from shared libs. */

typedef int (gdbarch_in_solib_return_trampoline_ftype) (struct gdbarch *gdbarch, CORE_ADDR pc, const char *name);
extern int gdbarch_in_solib_return_trampoline (struct gdbarch *gdbarch, CORE_ADDR pc, const char *name);
extern void set_gdbarch_in_solib_return_trampoline (struct gdbarch *gdbarch, gdbarch_in_solib_return_trampoline_ftype *in_solib_return_trampoline);

/* Return true if PC lies inside an indirect branch thunk. */

typedef bool (gdbarch_in_indirect_branch_thunk_ftype) (struct gdbarch *gdbarch, CORE_ADDR pc);
extern bool gdbarch_in_indirect_branch_thunk (struct gdbarch *gdbarch, CORE_ADDR pc);
extern void set_gdbarch_in_indirect_branch_thunk (struct gdbarch *gdbarch, gdbarch_in_indirect_branch_thunk_ftype *in_indirect_branch_thunk);

/* A target might have problems with watchpoints as soon as the stack
   frame of the current function has been destroyed.  This mostly happens
   as the first action in a function's epilogue.  stack_frame_destroyed_p()
   is defined to return a non-zero value if either the given addr is one
   instruction after the stack destroying instruction up to the trailing
   return instruction or if we can figure out that the stack frame has
   already been invalidated regardless of the value of addr.  Targets
   which don't suffer from that problem could just let this functionality
   untouched. */

typedef int (gdbarch_stack_frame_destroyed_p_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern int gdbarch_stack_frame_destroyed_p (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_stack_frame_destroyed_p (struct gdbarch *gdbarch, gdbarch_stack_frame_destroyed_p_ftype *stack_frame_destroyed_p);

/* Process an ELF symbol in the minimal symbol table in a backend-specific
   way.  Normally this hook is supposed to do nothing, however if required,
   then this hook can be used to apply tranformations to symbols that are
   considered special in some way.  For example the MIPS backend uses it
   to interpret `st_other' information to mark compressed code symbols so
   that they can be treated in the appropriate manner in the processing of
   the main symbol table and DWARF-2 records. */

extern bool gdbarch_elf_make_msymbol_special_p (struct gdbarch *gdbarch);

typedef void (gdbarch_elf_make_msymbol_special_ftype) (asymbol *sym, struct minimal_symbol *msym);
extern void gdbarch_elf_make_msymbol_special (struct gdbarch *gdbarch, asymbol *sym, struct minimal_symbol *msym);
extern void set_gdbarch_elf_make_msymbol_special (struct gdbarch *gdbarch, gdbarch_elf_make_msymbol_special_ftype *elf_make_msymbol_special);

typedef void (gdbarch_coff_make_msymbol_special_ftype) (int val, struct minimal_symbol *msym);
extern void gdbarch_coff_make_msymbol_special (struct gdbarch *gdbarch, int val, struct minimal_symbol *msym);
extern void set_gdbarch_coff_make_msymbol_special (struct gdbarch *gdbarch, gdbarch_coff_make_msymbol_special_ftype *coff_make_msymbol_special);

/* Process a symbol in the main symbol table in a backend-specific way.
   Normally this hook is supposed to do nothing, however if required,
   then this hook can be used to apply tranformations to symbols that
   are considered special in some way.  This is currently used by the
   MIPS backend to make sure compressed code symbols have the ISA bit
   set.  This in turn is needed for symbol values seen in GDB to match
   the values used at the runtime by the program itself, for function
   and label references. */

typedef void (gdbarch_make_symbol_special_ftype) (struct symbol *sym, struct objfile *objfile);
extern void gdbarch_make_symbol_special (struct gdbarch *gdbarch, struct symbol *sym, struct objfile *objfile);
extern void set_gdbarch_make_symbol_special (struct gdbarch *gdbarch, gdbarch_make_symbol_special_ftype *make_symbol_special);

/* Adjust the address retrieved from a DWARF-2 record other than a line
   entry in a backend-specific way.  Normally this hook is supposed to
   return the address passed unchanged, however if that is incorrect for
   any reason, then this hook can be used to fix the address up in the
   required manner.  This is currently used by the MIPS backend to make
   sure addresses in FDE, range records, etc. referring to compressed
   code have the ISA bit set, matching line information and the symbol
   table. */

typedef CORE_ADDR (gdbarch_adjust_dwarf2_addr_ftype) (CORE_ADDR pc);
extern CORE_ADDR gdbarch_adjust_dwarf2_addr (struct gdbarch *gdbarch, CORE_ADDR pc);
extern void set_gdbarch_adjust_dwarf2_addr (struct gdbarch *gdbarch, gdbarch_adjust_dwarf2_addr_ftype *adjust_dwarf2_addr);

/* Adjust the address updated by a line entry in a backend-specific way.
   Normally this hook is supposed to return the address passed unchanged,
   however in the case of inconsistencies in these records, this hook can
   be used to fix them up in the required manner.  This is currently used
   by the MIPS backend to make sure all line addresses in compressed code
   are presented with the ISA bit set, which is not always the case.  This
   in turn ensures breakpoint addresses are correctly matched against the
   stop PC. */

typedef CORE_ADDR (gdbarch_adjust_dwarf2_line_ftype) (CORE_ADDR addr, int rel);
extern CORE_ADDR gdbarch_adjust_dwarf2_line (struct gdbarch *gdbarch, CORE_ADDR addr, int rel);
extern void set_gdbarch_adjust_dwarf2_line (struct gdbarch *gdbarch, gdbarch_adjust_dwarf2_line_ftype *adjust_dwarf2_line);

extern int gdbarch_cannot_step_breakpoint (struct gdbarch *gdbarch);
extern void set_gdbarch_cannot_step_breakpoint (struct gdbarch *gdbarch, int cannot_step_breakpoint);

/* See comment in target.h about continuable, steppable and
   non-steppable watchpoints. */

extern int gdbarch_have_nonsteppable_watchpoint (struct gdbarch *gdbarch);
extern void set_gdbarch_have_nonsteppable_watchpoint (struct gdbarch *gdbarch, int have_nonsteppable_watchpoint);

extern bool gdbarch_address_class_type_flags_p (struct gdbarch *gdbarch);

typedef type_instance_flags (gdbarch_address_class_type_flags_ftype) (int byte_size, int dwarf2_addr_class);
extern type_instance_flags gdbarch_address_class_type_flags (struct gdbarch *gdbarch, int byte_size, int dwarf2_addr_class);
extern void set_gdbarch_address_class_type_flags (struct gdbarch *gdbarch, gdbarch_address_class_type_flags_ftype *address_class_type_flags);

extern bool gdbarch_address_class_type_flags_to_name_p (struct gdbarch *gdbarch);

typedef const char * (gdbarch_address_class_type_flags_to_name_ftype) (struct gdbarch *gdbarch, type_instance_flags type_flags);
extern const char * gdbarch_address_class_type_flags_to_name (struct gdbarch *gdbarch, type_instance_flags type_flags);
extern void set_gdbarch_address_class_type_flags_to_name (struct gdbarch *gdbarch, gdbarch_address_class_type_flags_to_name_ftype *address_class_type_flags_to_name);

/* Execute vendor-specific DWARF Call Frame Instruction.  OP is the instruction.
   FS are passed from the generic execute_cfa_program function. */

typedef bool (gdbarch_execute_dwarf_cfa_vendor_op_ftype) (struct gdbarch *gdbarch, gdb_byte op, struct dwarf2_frame_state *fs);
extern bool gdbarch_execute_dwarf_cfa_vendor_op (struct gdbarch *gdbarch, gdb_byte op, struct dwarf2_frame_state *fs);
extern void set_gdbarch_execute_dwarf_cfa_vendor_op (struct gdbarch *gdbarch, gdbarch_execute_dwarf_cfa_vendor_op_ftype *execute_dwarf_cfa_vendor_op);

/* Return the appropriate type_flags for the supplied address class.
   This function should return true if the address class was recognized and
   type_flags was set, false otherwise. */

extern bool gdbarch_address_class_name_to_type_flags_p (struct gdbarch *gdbarch);

typedef bool (gdbarch_address_class_name_to_type_flags_ftype) (struct gdbarch *gdbarch, const char *name, type_instance_flags *type_flags_ptr);
extern bool gdbarch_address_class_name_to_type_flags (struct gdbarch *gdbarch, const char *name, type_instance_flags *type_flags_ptr);
extern void set_gdbarch_address_class_name_to_type_flags (struct gdbarch *gdbarch, gdbarch_address_class_name_to_type_flags_ftype *address_class_name_to_type_flags);

/* Is a register in a group */

typedef int (gdbarch_register_reggroup_p_ftype) (struct gdbarch *gdbarch, int regnum, const struct reggroup *reggroup);
extern int gdbarch_register_reggroup_p (struct gdbarch *gdbarch, int regnum, const struct reggroup *reggroup);
extern void set_gdbarch_register_reggroup_p (struct gdbarch *gdbarch, gdbarch_register_reggroup_p_ftype *register_reggroup_p);

/* Fetch the pointer to the ith function argument. */

extern bool gdbarch_fetch_pointer_argument_p (struct gdbarch *gdbarch);

typedef CORE_ADDR (gdbarch_fetch_pointer_argument_ftype) (frame_info_ptr frame, int argi, struct type *type);
extern CORE_ADDR gdbarch_fetch_pointer_argument (struct gdbarch *gdbarch, frame_info_ptr frame, int argi, struct type *type);
extern void set_gdbarch_fetch_pointer_argument (struct gdbarch *gdbarch, gdbarch_fetch_pointer_argument_ftype *fetch_pointer_argument);

/* Iterate over all supported register notes in a core file.  For each
   supported register note section, the iterator must call CB and pass
   CB_DATA unchanged.  If REGCACHE is not NULL, the iterator can limit
   the supported register note sections based on the current register
   values.  Otherwise it should enumerate all supported register note
   sections. */

extern bool gdbarch_iterate_over_regset_sections_p (struct gdbarch *gdbarch);

typedef void (gdbarch_iterate_over_regset_sections_ftype) (struct gdbarch *gdbarch, iterate_over_regset_sections_cb *cb, void *cb_data, const struct regcache *regcache);
extern void gdbarch_iterate_over_regset_sections (struct gdbarch *gdbarch, iterate_over_regset_sections_cb *cb, void *cb_data, const struct regcache *regcache);
extern void set_gdbarch_iterate_over_regset_sections (struct gdbarch *gdbarch, gdbarch_iterate_over_regset_sections_ftype *iterate_over_regset_sections);

/* Create core file notes */

extern bool gdbarch_make_corefile_notes_p (struct gdbarch *gdbarch);

typedef gdb::unique_xmalloc_ptr<char> (gdbarch_make_corefile_notes_ftype) (struct gdbarch *gdbarch, bfd *obfd, int *note_size);
extern gdb::unique_xmalloc_ptr<char> gdbarch_make_corefile_notes (struct gdbarch *gdbarch, bfd *obfd, int *note_size);
extern void set_gdbarch_make_corefile_notes (struct gdbarch *gdbarch, gdbarch_make_corefile_notes_ftype *make_corefile_notes);

/* Find core file memory regions */

extern bool gdbarch_find_memory_regions_p (struct gdbarch *gdbarch);

typedef int (gdbarch_find_memory_regions_ftype) (struct gdbarch *gdbarch, find_memory_region_ftype func, void *data);
extern int gdbarch_find_memory_regions (struct gdbarch *gdbarch, find_memory_region_ftype func, void *data);
extern void set_gdbarch_find_memory_regions (struct gdbarch *gdbarch, gdbarch_find_memory_regions_ftype *find_memory_regions);

/* Given a bfd OBFD, segment ADDRESS and SIZE, create a memory tag section to be dumped to a core file */

extern bool gdbarch_create_memtag_section_p (struct gdbarch *gdbarch);

typedef asection * (gdbarch_create_memtag_section_ftype) (struct gdbarch *gdbarch, bfd *obfd, CORE_ADDR address, size_t size);
extern asection * gdbarch_create_memtag_section (struct gdbarch *gdbarch, bfd *obfd, CORE_ADDR address, size_t size);
extern void set_gdbarch_create_memtag_section (struct gdbarch *gdbarch, gdbarch_create_memtag_section_ftype *create_memtag_section);

/* Given a memory tag section OSEC, fill OSEC's contents with the appropriate tag data */

extern bool gdbarch_fill_memtag_section_p (struct gdbarch *gdbarch);

typedef bool (gdbarch_fill_memtag_section_ftype) (struct gdbarch *gdbarch, asection *osec);
extern bool gdbarch_fill_memtag_section (struct gdbarch *gdbarch, asection *osec);
extern void set_gdbarch_fill_memtag_section (struct gdbarch *gdbarch, gdbarch_fill_memtag_section_ftype *fill_memtag_section);

/* Decode a memory tag SECTION and return the tags of type TYPE contained in
   the memory range [ADDRESS, ADDRESS + LENGTH).
   If no tags were found, return an empty vector. */

extern bool gdbarch_decode_memtag_section_p (struct gdbarch *gdbarch);

typedef gdb::byte_vector (gdbarch_decode_memtag_section_ftype) (struct gdbarch *gdbarch, bfd_section *section, int type, CORE_ADDR address, size_t length);
extern gdb::byte_vector gdbarch_decode_memtag_section (struct gdbarch *gdbarch, bfd_section *section, int type, CORE_ADDR address, size_t length);
extern void set_gdbarch_decode_memtag_section (struct gdbarch *gdbarch, gdbarch_decode_memtag_section_ftype *decode_memtag_section);

/* Read offset OFFSET of TARGET_OBJECT_LIBRARIES formatted shared libraries list from
   core file into buffer READBUF with length LEN.  Return the number of bytes read
   (zero indicates failure).
   failed, otherwise, return the red length of READBUF. */

extern bool gdbarch_core_xfer_shared_libraries_p (struct gdbarch *gdbarch);

typedef ULONGEST (gdbarch_core_xfer_shared_libraries_ftype) (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len);
extern ULONGEST gdbarch_core_xfer_shared_libraries (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len);
extern void set_gdbarch_core_xfer_shared_libraries (struct gdbarch *gdbarch, gdbarch_core_xfer_shared_libraries_ftype *core_xfer_shared_libraries);

/* Read offset OFFSET of TARGET_OBJECT_LIBRARIES_AIX formatted shared
   libraries list from core file into buffer READBUF with length LEN.
   Return the number of bytes read (zero indicates failure). */

extern bool gdbarch_core_xfer_shared_libraries_aix_p (struct gdbarch *gdbarch);

typedef ULONGEST (gdbarch_core_xfer_shared_libraries_aix_ftype) (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len);
extern ULONGEST gdbarch_core_xfer_shared_libraries_aix (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len);
extern void set_gdbarch_core_xfer_shared_libraries_aix (struct gdbarch *gdbarch, gdbarch_core_xfer_shared_libraries_aix_ftype *core_xfer_shared_libraries_aix);

/* How the core target converts a PTID from a core file to a string. */

extern bool gdbarch_core_pid_to_str_p (struct gdbarch *gdbarch);

typedef std::string (gdbarch_core_pid_to_str_ftype) (struct gdbarch *gdbarch, ptid_t ptid);
extern std::string gdbarch_core_pid_to_str (struct gdbarch *gdbarch, ptid_t ptid);
extern void set_gdbarch_core_pid_to_str (struct gdbarch *gdbarch, gdbarch_core_pid_to_str_ftype *core_pid_to_str);

/* How the core target extracts the name of a thread from a core file. */

extern bool gdbarch_core_thread_name_p (struct gdbarch *gdbarch);

typedef const char * (gdbarch_core_thread_name_ftype) (struct gdbarch *gdbarch, struct thread_info *thr);
extern const char * gdbarch_core_thread_name (struct gdbarch *gdbarch, struct thread_info *thr);
extern void set_gdbarch_core_thread_name (struct gdbarch *gdbarch, gdbarch_core_thread_name_ftype *core_thread_name);

/* Read offset OFFSET of TARGET_OBJECT_SIGNAL_INFO signal information
   from core file into buffer READBUF with length LEN.  Return the number
   of bytes read (zero indicates EOF, a negative value indicates failure). */

extern bool gdbarch_core_xfer_siginfo_p (struct gdbarch *gdbarch);

typedef LONGEST (gdbarch_core_xfer_siginfo_ftype) (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len);
extern LONGEST gdbarch_core_xfer_siginfo (struct gdbarch *gdbarch, gdb_byte *readbuf, ULONGEST offset, ULONGEST len);
extern void set_gdbarch_core_xfer_siginfo (struct gdbarch *gdbarch, gdbarch_core_xfer_siginfo_ftype *core_xfer_siginfo);

/* Read x86 XSAVE layout information from core file into XSAVE_LAYOUT.
   Returns true if the layout was read successfully. */

extern bool gdbarch_core_read_x86_xsave_layout_p (struct gdbarch *gdbarch);

typedef bool (gdbarch_core_read_x86_xsave_layout_ftype) (struct gdbarch *gdbarch, x86_xsave_layout &xsave_layout);
extern bool gdbarch_core_read_x86_xsave_layout (struct gdbarch *gdbarch, x86_xsave_layout &xsave_layout);
extern void set_gdbarch_core_read_x86_xsave_layout (struct gdbarch *gdbarch, gdbarch_core_read_x86_xsave_layout_ftype *core_read_x86_xsave_layout);

/* BFD target to use when generating a core file. */

extern bool gdbarch_gcore_bfd_target_p (struct gdbarch *gdbarch);

extern const char * gdbarch_gcore_bfd_target (struct gdbarch *gdbarch);
extern void set_gdbarch_gcore_bfd_target (struct gdbarch *gdbarch, const char * gcore_bfd_target);

/* If the elements of C++ vtables are in-place function descriptors rather
   than normal function pointers (which may point to code or a descriptor),
   set this to one. */

extern int gdbarch_vtable_function_descriptors (struct gdbarch *gdbarch);
extern void set_gdbarch_vtable_function_descriptors (struct gdbarch *gdbarch, int vtable_function_descriptors);

/* Set if the least significant bit of the delta is used instead of the least
   significant bit of the pfn for pointers to virtual member functions. */

extern int gdbarch_vbit_in_delta (struct gdbarch *gdbarch);
extern void set_gdbarch_vbit_in_delta (struct gdbarch *gdbarch, int vbit_in_delta);

/* Advance PC to next instruction in order to skip a permanent breakpoint. */

typedef void (gdbarch_skip_permanent_breakpoint_ftype) (struct regcache *regcache);
extern void gdbarch_skip_permanent_breakpoint (struct gdbarch *gdbarch, struct regcache *regcache);
extern void set_gdbarch_skip_permanent_breakpoint (struct gdbarch *gdbarch, gdbarch_skip_permanent_breakpoint_ftype *skip_permanent_breakpoint);

/* The maximum length of an instruction on this architecture in bytes. */

extern bool gdbarch_max_insn_length_p (struct gdbarch *gdbarch);

extern ULONGEST gdbarch_max_insn_length (struct gdbarch *gdbarch);
extern void set_gdbarch_max_insn_length (struct gdbarch *gdbarch, ULONGEST max_insn_length);

/* Copy the instruction at FROM to TO, and make any adjustments
   necessary to single-step it at that address.

   REGS holds the state the thread's registers will have before
   executing the copied instruction; the PC in REGS will refer to FROM,
   not the copy at TO.  The caller should update it to point at TO later.

   Return a pointer to data of the architecture's choice to be passed
   to gdbarch_displaced_step_fixup.

   For a general explanation of displaced stepping and how GDB uses it,
   see the comments in infrun.c.

   The TO area is only guaranteed to have space for
   gdbarch_displaced_step_buffer_length (arch) octets, so this
   function must not write more octets than that to this area.

   If you do not provide this function, GDB assumes that the
   architecture does not support displaced stepping.

   If the instruction cannot execute out of line, return NULL.  The
   core falls back to stepping past the instruction in-line instead in
   that case. */

extern bool gdbarch_displaced_step_copy_insn_p (struct gdbarch *gdbarch);

typedef displaced_step_copy_insn_closure_up (gdbarch_displaced_step_copy_insn_ftype) (struct gdbarch *gdbarch, CORE_ADDR from, CORE_ADDR to, struct regcache *regs);
extern displaced_step_copy_insn_closure_up gdbarch_displaced_step_copy_insn (struct gdbarch *gdbarch, CORE_ADDR from, CORE_ADDR to, struct regcache *regs);
extern void set_gdbarch_displaced_step_copy_insn (struct gdbarch *gdbarch, gdbarch_displaced_step_copy_insn_ftype *displaced_step_copy_insn);

/* Return true if GDB should use hardware single-stepping to execute a displaced
   step instruction.  If false, GDB will simply restart execution at the
   displaced instruction location, and it is up to the target to ensure GDB will
   receive control again (e.g. by placing a software breakpoint instruction into
   the displaced instruction buffer).

   The default implementation returns false on all targets that provide a
   gdbarch_software_single_step routine, and true otherwise. */

typedef bool (gdbarch_displaced_step_hw_singlestep_ftype) (struct gdbarch *gdbarch);
extern bool gdbarch_displaced_step_hw_singlestep (struct gdbarch *gdbarch);
extern void set_gdbarch_displaced_step_hw_singlestep (struct gdbarch *gdbarch, gdbarch_displaced_step_hw_singlestep_ftype *displaced_step_hw_singlestep);

/* Fix up the state after attempting to single-step a displaced
   instruction, to give the result we would have gotten from stepping the
   instruction in its original location.

   REGS is the register state resulting from single-stepping the
   displaced instruction.

   CLOSURE is the result from the matching call to
   gdbarch_displaced_step_copy_insn.

   FROM is the address where the instruction was original located, TO is
   the address of the displaced buffer where the instruction was copied
   to for stepping.

   COMPLETED_P is true if GDB stopped as a result of the requested step
   having completed (e.g. the inferior stopped with SIGTRAP), otherwise
   COMPLETED_P is false and GDB stopped for some other reason.  In the
   case where a single instruction is expanded to multiple replacement
   instructions for stepping then it may be necessary to read the current
   program counter from REGS in order to decide how far through the
   series of replacement instructions the inferior got before stopping,
   this may impact what will need fixing up in this function.

   For a general explanation of displaced stepping and how GDB uses it,
   see the comments in infrun.c. */

typedef void (gdbarch_displaced_step_fixup_ftype) (struct gdbarch *gdbarch, struct displaced_step_copy_insn_closure *closure, CORE_ADDR from, CORE_ADDR to, struct regcache *regs, bool completed_p);
extern void gdbarch_displaced_step_fixup (struct gdbarch *gdbarch, struct displaced_step_copy_insn_closure *closure, CORE_ADDR from, CORE_ADDR to, struct regcache *regs, bool completed_p);
extern void set_gdbarch_displaced_step_fixup (struct gdbarch *gdbarch, gdbarch_displaced_step_fixup_ftype *displaced_step_fixup);

/* Prepare THREAD for it to displaced step the instruction at its current PC.

   Throw an exception if any unexpected error happens. */

extern bool gdbarch_displaced_step_prepare_p (struct gdbarch *gdbarch);

typedef displaced_step_prepare_status (gdbarch_displaced_step_prepare_ftype) (struct gdbarch *gdbarch, thread_info *thread, CORE_ADDR &displaced_pc);
extern displaced_step_prepare_status gdbarch_displaced_step_prepare (struct gdbarch *gdbarch, thread_info *thread, CORE_ADDR &displaced_pc);
extern void set_gdbarch_displaced_step_prepare (struct gdbarch *gdbarch, gdbarch_displaced_step_prepare_ftype *displaced_step_prepare);

/* Clean up after a displaced step of THREAD.

   It is possible for the displaced-stepped instruction to have caused
   the thread to exit.  The implementation can detect this case by
   checking if WS.kind is TARGET_WAITKIND_THREAD_EXITED. */

typedef displaced_step_finish_status (gdbarch_displaced_step_finish_ftype) (struct gdbarch *gdbarch, thread_info *thread, const target_waitstatus &ws);
extern displaced_step_finish_status gdbarch_displaced_step_finish (struct gdbarch *gdbarch, thread_info *thread, const target_waitstatus &ws);
extern void set_gdbarch_displaced_step_finish (struct gdbarch *gdbarch, gdbarch_displaced_step_finish_ftype *displaced_step_finish);

/* Return the closure associated to the displaced step buffer that is at ADDR. */

extern bool gdbarch_displaced_step_copy_insn_closure_by_addr_p (struct gdbarch *gdbarch);

typedef const displaced_step_copy_insn_closure * (gdbarch_displaced_step_copy_insn_closure_by_addr_ftype) (inferior *inf, CORE_ADDR addr);
extern const displaced_step_copy_insn_closure * gdbarch_displaced_step_copy_insn_closure_by_addr (struct gdbarch *gdbarch, inferior *inf, CORE_ADDR addr);
extern void set_gdbarch_displaced_step_copy_insn_closure_by_addr (struct gdbarch *gdbarch, gdbarch_displaced_step_copy_insn_closure_by_addr_ftype *displaced_step_copy_insn_closure_by_addr);

/* PARENT_INF has forked and CHILD_PTID is the ptid of the child.  Restore the
   contents of all displaced step buffers in the child's address space. */

typedef void (gdbarch_displaced_step_restore_all_in_ptid_ftype) (inferior *parent_inf, ptid_t child_ptid);
extern void gdbarch_displaced_step_restore_all_in_ptid (struct gdbarch *gdbarch, inferior *parent_inf, ptid_t child_ptid);
extern void set_gdbarch_displaced_step_restore_all_in_ptid (struct gdbarch *gdbarch, gdbarch_displaced_step_restore_all_in_ptid_ftype *displaced_step_restore_all_in_ptid);

/* The maximum length in octets required for a displaced-step instruction
   buffer.  By default this will be the same as gdbarch::max_insn_length,
   but should be overridden for architectures that might expand a
   displaced-step instruction to multiple replacement instructions. */

extern ULONGEST gdbarch_displaced_step_buffer_length (struct gdbarch *gdbarch);
extern void set_gdbarch_displaced_step_buffer_length (struct gdbarch *gdbarch, ULONGEST displaced_step_buffer_length);

/* Relocate an instruction to execute at a different address.  OLDLOC
   is the address in the inferior memory where the instruction to
   relocate is currently at.  On input, TO points to the destination
   where we want the instruction to be copied (and possibly adjusted)
   to.  On output, it points to one past the end of the resulting
   instruction(s).  The effect of executing the instruction at TO shall
   be the same as if executing it at FROM.  For example, call
   instructions that implicitly push the return address on the stack
   should be adjusted to return to the instruction after OLDLOC;
   relative branches, and other PC-relative instructions need the
   offset adjusted; etc. */

extern bool gdbarch_relocate_instruction_p (struct gdbarch *gdbarch);

typedef void (gdbarch_relocate_instruction_ftype) (struct gdbarch *gdbarch, CORE_ADDR *to, CORE_ADDR from);
extern void gdbarch_relocate_instruction (struct gdbarch *gdbarch, CORE_ADDR *to, CORE_ADDR from);
extern void set_gdbarch_relocate_instruction (struct gdbarch *gdbarch, gdbarch_relocate_instruction_ftype *relocate_instruction);

/* Refresh overlay mapped state for section OSECT. */

extern bool gdbarch_overlay_update_p (struct gdbarch *gdbarch);

typedef void (gdbarch_overlay_update_ftype) (struct obj_section *osect);
extern void gdbarch_overlay_update (struct gdbarch *gdbarch, struct obj_section *osect);
extern void set_gdbarch_overlay_update (struct gdbarch *gdbarch, gdbarch_overlay_update_ftype *overlay_update);

extern bool gdbarch_core_read_description_p (struct gdbarch *gdbarch);

typedef const struct target_desc * (gdbarch_core_read_description_ftype) (struct gdbarch *gdbarch, struct target_ops *target, bfd *abfd);
extern const struct target_desc * gdbarch_core_read_description (struct gdbarch *gdbarch, struct target_ops *target, bfd *abfd);
extern void set_gdbarch_core_read_description (struct gdbarch *gdbarch, gdbarch_core_read_description_ftype *core_read_description);

/* Set if the address in N_SO or N_FUN stabs may be zero. */

extern int gdbarch_sofun_address_maybe_missing (struct gdbarch *gdbarch);
extern void set_gdbarch_sofun_address_maybe_missing (struct gdbarch *gdbarch, int sofun_address_maybe_missing);

/* Parse the instruction at ADDR storing in the record execution log
   the registers REGCACHE and memory ranges that will be affected when
   the instruction executes, along with their current values.
   Return -1 if something goes wrong, 0 otherwise. */

extern bool gdbarch_process_record_p (struct gdbarch *gdbarch);

typedef int (gdbarch_process_record_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR addr);
extern int gdbarch_process_record (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR addr);
extern void set_gdbarch_process_record (struct gdbarch *gdbarch, gdbarch_process_record_ftype *process_record);

/* Save process state after a signal.
   Return -1 if something goes wrong, 0 otherwise. */

extern bool gdbarch_process_record_signal_p (struct gdbarch *gdbarch);

typedef int (gdbarch_process_record_signal_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, enum gdb_signal signal);
extern int gdbarch_process_record_signal (struct gdbarch *gdbarch, struct regcache *regcache, enum gdb_signal signal);
extern void set_gdbarch_process_record_signal (struct gdbarch *gdbarch, gdbarch_process_record_signal_ftype *process_record_signal);

/* Signal translation: translate inferior's signal (target's) number
   into GDB's representation.  The implementation of this method must
   be host independent.  IOW, don't rely on symbols of the NAT_FILE
   header (the nm-*.h files), the host <signal.h> header, or similar
   headers.  This is mainly used when cross-debugging core files ---
   "Live" targets hide the translation behind the target interface
   (target_wait, target_resume, etc.). */

extern bool gdbarch_gdb_signal_from_target_p (struct gdbarch *gdbarch);

typedef enum gdb_signal (gdbarch_gdb_signal_from_target_ftype) (struct gdbarch *gdbarch, int signo);
extern enum gdb_signal gdbarch_gdb_signal_from_target (struct gdbarch *gdbarch, int signo);
extern void set_gdbarch_gdb_signal_from_target (struct gdbarch *gdbarch, gdbarch_gdb_signal_from_target_ftype *gdb_signal_from_target);

/* Signal translation: translate the GDB's internal signal number into
   the inferior's signal (target's) representation.  The implementation
   of this method must be host independent.  IOW, don't rely on symbols
   of the NAT_FILE header (the nm-*.h files), the host <signal.h>
   header, or similar headers.
   Return the target signal number if found, or -1 if the GDB internal
   signal number is invalid. */

extern bool gdbarch_gdb_signal_to_target_p (struct gdbarch *gdbarch);

typedef int (gdbarch_gdb_signal_to_target_ftype) (struct gdbarch *gdbarch, enum gdb_signal signal);
extern int gdbarch_gdb_signal_to_target (struct gdbarch *gdbarch, enum gdb_signal signal);
extern void set_gdbarch_gdb_signal_to_target (struct gdbarch *gdbarch, gdbarch_gdb_signal_to_target_ftype *gdb_signal_to_target);

/* Extra signal info inspection.

   Return a type suitable to inspect extra signal information. */

extern bool gdbarch_get_siginfo_type_p (struct gdbarch *gdbarch);

typedef struct type * (gdbarch_get_siginfo_type_ftype) (struct gdbarch *gdbarch);
extern struct type * gdbarch_get_siginfo_type (struct gdbarch *gdbarch);
extern void set_gdbarch_get_siginfo_type (struct gdbarch *gdbarch, gdbarch_get_siginfo_type_ftype *get_siginfo_type);

/* Record architecture-specific information from the symbol table. */

extern bool gdbarch_record_special_symbol_p (struct gdbarch *gdbarch);

typedef void (gdbarch_record_special_symbol_ftype) (struct gdbarch *gdbarch, struct objfile *objfile, asymbol *sym);
extern void gdbarch_record_special_symbol (struct gdbarch *gdbarch, struct objfile *objfile, asymbol *sym);
extern void set_gdbarch_record_special_symbol (struct gdbarch *gdbarch, gdbarch_record_special_symbol_ftype *record_special_symbol);

/* Function for the 'catch syscall' feature.
   Get architecture-specific system calls information from registers. */

extern bool gdbarch_get_syscall_number_p (struct gdbarch *gdbarch);

typedef LONGEST (gdbarch_get_syscall_number_ftype) (struct gdbarch *gdbarch, thread_info *thread);
extern LONGEST gdbarch_get_syscall_number (struct gdbarch *gdbarch, thread_info *thread);
extern void set_gdbarch_get_syscall_number (struct gdbarch *gdbarch, gdbarch_get_syscall_number_ftype *get_syscall_number);

/* The filename of the XML syscall for this architecture. */

extern const char * gdbarch_xml_syscall_file (struct gdbarch *gdbarch);
extern void set_gdbarch_xml_syscall_file (struct gdbarch *gdbarch, const char * xml_syscall_file);

/* Information about system calls from this architecture */

extern struct syscalls_info * gdbarch_syscalls_info (struct gdbarch *gdbarch);
extern void set_gdbarch_syscalls_info (struct gdbarch *gdbarch, struct syscalls_info * syscalls_info);

/* SystemTap related fields and functions.
   A NULL-terminated array of prefixes used to mark an integer constant
   on the architecture's assembly.
   For example, on x86 integer constants are written as:

   $10 ;; integer constant 10

   in this case, this prefix would be the character `$'. */

extern const char *const * gdbarch_stap_integer_prefixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_integer_prefixes (struct gdbarch *gdbarch, const char *const * stap_integer_prefixes);

/* A NULL-terminated array of suffixes used to mark an integer constant
   on the architecture's assembly. */

extern const char *const * gdbarch_stap_integer_suffixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_integer_suffixes (struct gdbarch *gdbarch, const char *const * stap_integer_suffixes);

/* A NULL-terminated array of prefixes used to mark a register name on
   the architecture's assembly.
   For example, on x86 the register name is written as:

   %eax ;; register eax

   in this case, this prefix would be the character `%'. */

extern const char *const * gdbarch_stap_register_prefixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_register_prefixes (struct gdbarch *gdbarch, const char *const * stap_register_prefixes);

/* A NULL-terminated array of suffixes used to mark a register name on
   the architecture's assembly. */

extern const char *const * gdbarch_stap_register_suffixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_register_suffixes (struct gdbarch *gdbarch, const char *const * stap_register_suffixes);

/* A NULL-terminated array of prefixes used to mark a register
   indirection on the architecture's assembly.
   For example, on x86 the register indirection is written as:

   (%eax) ;; indirecting eax

   in this case, this prefix would be the charater `('.

   Please note that we use the indirection prefix also for register
   displacement, e.g., `4(%eax)' on x86. */

extern const char *const * gdbarch_stap_register_indirection_prefixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_register_indirection_prefixes (struct gdbarch *gdbarch, const char *const * stap_register_indirection_prefixes);

/* A NULL-terminated array of suffixes used to mark a register
   indirection on the architecture's assembly.
   For example, on x86 the register indirection is written as:

   (%eax) ;; indirecting eax

   in this case, this prefix would be the charater `)'.

   Please note that we use the indirection suffix also for register
   displacement, e.g., `4(%eax)' on x86. */

extern const char *const * gdbarch_stap_register_indirection_suffixes (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_register_indirection_suffixes (struct gdbarch *gdbarch, const char *const * stap_register_indirection_suffixes);

/* Prefix(es) used to name a register using GDB's nomenclature.

   For example, on PPC a register is represented by a number in the assembly
   language (e.g., `10' is the 10th general-purpose register).  However,
   inside GDB this same register has an `r' appended to its name, so the 10th
   register would be represented as `r10' internally. */

extern const char * gdbarch_stap_gdb_register_prefix (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_gdb_register_prefix (struct gdbarch *gdbarch, const char * stap_gdb_register_prefix);

/* Suffix used to name a register using GDB's nomenclature. */

extern const char * gdbarch_stap_gdb_register_suffix (struct gdbarch *gdbarch);
extern void set_gdbarch_stap_gdb_register_suffix (struct gdbarch *gdbarch, const char * stap_gdb_register_suffix);

/* Check if S is a single operand.

   Single operands can be:
   - Literal integers, e.g. `$10' on x86
   - Register access, e.g. `%eax' on x86
   - Register indirection, e.g. `(%eax)' on x86
   - Register displacement, e.g. `4(%eax)' on x86

   This function should check for these patterns on the string
   and return 1 if some were found, or zero otherwise.  Please try to match
   as much info as you can from the string, i.e., if you have to match
   something like `(%', do not match just the `('. */

extern bool gdbarch_stap_is_single_operand_p (struct gdbarch *gdbarch);

typedef int (gdbarch_stap_is_single_operand_ftype) (struct gdbarch *gdbarch, const char *s);
extern int gdbarch_stap_is_single_operand (struct gdbarch *gdbarch, const char *s);
extern void set_gdbarch_stap_is_single_operand (struct gdbarch *gdbarch, gdbarch_stap_is_single_operand_ftype *stap_is_single_operand);

/* Function used to handle a "special case" in the parser.

   A "special case" is considered to be an unknown token, i.e., a token
   that the parser does not know how to parse.  A good example of special
   case would be ARM's register displacement syntax:

   [R0, #4]  ;; displacing R0 by 4

   Since the parser assumes that a register displacement is of the form:

   <number> <indirection_prefix> <register_name> <indirection_suffix>

   it means that it will not be able to recognize and parse this odd syntax.
   Therefore, we should add a special case function that will handle this token.

   This function should generate the proper expression form of the expression
   using GDB's internal expression mechanism (e.g., `write_exp_elt_opcode'
   and so on).  It should also return 1 if the parsing was successful, or zero
   if the token was not recognized as a special token (in this case, returning
   zero means that the special parser is deferring the parsing to the generic
   parser), and should advance the buffer pointer (p->arg). */

extern bool gdbarch_stap_parse_special_token_p (struct gdbarch *gdbarch);

typedef expr::operation_up (gdbarch_stap_parse_special_token_ftype) (struct gdbarch *gdbarch, struct stap_parse_info *p);
extern expr::operation_up gdbarch_stap_parse_special_token (struct gdbarch *gdbarch, struct stap_parse_info *p);
extern void set_gdbarch_stap_parse_special_token (struct gdbarch *gdbarch, gdbarch_stap_parse_special_token_ftype *stap_parse_special_token);

/* Perform arch-dependent adjustments to a register name.

   In very specific situations, it may be necessary for the register
   name present in a SystemTap probe's argument to be handled in a
   special way.  For example, on i386, GCC may over-optimize the
   register allocation and use smaller registers than necessary.  In
   such cases, the client that is reading and evaluating the SystemTap
   probe (ourselves) will need to actually fetch values from the wider
   version of the register in question.

   To illustrate the example, consider the following probe argument
   (i386):

   4@%ax

   This argument says that its value can be found at the %ax register,
   which is a 16-bit register.  However, the argument's prefix says
   that its type is "uint32_t", which is 32-bit in size.  Therefore, in
   this case, GDB should actually fetch the probe's value from register
   %eax, not %ax.  In this scenario, this function would actually
   replace the register name from %ax to %eax.

   The rationale for this can be found at PR breakpoints/24541. */

extern bool gdbarch_stap_adjust_register_p (struct gdbarch *gdbarch);

typedef std::string (gdbarch_stap_adjust_register_ftype) (struct gdbarch *gdbarch, struct stap_parse_info *p, const std::string &regname, int regnum);
extern std::string gdbarch_stap_adjust_register (struct gdbarch *gdbarch, struct stap_parse_info *p, const std::string &regname, int regnum);
extern void set_gdbarch_stap_adjust_register (struct gdbarch *gdbarch, gdbarch_stap_adjust_register_ftype *stap_adjust_register);

/* DTrace related functions.
   The expression to compute the NARTGth+1 argument to a DTrace USDT probe.
   NARG must be >= 0. */

extern bool gdbarch_dtrace_parse_probe_argument_p (struct gdbarch *gdbarch);

typedef expr::operation_up (gdbarch_dtrace_parse_probe_argument_ftype) (struct gdbarch *gdbarch, int narg);
extern expr::operation_up gdbarch_dtrace_parse_probe_argument (struct gdbarch *gdbarch, int narg);
extern void set_gdbarch_dtrace_parse_probe_argument (struct gdbarch *gdbarch, gdbarch_dtrace_parse_probe_argument_ftype *dtrace_parse_probe_argument);

/* True if the given ADDR does not contain the instruction sequence
   corresponding to a disabled DTrace is-enabled probe. */

extern bool gdbarch_dtrace_probe_is_enabled_p (struct gdbarch *gdbarch);

typedef int (gdbarch_dtrace_probe_is_enabled_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern int gdbarch_dtrace_probe_is_enabled (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_dtrace_probe_is_enabled (struct gdbarch *gdbarch, gdbarch_dtrace_probe_is_enabled_ftype *dtrace_probe_is_enabled);

/* Enable a DTrace is-enabled probe at ADDR. */

extern bool gdbarch_dtrace_enable_probe_p (struct gdbarch *gdbarch);

typedef void (gdbarch_dtrace_enable_probe_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void gdbarch_dtrace_enable_probe (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_dtrace_enable_probe (struct gdbarch *gdbarch, gdbarch_dtrace_enable_probe_ftype *dtrace_enable_probe);

/* Disable a DTrace is-enabled probe at ADDR. */

extern bool gdbarch_dtrace_disable_probe_p (struct gdbarch *gdbarch);

typedef void (gdbarch_dtrace_disable_probe_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void gdbarch_dtrace_disable_probe (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_dtrace_disable_probe (struct gdbarch *gdbarch, gdbarch_dtrace_disable_probe_ftype *dtrace_disable_probe);

/* True if the list of shared libraries is one and only for all
   processes, as opposed to a list of shared libraries per inferior.
   This usually means that all processes, although may or may not share
   an address space, will see the same set of symbols at the same
   addresses. */

extern int gdbarch_has_global_solist (struct gdbarch *gdbarch);
extern void set_gdbarch_has_global_solist (struct gdbarch *gdbarch, int has_global_solist);

/* On some targets, even though each inferior has its own private
   address space, the debug interface takes care of making breakpoints
   visible to all address spaces automatically.  For such cases,
   this property should be set to true. */

extern int gdbarch_has_global_breakpoints (struct gdbarch *gdbarch);
extern void set_gdbarch_has_global_breakpoints (struct gdbarch *gdbarch, int has_global_breakpoints);

/* True if inferiors share an address space (e.g., uClinux). */

typedef int (gdbarch_has_shared_address_space_ftype) (struct gdbarch *gdbarch);
extern int gdbarch_has_shared_address_space (struct gdbarch *gdbarch);
extern void set_gdbarch_has_shared_address_space (struct gdbarch *gdbarch, gdbarch_has_shared_address_space_ftype *has_shared_address_space);

/* True if a fast tracepoint can be set at an address. */

typedef int (gdbarch_fast_tracepoint_valid_at_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr, std::string *msg);
extern int gdbarch_fast_tracepoint_valid_at (struct gdbarch *gdbarch, CORE_ADDR addr, std::string *msg);
extern void set_gdbarch_fast_tracepoint_valid_at (struct gdbarch *gdbarch, gdbarch_fast_tracepoint_valid_at_ftype *fast_tracepoint_valid_at);

/* Guess register state based on tracepoint location.  Used for tracepoints
   where no registers have been collected, but there's only one location,
   allowing us to guess the PC value, and perhaps some other registers.
   On entry, regcache has all registers marked as unavailable. */

typedef void (gdbarch_guess_tracepoint_registers_ftype) (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR addr);
extern void gdbarch_guess_tracepoint_registers (struct gdbarch *gdbarch, struct regcache *regcache, CORE_ADDR addr);
extern void set_gdbarch_guess_tracepoint_registers (struct gdbarch *gdbarch, gdbarch_guess_tracepoint_registers_ftype *guess_tracepoint_registers);

/* Return the "auto" target charset. */

typedef const char * (gdbarch_auto_charset_ftype) ();
extern const char * gdbarch_auto_charset (struct gdbarch *gdbarch);
extern void set_gdbarch_auto_charset (struct gdbarch *gdbarch, gdbarch_auto_charset_ftype *auto_charset);

/* Return the "auto" target wide charset. */

typedef const char * (gdbarch_auto_wide_charset_ftype) ();
extern const char * gdbarch_auto_wide_charset (struct gdbarch *gdbarch);
extern void set_gdbarch_auto_wide_charset (struct gdbarch *gdbarch, gdbarch_auto_wide_charset_ftype *auto_wide_charset);

/* If non-empty, this is a file extension that will be opened in place
   of the file extension reported by the shared library list.

   This is most useful for toolchains that use a post-linker tool,
   where the names of the files run on the target differ in extension
   compared to the names of the files GDB should load for debug info. */

extern const char * gdbarch_solib_symbols_extension (struct gdbarch *gdbarch);
extern void set_gdbarch_solib_symbols_extension (struct gdbarch *gdbarch, const char * solib_symbols_extension);

/* If true, the target OS has DOS-based file system semantics.  That
   is, absolute paths include a drive name, and the backslash is
   considered a directory separator. */

extern int gdbarch_has_dos_based_file_system (struct gdbarch *gdbarch);
extern void set_gdbarch_has_dos_based_file_system (struct gdbarch *gdbarch, int has_dos_based_file_system);

/* Generate bytecodes to collect the return address in a frame.
   Since the bytecodes run on the target, possibly with GDB not even
   connected, the full unwinding machinery is not available, and
   typically this function will issue bytecodes for one or more likely
   places that the return address may be found. */

typedef void (gdbarch_gen_return_address_ftype) (struct gdbarch *gdbarch, struct agent_expr *ax, struct axs_value *value, CORE_ADDR scope);
extern void gdbarch_gen_return_address (struct gdbarch *gdbarch, struct agent_expr *ax, struct axs_value *value, CORE_ADDR scope);
extern void set_gdbarch_gen_return_address (struct gdbarch *gdbarch, gdbarch_gen_return_address_ftype *gen_return_address);

/* Implement the "info proc" command. */

extern bool gdbarch_info_proc_p (struct gdbarch *gdbarch);

typedef void (gdbarch_info_proc_ftype) (struct gdbarch *gdbarch, const char *args, enum info_proc_what what);
extern void gdbarch_info_proc (struct gdbarch *gdbarch, const char *args, enum info_proc_what what);
extern void set_gdbarch_info_proc (struct gdbarch *gdbarch, gdbarch_info_proc_ftype *info_proc);

/* Implement the "info proc" command for core files.  Noe that there
   are two "info_proc"-like methods on gdbarch -- one for core files,
   one for live targets. */

extern bool gdbarch_core_info_proc_p (struct gdbarch *gdbarch);

typedef void (gdbarch_core_info_proc_ftype) (struct gdbarch *gdbarch, const char *args, enum info_proc_what what);
extern void gdbarch_core_info_proc (struct gdbarch *gdbarch, const char *args, enum info_proc_what what);
extern void set_gdbarch_core_info_proc (struct gdbarch *gdbarch, gdbarch_core_info_proc_ftype *core_info_proc);

/* Iterate over all objfiles in the order that makes the most sense
   for the architecture to make global symbol searches.

   CB is a callback function passed an objfile to be searched.  The iteration stops
   if this function returns nonzero.

   If not NULL, CURRENT_OBJFILE corresponds to the objfile being
   inspected when the symbol search was requested. */

typedef void (gdbarch_iterate_over_objfiles_in_search_order_ftype) (struct gdbarch *gdbarch, iterate_over_objfiles_in_search_order_cb_ftype cb, struct objfile *current_objfile);
extern void gdbarch_iterate_over_objfiles_in_search_order (struct gdbarch *gdbarch, iterate_over_objfiles_in_search_order_cb_ftype cb, struct objfile *current_objfile);
extern void set_gdbarch_iterate_over_objfiles_in_search_order (struct gdbarch *gdbarch, gdbarch_iterate_over_objfiles_in_search_order_ftype *iterate_over_objfiles_in_search_order);

/* Ravenscar arch-dependent ops. */

extern struct ravenscar_arch_ops * gdbarch_ravenscar_ops (struct gdbarch *gdbarch);
extern void set_gdbarch_ravenscar_ops (struct gdbarch *gdbarch, struct ravenscar_arch_ops * ravenscar_ops);

/* Return non-zero if the instruction at ADDR is a call; zero otherwise. */

typedef int (gdbarch_insn_is_call_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern int gdbarch_insn_is_call (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_insn_is_call (struct gdbarch *gdbarch, gdbarch_insn_is_call_ftype *insn_is_call);

/* Return non-zero if the instruction at ADDR is a return; zero otherwise. */

typedef int (gdbarch_insn_is_ret_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern int gdbarch_insn_is_ret (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_insn_is_ret (struct gdbarch *gdbarch, gdbarch_insn_is_ret_ftype *insn_is_ret);

/* Return non-zero if the instruction at ADDR is a jump; zero otherwise. */

typedef int (gdbarch_insn_is_jump_ftype) (struct gdbarch *gdbarch, CORE_ADDR addr);
extern int gdbarch_insn_is_jump (struct gdbarch *gdbarch, CORE_ADDR addr);
extern void set_gdbarch_insn_is_jump (struct gdbarch *gdbarch, gdbarch_insn_is_jump_ftype *insn_is_jump);

/* Return true if there's a program/permanent breakpoint planted in
   memory at ADDRESS, return false otherwise. */

typedef bool (gdbarch_program_breakpoint_here_p_ftype) (struct gdbarch *gdbarch, CORE_ADDR address);
extern bool gdbarch_program_breakpoint_here_p (struct gdbarch *gdbarch, CORE_ADDR address);
extern void set_gdbarch_program_breakpoint_here_p (struct gdbarch *gdbarch, gdbarch_program_breakpoint_here_p_ftype *program_breakpoint_here_p);

/* Read one auxv entry from *READPTR, not reading locations >= ENDPTR.
   Return 0 if *READPTR is already at the end of the buffer.
   Return -1 if there is insufficient buffer for a whole entry.
   Return 1 if an entry was read into *TYPEP and *VALP. */

extern bool gdbarch_auxv_parse_p (struct gdbarch *gdbarch);

typedef int (gdbarch_auxv_parse_ftype) (struct gdbarch *gdbarch, const gdb_byte **readptr, const gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp);
extern int gdbarch_auxv_parse (struct gdbarch *gdbarch, const gdb_byte **readptr, const gdb_byte *endptr, CORE_ADDR *typep, CORE_ADDR *valp);
extern void set_gdbarch_auxv_parse (struct gdbarch *gdbarch, gdbarch_auxv_parse_ftype *auxv_parse);

/* Print the description of a single auxv entry described by TYPE and VAL
   to FILE. */

typedef void (gdbarch_print_auxv_entry_ftype) (struct gdbarch *gdbarch, struct ui_file *file, CORE_ADDR type, CORE_ADDR val);
extern void gdbarch_print_auxv_entry (struct gdbarch *gdbarch, struct ui_file *file, CORE_ADDR type, CORE_ADDR val);
extern void set_gdbarch_print_auxv_entry (struct gdbarch *gdbarch, gdbarch_print_auxv_entry_ftype *print_auxv_entry);

/* Find the address range of the current inferior's vsyscall/vDSO, and
   write it to *RANGE.  If the vsyscall's length can't be determined, a
   range with zero length is returned.  Returns true if the vsyscall is
   found, false otherwise. */

typedef int (gdbarch_vsyscall_range_ftype) (struct gdbarch *gdbarch, struct mem_range *range);
extern int gdbarch_vsyscall_range (struct gdbarch *gdbarch, struct mem_range *range);
extern void set_gdbarch_vsyscall_range (struct gdbarch *gdbarch, gdbarch_vsyscall_range_ftype *vsyscall_range);

/* Allocate SIZE bytes of PROT protected page aligned memory in inferior.
   PROT has GDB_MMAP_PROT_* bitmask format.
   Throw an error if it is not possible.  Returned address is always valid. */

typedef CORE_ADDR (gdbarch_infcall_mmap_ftype) (CORE_ADDR size, unsigned prot);
extern CORE_ADDR gdbarch_infcall_mmap (struct gdbarch *gdbarch, CORE_ADDR size, unsigned prot);
extern void set_gdbarch_infcall_mmap (struct gdbarch *gdbarch, gdbarch_infcall_mmap_ftype *infcall_mmap);

/* Deallocate SIZE bytes of memory at ADDR in inferior from gdbarch_infcall_mmap.
   Print a warning if it is not possible. */

typedef void (gdbarch_infcall_munmap_ftype) (CORE_ADDR addr, CORE_ADDR size);
extern void gdbarch_infcall_munmap (struct gdbarch *gdbarch, CORE_ADDR addr, CORE_ADDR size);
extern void set_gdbarch_infcall_munmap (struct gdbarch *gdbarch, gdbarch_infcall_munmap_ftype *infcall_munmap);

/* Return string (caller has to use xfree for it) with options for GCC
   to produce code for this target, typically "-m64", "-m32" or "-m31".
   These options are put before CU's DW_AT_producer compilation options so that
   they can override it. */

typedef std::string (gdbarch_gcc_target_options_ftype) (struct gdbarch *gdbarch);
extern std::string gdbarch_gcc_target_options (struct gdbarch *gdbarch);
extern void set_gdbarch_gcc_target_options (struct gdbarch *gdbarch, gdbarch_gcc_target_options_ftype *gcc_target_options);

/* Return a regular expression that matches names used by this
   architecture in GNU configury triplets.  The result is statically
   allocated and must not be freed.  The default implementation simply
   returns the BFD architecture name, which is correct in nearly every
   case. */

typedef const char * (gdbarch_gnu_triplet_regexp_ftype) (struct gdbarch *gdbarch);
extern const char * gdbarch_gnu_triplet_regexp (struct gdbarch *gdbarch);
extern void set_gdbarch_gnu_triplet_regexp (struct gdbarch *gdbarch, gdbarch_gnu_triplet_regexp_ftype *gnu_triplet_regexp);

/* Return the size in 8-bit bytes of an addressable memory unit on this
   architecture.  This corresponds to the number of 8-bit bytes associated to
   each address in memory. */

typedef int (gdbarch_addressable_memory_unit_size_ftype) (struct gdbarch *gdbarch);
extern int gdbarch_addressable_memory_unit_size (struct gdbarch *gdbarch);
extern void set_gdbarch_addressable_memory_unit_size (struct gdbarch *gdbarch, gdbarch_addressable_memory_unit_size_ftype *addressable_memory_unit_size);

/* Functions for allowing a target to modify its disassembler options. */

extern const char * gdbarch_disassembler_options_implicit (struct gdbarch *gdbarch);
extern void set_gdbarch_disassembler_options_implicit (struct gdbarch *gdbarch, const char * disassembler_options_implicit);

extern char ** gdbarch_disassembler_options (struct gdbarch *gdbarch);
extern void set_gdbarch_disassembler_options (struct gdbarch *gdbarch, char ** disassembler_options);

extern const disasm_options_and_args_t * gdbarch_valid_disassembler_options (struct gdbarch *gdbarch);
extern void set_gdbarch_valid_disassembler_options (struct gdbarch *gdbarch, const disasm_options_and_args_t * valid_disassembler_options);

/* Type alignment override method.  Return the architecture specific
   alignment required for TYPE.  If there is no special handling
   required for TYPE then return the value 0, GDB will then apply the
   default rules as laid out in gdbtypes.c:type_align. */

typedef ULONGEST (gdbarch_type_align_ftype) (struct gdbarch *gdbarch, struct type *type);
extern ULONGEST gdbarch_type_align (struct gdbarch *gdbarch, struct type *type);
extern void set_gdbarch_type_align (struct gdbarch *gdbarch, gdbarch_type_align_ftype *type_align);

/* Return a string containing any flags for the given PC in the given FRAME. */

typedef std::string (gdbarch_get_pc_address_flags_ftype) (frame_info_ptr frame, CORE_ADDR pc);
extern std::string gdbarch_get_pc_address_flags (struct gdbarch *gdbarch, frame_info_ptr frame, CORE_ADDR pc);
extern void set_gdbarch_get_pc_address_flags (struct gdbarch *gdbarch, gdbarch_get_pc_address_flags_ftype *get_pc_address_flags);

/* Read core file mappings */

typedef void (gdbarch_read_core_file_mappings_ftype) (struct gdbarch *gdbarch, struct bfd *cbfd, read_core_file_mappings_pre_loop_ftype pre_loop_cb, read_core_file_mappings_loop_ftype loop_cb);
extern void gdbarch_read_core_file_mappings (struct gdbarch *gdbarch, struct bfd *cbfd, read_core_file_mappings_pre_loop_ftype pre_loop_cb, read_core_file_mappings_loop_ftype loop_cb);
extern void set_gdbarch_read_core_file_mappings (struct gdbarch *gdbarch, gdbarch_read_core_file_mappings_ftype *read_core_file_mappings);

/* Return true if the target description for all threads should be read from the
   target description core file note(s).  Return false if the target description
   for all threads should be inferred from the core file contents/sections.

   The corefile's bfd is passed through COREFILE_BFD. */

typedef bool (gdbarch_use_target_description_from_corefile_notes_ftype) (struct gdbarch *gdbarch, struct bfd *corefile_bfd);
extern bool gdbarch_use_target_description_from_corefile_notes (struct gdbarch *gdbarch, struct bfd *corefile_bfd);
extern void set_gdbarch_use_target_description_from_corefile_notes (struct gdbarch *gdbarch, gdbarch_use_target_description_from_corefile_notes_ftype *use_target_description_from_corefile_notes);
