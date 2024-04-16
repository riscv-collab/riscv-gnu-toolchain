# Dynamic architecture support for GDB, the GNU debugger.

# Copyright (C) 1998-2024 Free Software Foundation, Inc.

# This file is part of GDB.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# How to add to gdbarch:
#
# There are four kinds of fields in gdbarch:
#
# * Info - you should never need this; it is only for things that are
# copied directly from the gdbarch_info.
#
# * Value - a variable.
#
# * Function - a function pointer.
#
# * Method - a function pointer, but the function takes a gdbarch as
# its first parameter.
#
# You construct a new one with a call to one of those functions.  So,
# for instance, you can use the function named "Value" to make a new
# Value.
#
# All parameters are keyword-only.  This is done to help catch typos.
#
# Some parameters are shared among all types (including Info):
#
# * "name" - required, the name of the field.
#
# * "type" - required, the type of the field.  For functions and
# methods, this is the return type.
#
# * "printer" - an expression to turn this field into a 'const char
# *'.  This is used for dumping.  The string must live long enough to
# be passed to printf.
#
# Value, Function, and Method share some more parameters.  Some of
# these work in conjunction in a somewhat complicated way, so they are
# described in a separate sub-section below.
#
# * "comment" - a comment that's written to the .h file.  Please
# always use this.  (It isn't currently a required option for
# historical reasons.)
#
# * "predicate" - a boolean, if True then a _p predicate function will
# be generated.  The predicate will use the generic validation
# function for the field.  See below.
#
# * "predefault", "postdefault", and "invalid" - These are used for
# the initialization and verification steps:
#
# A gdbarch is zero-initialized.  Then, if a field has a "predefault",
# the field is set to that value.  This becomes the field's initial
# value.
#
# After initialization is complete (that is, after the tdep code has a
# chance to change the settings), the post-initialization step is
# done.
#
# If the field still has its initial value (see above), and the field
# has a "postdefault", then the field is set to this value.
#
# After the possible "postdefault" assignment, validation is
# performed for fields that don't have a "predicate".
#
# If the field has an "invalid" attribute with a string value, then
# this string is the expression that should evaluate to true when the
# field is invalid.
#
# Otherwise, if "invalid" is True (the default), then the generic
# validation function is used: the field is considered invalid it
# still contains its default value.  This validation is what is used
# within the _p predicate function if the field has "predicate" set to
# True.
#
# Function and Method share:
#
# * "params" - required, a tuple of tuples.  Each inner tuple is a
# pair of the form (TYPE, NAME), where TYPE is the type of this
# argument, and NAME is the name.  Note that while the names could be
# auto-generated, this approach lets the "comment" field refer to
# arguments in a nicer way.  It is also just nicer for users.
#
# * "param_checks" - optional, a list of strings.  Each string is an
# expression that is placed within a gdb_assert before the call is
# made to the Function/Method implementation.  Each expression is
# something that should be true, and it is expected that the
# expression will make use of the parameters named in 'params' (though
# this is not required).
#
# * "result_checks" - optional, a list of strings.  Each string is an
# expression that is placed within a gdb_assert after the call to the
# Function/Method implementation.  Within each expression the variable
# 'result' can be used to reference the result of the function/method
# implementation.  The 'result_checks' can only be used if the 'type'
# of this Function/Method is not 'void'.
#
# * "implement" - optional, a boolean.  If True (the default), a
# wrapper function for this function will be emitted.

from gdbarch_types import Function, Info, Method, Value

Info(
    type="const struct bfd_arch_info *",
    name="bfd_arch_info",
    printer="gdbarch_bfd_arch_info (gdbarch)->printable_name",
)

Info(
    type="enum bfd_endian",
    name="byte_order",
)

Info(
    type="enum bfd_endian",
    name="byte_order_for_code",
)

Info(
    type="enum gdb_osabi",
    name="osabi",
)

Info(
    type="const struct target_desc *",
    name="target_desc",
    printer="host_address_to_string (gdbarch->target_desc)",
)

Value(
    comment="""
Number of bits in a short or unsigned short for the target machine.
""",
    type="int",
    name="short_bit",
    predefault="2*TARGET_CHAR_BIT",
    invalid=False,
)

int_bit = Value(
    comment="""
Number of bits in an int or unsigned int for the target machine.
""",
    type="int",
    name="int_bit",
    predefault="4*TARGET_CHAR_BIT",
    invalid=False,
)

long_bit_predefault = "4*TARGET_CHAR_BIT"
long_bit = Value(
    comment="""
Number of bits in a long or unsigned long for the target machine.
""",
    type="int",
    name="long_bit",
    predefault=long_bit_predefault,
    invalid=False,
)

Value(
    comment="""
Number of bits in a long long or unsigned long long for the target
machine.
""",
    type="int",
    name="long_long_bit",
    predefault="2*" + long_bit_predefault,
    invalid=False,
)

Value(
    comment="""
The ABI default bit-size and format for "bfloat16", "half", "float", "double", and
"long double".  These bit/format pairs should eventually be combined
into a single object.  For the moment, just initialize them as a pair.
Each format describes both the big and little endian layouts (if
useful).
""",
    type="int",
    name="bfloat16_bit",
    predefault="2*TARGET_CHAR_BIT",
    invalid=False,
)

Value(
    type="const struct floatformat **",
    name="bfloat16_format",
    predefault="floatformats_bfloat16",
    printer="pformat (gdbarch, gdbarch->bfloat16_format)",
    invalid=False,
)

Value(
    type="int",
    name="half_bit",
    predefault="2*TARGET_CHAR_BIT",
    invalid=False,
)

Value(
    type="const struct floatformat **",
    name="half_format",
    predefault="floatformats_ieee_half",
    printer="pformat (gdbarch, gdbarch->half_format)",
    invalid=False,
)

Value(
    type="int",
    name="float_bit",
    predefault="4*TARGET_CHAR_BIT",
    invalid=False,
)

Value(
    type="const struct floatformat **",
    name="float_format",
    predefault="floatformats_ieee_single",
    printer="pformat (gdbarch, gdbarch->float_format)",
    invalid=False,
)

Value(
    type="int",
    name="double_bit",
    predefault="8*TARGET_CHAR_BIT",
    invalid=False,
)

Value(
    type="const struct floatformat **",
    name="double_format",
    predefault="floatformats_ieee_double",
    printer="pformat (gdbarch, gdbarch->double_format)",
    invalid=False,
)

Value(
    type="int",
    name="long_double_bit",
    predefault="8*TARGET_CHAR_BIT",
    invalid=False,
)

Value(
    type="const struct floatformat **",
    name="long_double_format",
    predefault="floatformats_ieee_double",
    printer="pformat (gdbarch, gdbarch->long_double_format)",
    invalid=False,
)

Value(
    comment="""
The ABI default bit-size for "wchar_t".  wchar_t is a built-in type
starting with C++11.
""",
    type="int",
    name="wchar_bit",
    predefault="4*TARGET_CHAR_BIT",
    invalid=False,
)

Value(
    comment="""
One if `wchar_t' is signed, zero if unsigned.
""",
    type="int",
    name="wchar_signed",
    predefault="-1",
    postdefault="1",
    invalid=False,
)

Method(
    comment="""
Returns the floating-point format to be used for values of length LENGTH.
NAME, if non-NULL, is the type name, which may be used to distinguish
different target formats of the same length.
""",
    type="const struct floatformat **",
    name="floatformat_for_type",
    params=[("const char *", "name"), ("int", "length")],
    predefault="default_floatformat_for_type",
    invalid=False,
)

Value(
    comment="""
For most targets, a pointer on the target and its representation as an
address in GDB have the same size and "look the same".  For such a
target, you need only set gdbarch_ptr_bit and gdbarch_addr_bit
/ addr_bit will be set from it.

If gdbarch_ptr_bit and gdbarch_addr_bit are different, you'll probably
also need to set gdbarch_dwarf2_addr_size, gdbarch_pointer_to_address and
gdbarch_address_to_pointer as well.

ptr_bit is the size of a pointer on the target
""",
    type="int",
    name="ptr_bit",
    predefault=int_bit.predefault,
    invalid=False,
)

Value(
    comment="""
addr_bit is the size of a target address as represented in gdb
""",
    type="int",
    name="addr_bit",
    predefault="0",
    postdefault="gdbarch_ptr_bit (gdbarch)",
    invalid=False,
)

Value(
    comment="""
dwarf2_addr_size is the target address size as used in the Dwarf debug
info.  For .debug_frame FDEs, this is supposed to be the target address
size from the associated CU header, and which is equivalent to the
DWARF2_ADDR_SIZE as defined by the target specific GCC back-end.
Unfortunately there is no good way to determine this value.  Therefore
dwarf2_addr_size simply defaults to the target pointer size.

dwarf2_addr_size is not used for .eh_frame FDEs, which are generally
defined using the target's pointer size so far.

Note that dwarf2_addr_size only needs to be redefined by a target if the
GCC back-end defines a DWARF2_ADDR_SIZE other than the target pointer size,
and if Dwarf versions < 4 need to be supported.
""",
    type="int",
    name="dwarf2_addr_size",
    postdefault="gdbarch_ptr_bit (gdbarch) / TARGET_CHAR_BIT",
    invalid=False,
)

Value(
    comment="""
One if `char' acts like `signed char', zero if `unsigned char'.
""",
    type="int",
    name="char_signed",
    predefault="-1",
    postdefault="1",
    invalid=False,
)

Function(
    type="CORE_ADDR",
    name="read_pc",
    params=[("readable_regcache *", "regcache")],
    predicate=True,
)

Function(
    type="void",
    name="write_pc",
    params=[("struct regcache *", "regcache"), ("CORE_ADDR", "val")],
    predicate=True,
)

Method(
    comment="""
Function for getting target's idea of a frame pointer.  FIXME: GDB's
whole scheme for dealing with "frames" and "frame pointers" needs a
serious shakedown.
""",
    type="void",
    name="virtual_frame_pointer",
    params=[
        ("CORE_ADDR", "pc"),
        ("int *", "frame_regnum"),
        ("LONGEST *", "frame_offset"),
    ],
    predefault="legacy_virtual_frame_pointer",
    invalid=False,
)

Method(
    type="enum register_status",
    name="pseudo_register_read",
    params=[
        ("readable_regcache *", "regcache"),
        ("int", "cookednum"),
        ("gdb_byte *", "buf"),
    ],
    predicate=True,
)

Method(
    comment="""
Read a register into a new struct value.  If the register is wholly
or partly unavailable, this should call mark_value_bytes_unavailable
as appropriate.  If this is defined, then pseudo_register_read will
never be called.
""",
    type="struct value *",
    name="pseudo_register_read_value",
    params=[("frame_info_ptr", "next_frame"), ("int", "cookednum")],
    predicate=True,
)

Method(
    comment="""
Write bytes in BUF to pseudo register with number PSEUDO_REG_NUM.

Raw registers backing the pseudo register should be written to using
NEXT_FRAME.
""",
    type="void",
    name="pseudo_register_write",
    params=[
        ("frame_info_ptr", "next_frame"),
        ("int", "pseudo_reg_num"),
        ("gdb::array_view<const gdb_byte>", "buf"),
    ],
    predicate=True,
)

Method(
    comment="""
Write bytes to a pseudo register.

This is marked as deprecated because it gets passed a regcache for
implementations to write raw registers in.  This doesn't work for unwound
frames, where the raw registers backing the pseudo registers may have been
saved elsewhere.

Implementations should be migrated to implement pseudo_register_write instead.
""",
    type="void",
    name="deprecated_pseudo_register_write",
    params=[
        ("struct regcache *", "regcache"),
        ("int", "cookednum"),
        ("const gdb_byte *", "buf"),
    ],
    predicate=True,
)

Value(
    type="int",
    name="num_regs",
    predefault="-1",
)

Value(
    comment="""
This macro gives the number of pseudo-registers that live in the
register namespace but do not get fetched or stored on the target.
These pseudo-registers may be aliases for other registers,
combinations of other registers, or they may be computed by GDB.
""",
    type="int",
    name="num_pseudo_regs",
    predefault="0",
    invalid=False,
)

Method(
    comment="""
Assemble agent expression bytecode to collect pseudo-register REG.
Return -1 if something goes wrong, 0 otherwise.
""",
    type="int",
    name="ax_pseudo_register_collect",
    params=[("struct agent_expr *", "ax"), ("int", "reg")],
    predicate=True,
)

Method(
    comment="""
Assemble agent expression bytecode to push the value of pseudo-register
REG on the interpreter stack.
Return -1 if something goes wrong, 0 otherwise.
""",
    type="int",
    name="ax_pseudo_register_push_stack",
    params=[("struct agent_expr *", "ax"), ("int", "reg")],
    predicate=True,
)

Method(
    comment="""
Some architectures can display additional information for specific
signals.
UIOUT is the output stream where the handler will place information.
""",
    type="void",
    name="report_signal_info",
    params=[("struct ui_out *", "uiout"), ("enum gdb_signal", "siggnal")],
    predicate=True,
)

Value(
    comment="""
GDB's standard (or well known) register numbers.  These can map onto
a real register or a pseudo (computed) register or not be defined at
all (-1).
gdbarch_sp_regnum will hopefully be replaced by UNWIND_SP.
""",
    type="int",
    name="sp_regnum",
    predefault="-1",
    invalid=False,
)

Value(
    type="int",
    name="pc_regnum",
    predefault="-1",
    invalid=False,
)

Value(
    type="int",
    name="ps_regnum",
    predefault="-1",
    invalid=False,
)

Value(
    type="int",
    name="fp0_regnum",
    predefault="-1",
    invalid=False,
)

Method(
    comment="""
Convert stab register number (from `r' declaration) to a gdb REGNUM.
""",
    type="int",
    name="stab_reg_to_regnum",
    params=[("int", "stab_regnr")],
    predefault="no_op_reg_to_regnum",
    invalid=False,
)

Method(
    comment="""
Provide a default mapping from a ecoff register number to a gdb REGNUM.
""",
    type="int",
    name="ecoff_reg_to_regnum",
    params=[("int", "ecoff_regnr")],
    predefault="no_op_reg_to_regnum",
    invalid=False,
)

Method(
    comment="""
Convert from an sdb register number to an internal gdb register number.
""",
    type="int",
    name="sdb_reg_to_regnum",
    params=[("int", "sdb_regnr")],
    predefault="no_op_reg_to_regnum",
    invalid=False,
)

Method(
    comment="""
Provide a default mapping from a DWARF2 register number to a gdb REGNUM.
Return -1 for bad REGNUM.  Note: Several targets get this wrong.
""",
    type="int",
    name="dwarf2_reg_to_regnum",
    params=[("int", "dwarf2_regnr")],
    predefault="no_op_reg_to_regnum",
    invalid=False,
)

Method(
    comment="""
Return the name of register REGNR for the specified architecture.
REGNR can be any value greater than, or equal to zero, and less than
'gdbarch_num_cooked_regs (GDBARCH)'.  If REGNR is not supported for
GDBARCH, then this function will return an empty string, this function
should never return nullptr.
""",
    type="const char *",
    name="register_name",
    params=[("int", "regnr")],
    param_checks=["regnr >= 0", "regnr < gdbarch_num_cooked_regs (gdbarch)"],
    result_checks=["result != nullptr"],
)

Method(
    comment="""
Return the type of a register specified by the architecture.  Only
the register cache should call this function directly; others should
use "register_type".
""",
    type="struct type *",
    name="register_type",
    params=[("int", "reg_nr")],
)

Method(
    comment="""
Generate a dummy frame_id for THIS_FRAME assuming that the frame is
a dummy frame.  A dummy frame is created before an inferior call,
the frame_id returned here must match the frame_id that was built
for the inferior call.  Usually this means the returned frame_id's
stack address should match the address returned by
gdbarch_push_dummy_call, and the returned frame_id's code address
should match the address at which the breakpoint was set in the dummy
frame.
""",
    type="struct frame_id",
    name="dummy_id",
    params=[("frame_info_ptr", "this_frame")],
    predefault="default_dummy_id",
    invalid=False,
)

Value(
    comment="""
Implement DUMMY_ID and PUSH_DUMMY_CALL, then delete
deprecated_fp_regnum.
""",
    type="int",
    name="deprecated_fp_regnum",
    predefault="-1",
    invalid=False,
)

Method(
    type="CORE_ADDR",
    name="push_dummy_call",
    params=[
        ("struct value *", "function"),
        ("struct regcache *", "regcache"),
        ("CORE_ADDR", "bp_addr"),
        ("int", "nargs"),
        ("struct value **", "args"),
        ("CORE_ADDR", "sp"),
        ("function_call_return_method", "return_method"),
        ("CORE_ADDR", "struct_addr"),
    ],
    predicate=True,
)

Value(
    type="enum call_dummy_location_type",
    name="call_dummy_location",
    predefault="AT_ENTRY_POINT",
    invalid=False,
)

Method(
    type="CORE_ADDR",
    name="push_dummy_code",
    params=[
        ("CORE_ADDR", "sp"),
        ("CORE_ADDR", "funaddr"),
        ("struct value **", "args"),
        ("int", "nargs"),
        ("struct type *", "value_type"),
        ("CORE_ADDR *", "real_pc"),
        ("CORE_ADDR *", "bp_addr"),
        ("struct regcache *", "regcache"),
    ],
    predicate=True,
)

Method(
    comment="""
Return true if the code of FRAME is writable.
""",
    type="int",
    name="code_of_frame_writable",
    params=[("frame_info_ptr", "frame")],
    predefault="default_code_of_frame_writable",
    invalid=False,
)

Method(
    type="void",
    name="print_registers_info",
    params=[
        ("struct ui_file *", "file"),
        ("frame_info_ptr", "frame"),
        ("int", "regnum"),
        ("int", "all"),
    ],
    predefault="default_print_registers_info",
    invalid=False,
)

Method(
    type="void",
    name="print_float_info",
    params=[
        ("struct ui_file *", "file"),
        ("frame_info_ptr", "frame"),
        ("const char *", "args"),
    ],
    predefault="default_print_float_info",
    invalid=False,
)

Method(
    type="void",
    name="print_vector_info",
    params=[
        ("struct ui_file *", "file"),
        ("frame_info_ptr", "frame"),
        ("const char *", "args"),
    ],
    predicate=True,
)

Method(
    comment="""
MAP a GDB RAW register number onto a simulator register number.  See
also include/...-sim.h.
""",
    type="int",
    name="register_sim_regno",
    params=[("int", "reg_nr")],
    predefault="legacy_register_sim_regno",
    invalid=False,
)

Method(
    type="int",
    name="cannot_fetch_register",
    params=[("int", "regnum")],
    predefault="cannot_register_not",
    invalid=False,
)

Method(
    type="int",
    name="cannot_store_register",
    params=[("int", "regnum")],
    predefault="cannot_register_not",
    invalid=False,
)

Function(
    comment="""
Determine the address where a longjmp will land and save this address
in PC.  Return nonzero on success.

FRAME corresponds to the longjmp frame.
""",
    type="int",
    name="get_longjmp_target",
    params=[("frame_info_ptr", "frame"), ("CORE_ADDR *", "pc")],
    predicate=True,
)

Value(
    type="int",
    name="believe_pcc_promotion",
    invalid=False,
)

Method(
    type="int",
    name="convert_register_p",
    params=[("int", "regnum"), ("struct type *", "type")],
    predefault="generic_convert_register_p",
    invalid=False,
)

Function(
    type="int",
    name="register_to_value",
    params=[
        ("frame_info_ptr", "frame"),
        ("int", "regnum"),
        ("struct type *", "type"),
        ("gdb_byte *", "buf"),
        ("int *", "optimizedp"),
        ("int *", "unavailablep"),
    ],
    invalid=False,
)

Function(
    type="void",
    name="value_to_register",
    params=[
        ("frame_info_ptr", "frame"),
        ("int", "regnum"),
        ("struct type *", "type"),
        ("const gdb_byte *", "buf"),
    ],
    invalid=False,
)

Method(
    comment="""
Construct a value representing the contents of register REGNUM in
frame THIS_FRAME, interpreted as type TYPE.  The routine needs to
allocate and return a struct value with all value attributes
(but not the value contents) filled in.
""",
    type="struct value *",
    name="value_from_register",
    params=[
        ("struct type *", "type"),
        ("int", "regnum"),
        ("const frame_info_ptr &", "this_frame"),
    ],
    predefault="default_value_from_register",
    invalid=False,
)

Method(
    type="CORE_ADDR",
    name="pointer_to_address",
    params=[("struct type *", "type"), ("const gdb_byte *", "buf")],
    predefault="unsigned_pointer_to_address",
    invalid=False,
)

Method(
    type="void",
    name="address_to_pointer",
    params=[("struct type *", "type"), ("gdb_byte *", "buf"), ("CORE_ADDR", "addr")],
    predefault="unsigned_address_to_pointer",
    invalid=False,
)

Method(
    type="CORE_ADDR",
    name="integer_to_address",
    params=[("struct type *", "type"), ("const gdb_byte *", "buf")],
    predicate=True,
)

Method(
    comment="""
Return the return-value convention that will be used by FUNCTION
to return a value of type VALTYPE.  FUNCTION may be NULL in which
case the return convention is computed based only on VALTYPE.

If READBUF is not NULL, extract the return value and save it in this buffer.

If WRITEBUF is not NULL, it contains a return value which will be
stored into the appropriate register.  This can be used when we want
to force the value returned by a function (see the "return" command
for instance).

NOTE: it is better to implement return_value_as_value instead, as that
method can properly handle variably-sized types.
""",
    type="enum return_value_convention",
    name="return_value",
    params=[
        ("struct value *", "function"),
        ("struct type *", "valtype"),
        ("struct regcache *", "regcache"),
        ("gdb_byte *", "readbuf"),
        ("const gdb_byte *", "writebuf"),
    ],
    invalid=False,
    # We don't want to accidentally introduce calls to this, as gdb
    # should only ever call return_value_new (see below).
    implement=False,
)

Method(
    comment="""
Return the return-value convention that will be used by FUNCTION
to return a value of type VALTYPE.  FUNCTION may be NULL in which
case the return convention is computed based only on VALTYPE.

If READ_VALUE is not NULL, extract the return value and save it in
this pointer.

If WRITEBUF is not NULL, it contains a return value which will be
stored into the appropriate register.  This can be used when we want
to force the value returned by a function (see the "return" command
for instance).
""",
    type="enum return_value_convention",
    name="return_value_as_value",
    params=[
        ("struct value *", "function"),
        ("struct type *", "valtype"),
        ("struct regcache *", "regcache"),
        ("struct value **", "read_value"),
        ("const gdb_byte *", "writebuf"),
    ],
    predefault="default_gdbarch_return_value",
    # If we're using the default, then the other method must be set;
    # but if we aren't using the default here then the other method
    # must not be set.
    invalid="(gdbarch->return_value_as_value == default_gdbarch_return_value) == (gdbarch->return_value == nullptr)",
)

Function(
    comment="""
Return the address at which the value being returned from
the current function will be stored.  This routine is only
called if the current function uses the the "struct return
convention".

May return 0 when unable to determine that address.""",
    type="CORE_ADDR",
    name="get_return_buf_addr",
    params=[("struct type *", "val_type"), ("frame_info_ptr", "cur_frame")],
    predefault="default_get_return_buf_addr",
    invalid=False,
)


# The DWARF info currently does not distinguish between IEEE 128-bit floating
# point values and the IBM 128-bit floating point format.  GCC has an internal
# hack to identify the IEEE 128-bit floating point value.  The long double is a
# defined base type in C.  The GCC hack uses a typedef for long double to
# reference_Float128 base to identify the long double as and IEEE 128-bit
# value.  The following method is used to "fix" the long double type to be a
# base type with the IEEE float format info from the _Float128 basetype and
# the long double name.  With the fix, the proper name is printed for the
# GDB typedef command.
Function(
    comment="""
Return true if the typedef record needs to be replaced.".

Return 0 by default""",
    type="bool",
    name="dwarf2_omit_typedef_p",
    params=[
        ("struct type *", "target_type"),
        ("const char *", "producer"),
        ("const char *", "name"),
    ],
    predefault="default_dwarf2_omit_typedef_p",
    invalid=False,
)

Method(
    comment="""
Update PC when trying to find a call site.  This is useful on
architectures where the call site PC, as reported in the DWARF, can be
incorrect for some reason.

The passed-in PC will be an address in the inferior.  GDB will have
already failed to find a call site at this PC.  This function may
simply return its parameter if it thinks that should be the correct
address.""",
    type="CORE_ADDR",
    name="update_call_site_pc",
    params=[("CORE_ADDR", "pc")],
    predefault="default_update_call_site_pc",
    invalid=False,
)

Method(
    comment="""
Return true if the return value of function is stored in the first hidden
parameter.  In theory, this feature should be language-dependent, specified
by language and its ABI, such as C++.  Unfortunately, compiler may
implement it to a target-dependent feature.  So that we need such hook here
to be aware of this in GDB.
""",
    type="int",
    name="return_in_first_hidden_param_p",
    params=[("struct type *", "type")],
    predefault="default_return_in_first_hidden_param_p",
    invalid=False,
)

Method(
    type="CORE_ADDR",
    name="skip_prologue",
    params=[("CORE_ADDR", "ip")],
)

Method(
    type="CORE_ADDR",
    name="skip_main_prologue",
    params=[("CORE_ADDR", "ip")],
    predicate=True,
)

Method(
    comment="""
On some platforms, a single function may provide multiple entry points,
e.g. one that is used for function-pointer calls and a different one
that is used for direct function calls.
In order to ensure that breakpoints set on the function will trigger
no matter via which entry point the function is entered, a platform
may provide the skip_entrypoint callback.  It is called with IP set
to the main entry point of a function (as determined by the symbol table),
and should return the address of the innermost entry point, where the
actual breakpoint needs to be set.  Note that skip_entrypoint is used
by GDB common code even when debugging optimized code, where skip_prologue
is not used.
""",
    type="CORE_ADDR",
    name="skip_entrypoint",
    params=[("CORE_ADDR", "ip")],
    predicate=True,
)

Function(
    type="int",
    name="inner_than",
    params=[("CORE_ADDR", "lhs"), ("CORE_ADDR", "rhs")],
)

Method(
    type="const gdb_byte *",
    name="breakpoint_from_pc",
    params=[("CORE_ADDR *", "pcptr"), ("int *", "lenptr")],
    predefault="default_breakpoint_from_pc",
    invalid=False,
)

Method(
    comment="""
Return the breakpoint kind for this target based on *PCPTR.
""",
    type="int",
    name="breakpoint_kind_from_pc",
    params=[("CORE_ADDR *", "pcptr")],
)

Method(
    comment="""
Return the software breakpoint from KIND.  KIND can have target
specific meaning like the Z0 kind parameter.
SIZE is set to the software breakpoint's length in memory.
""",
    type="const gdb_byte *",
    name="sw_breakpoint_from_kind",
    params=[("int", "kind"), ("int *", "size")],
    predefault="NULL",
    invalid=False,
)

Method(
    comment="""
Return the breakpoint kind for this target based on the current
processor state (e.g. the current instruction mode on ARM) and the
*PCPTR.  In default, it is gdbarch->breakpoint_kind_from_pc.
""",
    type="int",
    name="breakpoint_kind_from_current_state",
    params=[("struct regcache *", "regcache"), ("CORE_ADDR *", "pcptr")],
    predefault="default_breakpoint_kind_from_current_state",
    invalid=False,
)

Method(
    type="CORE_ADDR",
    name="adjust_breakpoint_address",
    params=[("CORE_ADDR", "bpaddr")],
    predicate=True,
)

Method(
    type="int",
    name="memory_insert_breakpoint",
    params=[("struct bp_target_info *", "bp_tgt")],
    predefault="default_memory_insert_breakpoint",
    invalid=False,
)

Method(
    type="int",
    name="memory_remove_breakpoint",
    params=[("struct bp_target_info *", "bp_tgt")],
    predefault="default_memory_remove_breakpoint",
    invalid=False,
)

Value(
    type="CORE_ADDR",
    name="decr_pc_after_break",
    invalid=False,
)

Value(
    comment="""
A function can be addressed by either its "pointer" (possibly a
descriptor address) or "entry point" (first executable instruction).
The method "convert_from_func_ptr_addr" converting the former to the
latter.  gdbarch_deprecated_function_start_offset is being used to implement
a simplified subset of that functionality - the function's address
corresponds to the "function pointer" and the function's start
corresponds to the "function entry point" - and hence is redundant.
""",
    type="CORE_ADDR",
    name="deprecated_function_start_offset",
    invalid=False,
)

Method(
    comment="""
Return the remote protocol register number associated with this
register.  Normally the identity mapping.
""",
    type="int",
    name="remote_register_number",
    params=[("int", "regno")],
    predefault="default_remote_register_number",
    invalid=False,
)

Function(
    comment="""
Fetch the target specific address used to represent a load module.
""",
    type="CORE_ADDR",
    name="fetch_tls_load_module_address",
    params=[("struct objfile *", "objfile")],
    predicate=True,
)

Method(
    comment="""
Return the thread-local address at OFFSET in the thread-local
storage for the thread PTID and the shared library or executable
file given by LM_ADDR.  If that block of thread-local storage hasn't
been allocated yet, this function may throw an error.  LM_ADDR may
be zero for statically linked multithreaded inferiors.
""",
    type="CORE_ADDR",
    name="get_thread_local_address",
    params=[("ptid_t", "ptid"), ("CORE_ADDR", "lm_addr"), ("CORE_ADDR", "offset")],
    predicate=True,
)

Value(
    type="CORE_ADDR",
    name="frame_args_skip",
    invalid=False,
)

Method(
    type="CORE_ADDR",
    name="unwind_pc",
    params=[("frame_info_ptr", "next_frame")],
    predefault="default_unwind_pc",
    invalid=False,
)

Method(
    type="CORE_ADDR",
    name="unwind_sp",
    params=[("frame_info_ptr", "next_frame")],
    predefault="default_unwind_sp",
    invalid=False,
)

Function(
    comment="""
DEPRECATED_FRAME_LOCALS_ADDRESS as been replaced by the per-frame
frame-base.  Enable frame-base before frame-unwind.
""",
    type="int",
    name="frame_num_args",
    params=[("frame_info_ptr", "frame")],
    predicate=True,
)

Method(
    type="CORE_ADDR",
    name="frame_align",
    params=[("CORE_ADDR", "address")],
    predicate=True,
)

Method(
    type="int",
    name="stabs_argument_has_addr",
    params=[("struct type *", "type")],
    predefault="default_stabs_argument_has_addr",
    invalid=False,
)

Value(
    type="int",
    name="frame_red_zone_size",
    invalid=False,
)

Method(
    type="CORE_ADDR",
    name="convert_from_func_ptr_addr",
    params=[("CORE_ADDR", "addr"), ("struct target_ops *", "targ")],
    predefault="convert_from_func_ptr_addr_identity",
    invalid=False,
)

Method(
    comment="""
On some machines there are bits in addresses which are not really
part of the address, but are used by the kernel, the hardware, etc.
for special purposes.  gdbarch_addr_bits_remove takes out any such bits so
we get a "real" address such as one would find in a symbol table.
This is used only for addresses of instructions, and even then I'm
not sure it's used in all contexts.  It exists to deal with there
being a few stray bits in the PC which would mislead us, not as some
sort of generic thing to handle alignment or segmentation (it's
possible it should be in TARGET_READ_PC instead).
""",
    type="CORE_ADDR",
    name="addr_bits_remove",
    params=[("CORE_ADDR", "addr")],
    predefault="core_addr_identity",
    invalid=False,
)

Method(
    comment="""
On some architectures, not all bits of a pointer are significant.
On AArch64, for example, the top bits of a pointer may carry a "tag", which
can be ignored by the kernel and the hardware.  The "tag" can be regarded as
additional data associated with the pointer, but it is not part of the address.

Given a pointer for the architecture, this hook removes all the
non-significant bits and sign-extends things as needed.  It gets used to remove
non-address bits from data pointers (for example, removing the AArch64 MTE tag
bits from a pointer) and from code pointers (removing the AArch64 PAC signature
from a pointer containing the return address).
""",
    type="CORE_ADDR",
    name="remove_non_address_bits",
    params=[("CORE_ADDR", "pointer")],
    predefault="default_remove_non_address_bits",
    invalid=False,
)

Method(
    comment="""
Return a string representation of the memory tag TAG.
""",
    type="std::string",
    name="memtag_to_string",
    params=[("struct value *", "tag")],
    predefault="default_memtag_to_string",
    invalid=False,
)

Method(
    comment="""
Return true if ADDRESS contains a tag and false otherwise.  ADDRESS
must be either a pointer or a reference type.
""",
    type="bool",
    name="tagged_address_p",
    params=[("struct value *", "address")],
    predefault="default_tagged_address_p",
    invalid=False,
)

Method(
    comment="""
Return true if the tag from ADDRESS matches the memory tag for that
particular address.  Return false otherwise.
""",
    type="bool",
    name="memtag_matches_p",
    params=[("struct value *", "address")],
    predefault="default_memtag_matches_p",
    invalid=False,
)

Method(
    comment="""
Set the tags of type TAG_TYPE, for the memory address range
[ADDRESS, ADDRESS + LENGTH) to TAGS.
Return true if successful and false otherwise.
""",
    type="bool",
    name="set_memtags",
    params=[
        ("struct value *", "address"),
        ("size_t", "length"),
        ("const gdb::byte_vector &", "tags"),
        ("memtag_type", "tag_type"),
    ],
    predefault="default_set_memtags",
    invalid=False,
)

Method(
    comment="""
Return the tag of type TAG_TYPE associated with the memory address ADDRESS,
assuming ADDRESS is tagged.
""",
    type="struct value *",
    name="get_memtag",
    params=[("struct value *", "address"), ("memtag_type", "tag_type")],
    predefault="default_get_memtag",
    invalid=False,
)

Value(
    comment="""
memtag_granule_size is the size of the allocation tag granule, for
architectures that support memory tagging.
This is 0 for architectures that do not support memory tagging.
For a non-zero value, this represents the number of bytes of memory per tag.
""",
    type="CORE_ADDR",
    name="memtag_granule_size",
    invalid=False,
)

Function(
    comment="""
FIXME/cagney/2001-01-18: This should be split in two.  A target method that
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
past a conditional branch to self.
""",
    type="std::vector<CORE_ADDR>",
    name="software_single_step",
    params=[("struct regcache *", "regcache")],
    predicate=True,
)

Method(
    comment="""
Return non-zero if the processor is executing a delay slot and a
further single-step is needed before the instruction finishes.
""",
    type="int",
    name="single_step_through_delay",
    params=[("frame_info_ptr", "frame")],
    predicate=True,
)

Function(
    comment="""
FIXME: cagney/2003-08-28: Need to find a better way of selecting the
disassembler.  Perhaps objdump can handle it?
""",
    type="int",
    name="print_insn",
    params=[("bfd_vma", "vma"), ("struct disassemble_info *", "info")],
    predefault="default_print_insn",
    invalid=False,
)

Function(
    type="CORE_ADDR",
    name="skip_trampoline_code",
    params=[("frame_info_ptr", "frame"), ("CORE_ADDR", "pc")],
    predefault="generic_skip_trampoline_code",
    invalid=False,
)

Value(
    comment="Vtable of solib operations functions.",
    type="const struct target_so_ops *",
    name="so_ops",
    predefault="&solib_target_so_ops",
    printer="host_address_to_string (gdbarch->so_ops)",
    invalid=False,
)

Method(
    comment="""
If in_solib_dynsym_resolve_code() returns true, and SKIP_SOLIB_RESOLVER
evaluates non-zero, this is the address where the debugger will place
a step-resume breakpoint to get us past the dynamic linker.
""",
    type="CORE_ADDR",
    name="skip_solib_resolver",
    params=[("CORE_ADDR", "pc")],
    predefault="generic_skip_solib_resolver",
    invalid=False,
)

Method(
    comment="""
Some systems also have trampoline code for returning from shared libs.
""",
    type="int",
    name="in_solib_return_trampoline",
    params=[("CORE_ADDR", "pc"), ("const char *", "name")],
    predefault="generic_in_solib_return_trampoline",
    invalid=False,
)

Method(
    comment="""
Return true if PC lies inside an indirect branch thunk.
""",
    type="bool",
    name="in_indirect_branch_thunk",
    params=[("CORE_ADDR", "pc")],
    predefault="default_in_indirect_branch_thunk",
    invalid=False,
)

Method(
    comment="""
A target might have problems with watchpoints as soon as the stack
frame of the current function has been destroyed.  This mostly happens
as the first action in a function's epilogue.  stack_frame_destroyed_p()
is defined to return a non-zero value if either the given addr is one
instruction after the stack destroying instruction up to the trailing
return instruction or if we can figure out that the stack frame has
already been invalidated regardless of the value of addr.  Targets
which don't suffer from that problem could just let this functionality
untouched.
""",
    type="int",
    name="stack_frame_destroyed_p",
    params=[("CORE_ADDR", "addr")],
    predefault="generic_stack_frame_destroyed_p",
    invalid=False,
)

Function(
    comment="""
Process an ELF symbol in the minimal symbol table in a backend-specific
way.  Normally this hook is supposed to do nothing, however if required,
then this hook can be used to apply tranformations to symbols that are
considered special in some way.  For example the MIPS backend uses it
to interpret `st_other' information to mark compressed code symbols so
that they can be treated in the appropriate manner in the processing of
the main symbol table and DWARF-2 records.
""",
    type="void",
    name="elf_make_msymbol_special",
    params=[("asymbol *", "sym"), ("struct minimal_symbol *", "msym")],
    predicate=True,
)

Function(
    type="void",
    name="coff_make_msymbol_special",
    params=[("int", "val"), ("struct minimal_symbol *", "msym")],
    predefault="default_coff_make_msymbol_special",
    invalid=False,
)

Function(
    comment="""
Process a symbol in the main symbol table in a backend-specific way.
Normally this hook is supposed to do nothing, however if required,
then this hook can be used to apply tranformations to symbols that
are considered special in some way.  This is currently used by the
MIPS backend to make sure compressed code symbols have the ISA bit
set.  This in turn is needed for symbol values seen in GDB to match
the values used at the runtime by the program itself, for function
and label references.
""",
    type="void",
    name="make_symbol_special",
    params=[("struct symbol *", "sym"), ("struct objfile *", "objfile")],
    predefault="default_make_symbol_special",
    invalid=False,
)

Function(
    comment="""
Adjust the address retrieved from a DWARF-2 record other than a line
entry in a backend-specific way.  Normally this hook is supposed to
return the address passed unchanged, however if that is incorrect for
any reason, then this hook can be used to fix the address up in the
required manner.  This is currently used by the MIPS backend to make
sure addresses in FDE, range records, etc. referring to compressed
code have the ISA bit set, matching line information and the symbol
table.
""",
    type="CORE_ADDR",
    name="adjust_dwarf2_addr",
    params=[("CORE_ADDR", "pc")],
    predefault="default_adjust_dwarf2_addr",
    invalid=False,
)

Function(
    comment="""
Adjust the address updated by a line entry in a backend-specific way.
Normally this hook is supposed to return the address passed unchanged,
however in the case of inconsistencies in these records, this hook can
be used to fix them up in the required manner.  This is currently used
by the MIPS backend to make sure all line addresses in compressed code
are presented with the ISA bit set, which is not always the case.  This
in turn ensures breakpoint addresses are correctly matched against the
stop PC.
""",
    type="CORE_ADDR",
    name="adjust_dwarf2_line",
    params=[("CORE_ADDR", "addr"), ("int", "rel")],
    predefault="default_adjust_dwarf2_line",
    invalid=False,
)

Value(
    type="int",
    name="cannot_step_breakpoint",
    predefault="0",
    invalid=False,
)

Value(
    comment="""
See comment in target.h about continuable, steppable and
non-steppable watchpoints.
""",
    type="int",
    name="have_nonsteppable_watchpoint",
    predefault="0",
    invalid=False,
)

Function(
    type="type_instance_flags",
    name="address_class_type_flags",
    params=[("int", "byte_size"), ("int", "dwarf2_addr_class")],
    predicate=True,
)

Method(
    type="const char *",
    name="address_class_type_flags_to_name",
    params=[("type_instance_flags", "type_flags")],
    predicate=True,
)

Method(
    comment="""
Execute vendor-specific DWARF Call Frame Instruction.  OP is the instruction.
FS are passed from the generic execute_cfa_program function.
""",
    type="bool",
    name="execute_dwarf_cfa_vendor_op",
    params=[("gdb_byte", "op"), ("struct dwarf2_frame_state *", "fs")],
    predefault="default_execute_dwarf_cfa_vendor_op",
    invalid=False,
)

Method(
    comment="""
Return the appropriate type_flags for the supplied address class.
This function should return true if the address class was recognized and
type_flags was set, false otherwise.
""",
    type="bool",
    name="address_class_name_to_type_flags",
    params=[("const char *", "name"), ("type_instance_flags *", "type_flags_ptr")],
    predicate=True,
)

Method(
    comment="""
Is a register in a group
""",
    type="int",
    name="register_reggroup_p",
    params=[("int", "regnum"), ("const struct reggroup *", "reggroup")],
    predefault="default_register_reggroup_p",
    invalid=False,
)

Function(
    comment="""
Fetch the pointer to the ith function argument.
""",
    type="CORE_ADDR",
    name="fetch_pointer_argument",
    params=[
        ("frame_info_ptr", "frame"),
        ("int", "argi"),
        ("struct type *", "type"),
    ],
    predicate=True,
)

Method(
    comment="""
Iterate over all supported register notes in a core file.  For each
supported register note section, the iterator must call CB and pass
CB_DATA unchanged.  If REGCACHE is not NULL, the iterator can limit
the supported register note sections based on the current register
values.  Otherwise it should enumerate all supported register note
sections.
""",
    type="void",
    name="iterate_over_regset_sections",
    params=[
        ("iterate_over_regset_sections_cb *", "cb"),
        ("void *", "cb_data"),
        ("const struct regcache *", "regcache"),
    ],
    predicate=True,
)

Method(
    comment="""
Create core file notes
""",
    type="gdb::unique_xmalloc_ptr<char>",
    name="make_corefile_notes",
    params=[("bfd *", "obfd"), ("int *", "note_size")],
    predicate=True,
)

Method(
    comment="""
Find core file memory regions
""",
    type="int",
    name="find_memory_regions",
    params=[("find_memory_region_ftype", "func"), ("void *", "data")],
    predicate=True,
)

Method(
    comment="""
Given a bfd OBFD, segment ADDRESS and SIZE, create a memory tag section to be dumped to a core file
""",
    type="asection *",
    name="create_memtag_section",
    params=[("bfd *", "obfd"), ("CORE_ADDR", "address"), ("size_t", "size")],
    predicate=True,
)

Method(
    comment="""
Given a memory tag section OSEC, fill OSEC's contents with the appropriate tag data
""",
    type="bool",
    name="fill_memtag_section",
    params=[("asection *", "osec")],
    predicate=True,
)

Method(
    comment="""
Decode a memory tag SECTION and return the tags of type TYPE contained in
the memory range [ADDRESS, ADDRESS + LENGTH).
If no tags were found, return an empty vector.
""",
    type="gdb::byte_vector",
    name="decode_memtag_section",
    params=[
        ("bfd_section *", "section"),
        ("int", "type"),
        ("CORE_ADDR", "address"),
        ("size_t", "length"),
    ],
    predicate=True,
)

Method(
    comment="""
Read offset OFFSET of TARGET_OBJECT_LIBRARIES formatted shared libraries list from
core file into buffer READBUF with length LEN.  Return the number of bytes read
(zero indicates failure).
failed, otherwise, return the red length of READBUF.
""",
    type="ULONGEST",
    name="core_xfer_shared_libraries",
    params=[("gdb_byte *", "readbuf"), ("ULONGEST", "offset"), ("ULONGEST", "len")],
    predicate=True,
)

Method(
    comment="""
Read offset OFFSET of TARGET_OBJECT_LIBRARIES_AIX formatted shared
libraries list from core file into buffer READBUF with length LEN.
Return the number of bytes read (zero indicates failure).
""",
    type="ULONGEST",
    name="core_xfer_shared_libraries_aix",
    params=[("gdb_byte *", "readbuf"), ("ULONGEST", "offset"), ("ULONGEST", "len")],
    predicate=True,
)

Method(
    comment="""
How the core target converts a PTID from a core file to a string.
""",
    type="std::string",
    name="core_pid_to_str",
    params=[("ptid_t", "ptid")],
    predicate=True,
)

Method(
    comment="""
How the core target extracts the name of a thread from a core file.
""",
    type="const char *",
    name="core_thread_name",
    params=[("struct thread_info *", "thr")],
    predicate=True,
)

Method(
    comment="""
Read offset OFFSET of TARGET_OBJECT_SIGNAL_INFO signal information
from core file into buffer READBUF with length LEN.  Return the number
of bytes read (zero indicates EOF, a negative value indicates failure).
""",
    type="LONGEST",
    name="core_xfer_siginfo",
    params=[("gdb_byte *", "readbuf"), ("ULONGEST", "offset"), ("ULONGEST", "len")],
    predicate=True,
)

Method(
    comment="""
Read x86 XSAVE layout information from core file into XSAVE_LAYOUT.
Returns true if the layout was read successfully.
""",
    type="bool",
    name="core_read_x86_xsave_layout",
    params=[("x86_xsave_layout &", "xsave_layout")],
    predicate=True,
)

Value(
    comment="""
BFD target to use when generating a core file.
""",
    type="const char *",
    name="gcore_bfd_target",
    predicate=True,
    printer="pstring (gdbarch->gcore_bfd_target)",
)

Value(
    comment="""
If the elements of C++ vtables are in-place function descriptors rather
than normal function pointers (which may point to code or a descriptor),
set this to one.
""",
    type="int",
    name="vtable_function_descriptors",
    predefault="0",
    invalid=False,
)

Value(
    comment="""
Set if the least significant bit of the delta is used instead of the least
significant bit of the pfn for pointers to virtual member functions.
""",
    type="int",
    name="vbit_in_delta",
    invalid=False,
)

Function(
    comment="""
Advance PC to next instruction in order to skip a permanent breakpoint.
""",
    type="void",
    name="skip_permanent_breakpoint",
    params=[("struct regcache *", "regcache")],
    predefault="default_skip_permanent_breakpoint",
    invalid=False,
)

Value(
    comment="""
The maximum length of an instruction on this architecture in bytes.
""",
    type="ULONGEST",
    name="max_insn_length",
    predefault="0",
    predicate=True,
)

Method(
    comment="""
Copy the instruction at FROM to TO, and make any adjustments
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
that case.
""",
    type="displaced_step_copy_insn_closure_up",
    name="displaced_step_copy_insn",
    params=[("CORE_ADDR", "from"), ("CORE_ADDR", "to"), ("struct regcache *", "regs")],
    predicate=True,
)

Method(
    comment="""
Return true if GDB should use hardware single-stepping to execute a displaced
step instruction.  If false, GDB will simply restart execution at the
displaced instruction location, and it is up to the target to ensure GDB will
receive control again (e.g. by placing a software breakpoint instruction into
the displaced instruction buffer).

The default implementation returns false on all targets that provide a
gdbarch_software_single_step routine, and true otherwise.
""",
    type="bool",
    name="displaced_step_hw_singlestep",
    params=[],
    predefault="default_displaced_step_hw_singlestep",
    invalid=False,
)

Method(
    comment="""
Fix up the state after attempting to single-step a displaced
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
see the comments in infrun.c.
""",
    type="void",
    name="displaced_step_fixup",
    params=[
        ("struct displaced_step_copy_insn_closure *", "closure"),
        ("CORE_ADDR", "from"),
        ("CORE_ADDR", "to"),
        ("struct regcache *", "regs"),
        ("bool", "completed_p"),
    ],
    predicate=False,
    predefault="NULL",
    invalid="(gdbarch->displaced_step_copy_insn == nullptr) != (gdbarch->displaced_step_fixup == nullptr)",
)

Method(
    comment="""
Prepare THREAD for it to displaced step the instruction at its current PC.

Throw an exception if any unexpected error happens.
""",
    type="displaced_step_prepare_status",
    name="displaced_step_prepare",
    params=[("thread_info *", "thread"), ("CORE_ADDR &", "displaced_pc")],
    predicate=True,
)

Method(
    comment="""
Clean up after a displaced step of THREAD.

It is possible for the displaced-stepped instruction to have caused
the thread to exit.  The implementation can detect this case by
checking if WS.kind is TARGET_WAITKIND_THREAD_EXITED.
""",
    type="displaced_step_finish_status",
    name="displaced_step_finish",
    params=[("thread_info *", "thread"), ("const target_waitstatus &", "ws")],
    predefault="NULL",
    invalid="(! gdbarch->displaced_step_finish) != (! gdbarch->displaced_step_prepare)",
)

Function(
    comment="""
Return the closure associated to the displaced step buffer that is at ADDR.
""",
    type="const displaced_step_copy_insn_closure *",
    name="displaced_step_copy_insn_closure_by_addr",
    params=[("inferior *", "inf"), ("CORE_ADDR", "addr")],
    predicate=True,
)

Function(
    comment="""
PARENT_INF has forked and CHILD_PTID is the ptid of the child.  Restore the
contents of all displaced step buffers in the child's address space.
""",
    type="void",
    name="displaced_step_restore_all_in_ptid",
    params=[("inferior *", "parent_inf"), ("ptid_t", "child_ptid")],
    invalid=False,
)

Value(
    comment="""
The maximum length in octets required for a displaced-step instruction
buffer.  By default this will be the same as gdbarch::max_insn_length,
but should be overridden for architectures that might expand a
displaced-step instruction to multiple replacement instructions.
""",
    type="ULONGEST",
    name="displaced_step_buffer_length",
    predefault="0",
    postdefault="gdbarch->max_insn_length",
    invalid="gdbarch->displaced_step_buffer_length < gdbarch->max_insn_length",
)

Method(
    comment="""
Relocate an instruction to execute at a different address.  OLDLOC
is the address in the inferior memory where the instruction to
relocate is currently at.  On input, TO points to the destination
where we want the instruction to be copied (and possibly adjusted)
to.  On output, it points to one past the end of the resulting
instruction(s).  The effect of executing the instruction at TO shall
be the same as if executing it at FROM.  For example, call
instructions that implicitly push the return address on the stack
should be adjusted to return to the instruction after OLDLOC;
relative branches, and other PC-relative instructions need the
offset adjusted; etc.
""",
    type="void",
    name="relocate_instruction",
    params=[("CORE_ADDR *", "to"), ("CORE_ADDR", "from")],
    predicate=True,
    predefault="NULL",
)

Function(
    comment="""
Refresh overlay mapped state for section OSECT.
""",
    type="void",
    name="overlay_update",
    params=[("struct obj_section *", "osect")],
    predicate=True,
)

Method(
    type="const struct target_desc *",
    name="core_read_description",
    params=[("struct target_ops *", "target"), ("bfd *", "abfd")],
    predicate=True,
)

Value(
    comment="""
Set if the address in N_SO or N_FUN stabs may be zero.
""",
    type="int",
    name="sofun_address_maybe_missing",
    predefault="0",
    invalid=False,
)

Method(
    comment="""
Parse the instruction at ADDR storing in the record execution log
the registers REGCACHE and memory ranges that will be affected when
the instruction executes, along with their current values.
Return -1 if something goes wrong, 0 otherwise.
""",
    type="int",
    name="process_record",
    params=[("struct regcache *", "regcache"), ("CORE_ADDR", "addr")],
    predicate=True,
)

Method(
    comment="""
Save process state after a signal.
Return -1 if something goes wrong, 0 otherwise.
""",
    type="int",
    name="process_record_signal",
    params=[("struct regcache *", "regcache"), ("enum gdb_signal", "signal")],
    predicate=True,
)

Method(
    comment="""
Signal translation: translate inferior's signal (target's) number
into GDB's representation.  The implementation of this method must
be host independent.  IOW, don't rely on symbols of the NAT_FILE
header (the nm-*.h files), the host <signal.h> header, or similar
headers.  This is mainly used when cross-debugging core files ---
"Live" targets hide the translation behind the target interface
(target_wait, target_resume, etc.).
""",
    type="enum gdb_signal",
    name="gdb_signal_from_target",
    params=[("int", "signo")],
    predicate=True,
)

Method(
    comment="""
Signal translation: translate the GDB's internal signal number into
the inferior's signal (target's) representation.  The implementation
of this method must be host independent.  IOW, don't rely on symbols
of the NAT_FILE header (the nm-*.h files), the host <signal.h>
header, or similar headers.
Return the target signal number if found, or -1 if the GDB internal
signal number is invalid.
""",
    type="int",
    name="gdb_signal_to_target",
    params=[("enum gdb_signal", "signal")],
    predicate=True,
)

Method(
    comment="""
Extra signal info inspection.

Return a type suitable to inspect extra signal information.
""",
    type="struct type *",
    name="get_siginfo_type",
    params=[],
    predicate=True,
)

Method(
    comment="""
Record architecture-specific information from the symbol table.
""",
    type="void",
    name="record_special_symbol",
    params=[("struct objfile *", "objfile"), ("asymbol *", "sym")],
    predicate=True,
)

Method(
    comment="""
Function for the 'catch syscall' feature.
Get architecture-specific system calls information from registers.
""",
    type="LONGEST",
    name="get_syscall_number",
    params=[("thread_info *", "thread")],
    predicate=True,
)

Value(
    comment="""
The filename of the XML syscall for this architecture.
""",
    type="const char *",
    name="xml_syscall_file",
    invalid=False,
    printer="pstring (gdbarch->xml_syscall_file)",
)

Value(
    comment="""
Information about system calls from this architecture
""",
    type="struct syscalls_info *",
    name="syscalls_info",
    invalid=False,
    printer="host_address_to_string (gdbarch->syscalls_info)",
)

Value(
    comment="""
SystemTap related fields and functions.
A NULL-terminated array of prefixes used to mark an integer constant
on the architecture's assembly.
For example, on x86 integer constants are written as:

$10 ;; integer constant 10

in this case, this prefix would be the character `$'.
""",
    type="const char *const *",
    name="stap_integer_prefixes",
    invalid=False,
    printer="pstring_list (gdbarch->stap_integer_prefixes)",
)

Value(
    comment="""
A NULL-terminated array of suffixes used to mark an integer constant
on the architecture's assembly.
""",
    type="const char *const *",
    name="stap_integer_suffixes",
    invalid=False,
    printer="pstring_list (gdbarch->stap_integer_suffixes)",
)

Value(
    comment="""
A NULL-terminated array of prefixes used to mark a register name on
the architecture's assembly.
For example, on x86 the register name is written as:

%eax ;; register eax

in this case, this prefix would be the character `%'.
""",
    type="const char *const *",
    name="stap_register_prefixes",
    invalid=False,
    printer="pstring_list (gdbarch->stap_register_prefixes)",
)

Value(
    comment="""
A NULL-terminated array of suffixes used to mark a register name on
the architecture's assembly.
""",
    type="const char *const *",
    name="stap_register_suffixes",
    invalid=False,
    printer="pstring_list (gdbarch->stap_register_suffixes)",
)

Value(
    comment="""
A NULL-terminated array of prefixes used to mark a register
indirection on the architecture's assembly.
For example, on x86 the register indirection is written as:

(%eax) ;; indirecting eax

in this case, this prefix would be the charater `('.

Please note that we use the indirection prefix also for register
displacement, e.g., `4(%eax)' on x86.
""",
    type="const char *const *",
    name="stap_register_indirection_prefixes",
    invalid=False,
    printer="pstring_list (gdbarch->stap_register_indirection_prefixes)",
)

Value(
    comment="""
A NULL-terminated array of suffixes used to mark a register
indirection on the architecture's assembly.
For example, on x86 the register indirection is written as:

(%eax) ;; indirecting eax

in this case, this prefix would be the charater `)'.

Please note that we use the indirection suffix also for register
displacement, e.g., `4(%eax)' on x86.
""",
    type="const char *const *",
    name="stap_register_indirection_suffixes",
    invalid=False,
    printer="pstring_list (gdbarch->stap_register_indirection_suffixes)",
)

Value(
    comment="""
Prefix(es) used to name a register using GDB's nomenclature.

For example, on PPC a register is represented by a number in the assembly
language (e.g., `10' is the 10th general-purpose register).  However,
inside GDB this same register has an `r' appended to its name, so the 10th
register would be represented as `r10' internally.
""",
    type="const char *",
    name="stap_gdb_register_prefix",
    invalid=False,
    printer="pstring (gdbarch->stap_gdb_register_prefix)",
)

Value(
    comment="""
Suffix used to name a register using GDB's nomenclature.
""",
    type="const char *",
    name="stap_gdb_register_suffix",
    invalid=False,
    printer="pstring (gdbarch->stap_gdb_register_suffix)",
)

Method(
    comment="""
Check if S is a single operand.

Single operands can be:
- Literal integers, e.g. `$10' on x86
- Register access, e.g. `%eax' on x86
- Register indirection, e.g. `(%eax)' on x86
- Register displacement, e.g. `4(%eax)' on x86

This function should check for these patterns on the string
and return 1 if some were found, or zero otherwise.  Please try to match
as much info as you can from the string, i.e., if you have to match
something like `(%', do not match just the `('.
""",
    type="int",
    name="stap_is_single_operand",
    params=[("const char *", "s")],
    predicate=True,
)

Method(
    comment="""
Function used to handle a "special case" in the parser.

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
parser), and should advance the buffer pointer (p->arg).
""",
    type="expr::operation_up",
    name="stap_parse_special_token",
    params=[("struct stap_parse_info *", "p")],
    predicate=True,
)

Method(
    comment="""
Perform arch-dependent adjustments to a register name.

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

The rationale for this can be found at PR breakpoints/24541.
""",
    type="std::string",
    name="stap_adjust_register",
    params=[
        ("struct stap_parse_info *", "p"),
        ("const std::string &", "regname"),
        ("int", "regnum"),
    ],
    predicate=True,
)

Method(
    comment="""
DTrace related functions.
The expression to compute the NARTGth+1 argument to a DTrace USDT probe.
NARG must be >= 0.
""",
    type="expr::operation_up",
    name="dtrace_parse_probe_argument",
    params=[("int", "narg")],
    predicate=True,
)

Method(
    comment="""
True if the given ADDR does not contain the instruction sequence
corresponding to a disabled DTrace is-enabled probe.
""",
    type="int",
    name="dtrace_probe_is_enabled",
    params=[("CORE_ADDR", "addr")],
    predicate=True,
)

Method(
    comment="""
Enable a DTrace is-enabled probe at ADDR.
""",
    type="void",
    name="dtrace_enable_probe",
    params=[("CORE_ADDR", "addr")],
    predicate=True,
)

Method(
    comment="""
Disable a DTrace is-enabled probe at ADDR.
""",
    type="void",
    name="dtrace_disable_probe",
    params=[("CORE_ADDR", "addr")],
    predicate=True,
)

Value(
    comment="""
True if the list of shared libraries is one and only for all
processes, as opposed to a list of shared libraries per inferior.
This usually means that all processes, although may or may not share
an address space, will see the same set of symbols at the same
addresses.
""",
    type="int",
    name="has_global_solist",
    predefault="0",
    invalid=False,
)

Value(
    comment="""
On some targets, even though each inferior has its own private
address space, the debug interface takes care of making breakpoints
visible to all address spaces automatically.  For such cases,
this property should be set to true.
""",
    type="int",
    name="has_global_breakpoints",
    predefault="0",
    invalid=False,
)

Method(
    comment="""
True if inferiors share an address space (e.g., uClinux).
""",
    type="int",
    name="has_shared_address_space",
    params=[],
    predefault="default_has_shared_address_space",
    invalid=False,
)

Method(
    comment="""
True if a fast tracepoint can be set at an address.
""",
    type="int",
    name="fast_tracepoint_valid_at",
    params=[("CORE_ADDR", "addr"), ("std::string *", "msg")],
    predefault="default_fast_tracepoint_valid_at",
    invalid=False,
)

Method(
    comment="""
Guess register state based on tracepoint location.  Used for tracepoints
where no registers have been collected, but there's only one location,
allowing us to guess the PC value, and perhaps some other registers.
On entry, regcache has all registers marked as unavailable.
""",
    type="void",
    name="guess_tracepoint_registers",
    params=[("struct regcache *", "regcache"), ("CORE_ADDR", "addr")],
    predefault="default_guess_tracepoint_registers",
    invalid=False,
)

Function(
    comment="""
Return the "auto" target charset.
""",
    type="const char *",
    name="auto_charset",
    params=[],
    predefault="default_auto_charset",
    invalid=False,
)

Function(
    comment="""
Return the "auto" target wide charset.
""",
    type="const char *",
    name="auto_wide_charset",
    params=[],
    predefault="default_auto_wide_charset",
    invalid=False,
)

Value(
    comment="""
If non-empty, this is a file extension that will be opened in place
of the file extension reported by the shared library list.

This is most useful for toolchains that use a post-linker tool,
where the names of the files run on the target differ in extension
compared to the names of the files GDB should load for debug info.
""",
    type="const char *",
    name="solib_symbols_extension",
    invalid=False,
    printer="pstring (gdbarch->solib_symbols_extension)",
)

Value(
    comment="""
If true, the target OS has DOS-based file system semantics.  That
is, absolute paths include a drive name, and the backslash is
considered a directory separator.
""",
    type="int",
    name="has_dos_based_file_system",
    predefault="0",
    invalid=False,
)

Method(
    comment="""
Generate bytecodes to collect the return address in a frame.
Since the bytecodes run on the target, possibly with GDB not even
connected, the full unwinding machinery is not available, and
typically this function will issue bytecodes for one or more likely
places that the return address may be found.
""",
    type="void",
    name="gen_return_address",
    params=[
        ("struct agent_expr *", "ax"),
        ("struct axs_value *", "value"),
        ("CORE_ADDR", "scope"),
    ],
    predefault="default_gen_return_address",
    invalid=False,
)

Method(
    comment="""
Implement the "info proc" command.
""",
    type="void",
    name="info_proc",
    params=[("const char *", "args"), ("enum info_proc_what", "what")],
    predicate=True,
)

Method(
    comment="""
Implement the "info proc" command for core files.  Noe that there
are two "info_proc"-like methods on gdbarch -- one for core files,
one for live targets.
""",
    type="void",
    name="core_info_proc",
    params=[("const char *", "args"), ("enum info_proc_what", "what")],
    predicate=True,
)

Method(
    comment="""
Iterate over all objfiles in the order that makes the most sense
for the architecture to make global symbol searches.

CB is a callback function passed an objfile to be searched.  The iteration stops
if this function returns nonzero.

If not NULL, CURRENT_OBJFILE corresponds to the objfile being
inspected when the symbol search was requested.
""",
    type="void",
    name="iterate_over_objfiles_in_search_order",
    params=[
        ("iterate_over_objfiles_in_search_order_cb_ftype", "cb"),
        ("struct objfile *", "current_objfile"),
    ],
    predefault="default_iterate_over_objfiles_in_search_order",
    invalid=False,
)

Value(
    comment="""
Ravenscar arch-dependent ops.
""",
    type="struct ravenscar_arch_ops *",
    name="ravenscar_ops",
    predefault="NULL",
    invalid=False,
    printer="host_address_to_string (gdbarch->ravenscar_ops)",
)

Method(
    comment="""
Return non-zero if the instruction at ADDR is a call; zero otherwise.
""",
    type="int",
    name="insn_is_call",
    params=[("CORE_ADDR", "addr")],
    predefault="default_insn_is_call",
    invalid=False,
)

Method(
    comment="""
Return non-zero if the instruction at ADDR is a return; zero otherwise.
""",
    type="int",
    name="insn_is_ret",
    params=[("CORE_ADDR", "addr")],
    predefault="default_insn_is_ret",
    invalid=False,
)

Method(
    comment="""
Return non-zero if the instruction at ADDR is a jump; zero otherwise.
""",
    type="int",
    name="insn_is_jump",
    params=[("CORE_ADDR", "addr")],
    predefault="default_insn_is_jump",
    invalid=False,
)

Method(
    comment="""
Return true if there's a program/permanent breakpoint planted in
memory at ADDRESS, return false otherwise.
""",
    type="bool",
    name="program_breakpoint_here_p",
    params=[("CORE_ADDR", "address")],
    predefault="default_program_breakpoint_here_p",
    invalid=False,
)

Method(
    comment="""
Read one auxv entry from *READPTR, not reading locations >= ENDPTR.
Return 0 if *READPTR is already at the end of the buffer.
Return -1 if there is insufficient buffer for a whole entry.
Return 1 if an entry was read into *TYPEP and *VALP.
""",
    type="int",
    name="auxv_parse",
    params=[
        ("const gdb_byte **", "readptr"),
        ("const gdb_byte *", "endptr"),
        ("CORE_ADDR *", "typep"),
        ("CORE_ADDR *", "valp"),
    ],
    predicate=True,
)

Method(
    comment="""
Print the description of a single auxv entry described by TYPE and VAL
to FILE.
""",
    type="void",
    name="print_auxv_entry",
    params=[("struct ui_file *", "file"), ("CORE_ADDR", "type"), ("CORE_ADDR", "val")],
    predefault="default_print_auxv_entry",
    invalid=False,
)

Method(
    comment="""
Find the address range of the current inferior's vsyscall/vDSO, and
write it to *RANGE.  If the vsyscall's length can't be determined, a
range with zero length is returned.  Returns true if the vsyscall is
found, false otherwise.
""",
    type="int",
    name="vsyscall_range",
    params=[("struct mem_range *", "range")],
    predefault="default_vsyscall_range",
    invalid=False,
)

Function(
    comment="""
Allocate SIZE bytes of PROT protected page aligned memory in inferior.
PROT has GDB_MMAP_PROT_* bitmask format.
Throw an error if it is not possible.  Returned address is always valid.
""",
    type="CORE_ADDR",
    name="infcall_mmap",
    params=[("CORE_ADDR", "size"), ("unsigned", "prot")],
    predefault="default_infcall_mmap",
    invalid=False,
)

Function(
    comment="""
Deallocate SIZE bytes of memory at ADDR in inferior from gdbarch_infcall_mmap.
Print a warning if it is not possible.
""",
    type="void",
    name="infcall_munmap",
    params=[("CORE_ADDR", "addr"), ("CORE_ADDR", "size")],
    predefault="default_infcall_munmap",
    invalid=False,
)

Method(
    comment="""
Return string (caller has to use xfree for it) with options for GCC
to produce code for this target, typically "-m64", "-m32" or "-m31".
These options are put before CU's DW_AT_producer compilation options so that
they can override it.
""",
    type="std::string",
    name="gcc_target_options",
    params=[],
    predefault="default_gcc_target_options",
    invalid=False,
)

Method(
    comment="""
Return a regular expression that matches names used by this
architecture in GNU configury triplets.  The result is statically
allocated and must not be freed.  The default implementation simply
returns the BFD architecture name, which is correct in nearly every
case.
""",
    type="const char *",
    name="gnu_triplet_regexp",
    params=[],
    predefault="default_gnu_triplet_regexp",
    invalid=False,
)

Method(
    comment="""
Return the size in 8-bit bytes of an addressable memory unit on this
architecture.  This corresponds to the number of 8-bit bytes associated to
each address in memory.
""",
    type="int",
    name="addressable_memory_unit_size",
    params=[],
    predefault="default_addressable_memory_unit_size",
    invalid=False,
)

Value(
    comment="""
Functions for allowing a target to modify its disassembler options.
""",
    type="const char *",
    name="disassembler_options_implicit",
    invalid=False,
    printer="pstring (gdbarch->disassembler_options_implicit)",
)

Value(
    type="char **",
    name="disassembler_options",
    invalid=False,
    printer="pstring_ptr (gdbarch->disassembler_options)",
)

Value(
    type="const disasm_options_and_args_t *",
    name="valid_disassembler_options",
    invalid=False,
    printer="host_address_to_string (gdbarch->valid_disassembler_options)",
)

Method(
    comment="""
Type alignment override method.  Return the architecture specific
alignment required for TYPE.  If there is no special handling
required for TYPE then return the value 0, GDB will then apply the
default rules as laid out in gdbtypes.c:type_align.
""",
    type="ULONGEST",
    name="type_align",
    params=[("struct type *", "type")],
    predefault="default_type_align",
    invalid=False,
)

Function(
    comment="""
Return a string containing any flags for the given PC in the given FRAME.
""",
    type="std::string",
    name="get_pc_address_flags",
    params=[("frame_info_ptr", "frame"), ("CORE_ADDR", "pc")],
    predefault="default_get_pc_address_flags",
    invalid=False,
)

Method(
    comment="""
Read core file mappings
""",
    type="void",
    name="read_core_file_mappings",
    params=[
        ("struct bfd *", "cbfd"),
        ("read_core_file_mappings_pre_loop_ftype", "pre_loop_cb"),
        ("read_core_file_mappings_loop_ftype", "loop_cb"),
    ],
    predefault="default_read_core_file_mappings",
    invalid=False,
)

Method(
    comment="""
Return true if the target description for all threads should be read from the
target description core file note(s).  Return false if the target description
for all threads should be inferred from the core file contents/sections.

The corefile's bfd is passed through COREFILE_BFD.
""",
    type="bool",
    name="use_target_description_from_corefile_notes",
    params=[("struct bfd *", "corefile_bfd")],
    predefault="default_use_target_description_from_corefile_notes",
    invalid=False,
)
