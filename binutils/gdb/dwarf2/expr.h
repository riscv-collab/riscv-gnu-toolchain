/* DWARF 2 Expression Evaluator.

   Copyright (C) 2001-2024 Free Software Foundation, Inc.

   Contributed by Daniel Berlin <dan@dberlin.org>.

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

#if !defined (DWARF2EXPR_H)
#define DWARF2EXPR_H

#include "leb128.h"
#include "dwarf2/call-site.h"

struct dwarf2_per_objfile;

/* The location of a value.  */
enum dwarf_value_location
{
  /* The piece is in memory.
     The value on the dwarf stack is its address.  */
  DWARF_VALUE_MEMORY,

  /* The piece is in a register.
     The value on the dwarf stack is the register number.  */
  DWARF_VALUE_REGISTER,

  /* The piece is on the dwarf stack.  */
  DWARF_VALUE_STACK,

  /* The piece is a literal.  */
  DWARF_VALUE_LITERAL,

  /* The piece was optimized out.  */
  DWARF_VALUE_OPTIMIZED_OUT,

  /* The piece is an implicit pointer.  */
  DWARF_VALUE_IMPLICIT_POINTER
};

/* A piece of an object, as recorded by DW_OP_piece or DW_OP_bit_piece.  */
struct dwarf_expr_piece
{
  enum dwarf_value_location location;

  union
  {
    struct
    {
      /* This piece's address, for DWARF_VALUE_MEMORY pieces.  */
      CORE_ADDR addr;
      /* Non-zero if the piece is known to be in memory and on
	 the program's stack.  */
      bool in_stack_memory;
    } mem;

    /* The piece's register number, for DWARF_VALUE_REGISTER pieces.  */
    int regno;

    /* The piece's literal value, for DWARF_VALUE_STACK pieces.  */
    struct value *value;

    struct
    {
      /* A pointer to the data making up this piece,
	 for DWARF_VALUE_LITERAL pieces.  */
      const gdb_byte *data;
      /* The length of the available data.  */
      ULONGEST length;
    } literal;

    /* Used for DWARF_VALUE_IMPLICIT_POINTER.  */
    struct
    {
      /* The referent DIE from DW_OP_implicit_pointer.  */
      sect_offset die_sect_off;
      /* The byte offset into the resulting data.  */
      LONGEST offset;
    } ptr;
  } v;

  /* The length of the piece, in bits.  */
  ULONGEST size;
  /* The piece offset, in bits.  */
  ULONGEST offset;
};

/* The dwarf expression stack.  */

struct dwarf_stack_value
{
  dwarf_stack_value (struct value *value_, int in_stack_memory_)
  : value (value_), in_stack_memory (in_stack_memory_)
  {}

  struct value *value;

  /* True if the piece is in memory and is known to be on the program's stack.
     It is always ok to set this to zero.  This is used, for example, to
     optimize memory access from the target.  It can vastly speed up backtraces
     on long latency connections when "set stack-cache on".  */
  bool in_stack_memory;
};

/* The expression evaluator works with a dwarf_expr_context, describing
   its current state and its callbacks.  */
struct dwarf_expr_context
{
  dwarf_expr_context (dwarf2_per_objfile *per_objfile,
		      int addr_size);
  virtual ~dwarf_expr_context () = default;

  void push_address (CORE_ADDR value, bool in_stack_memory);

  /* Evaluate the expression at ADDR (LEN bytes long) in a given PER_CU
     and FRAME context.

     AS_LVAL defines if the returned struct value is expected to be a
     value (false) or a location description (true).

     TYPE, SUBOBJ_TYPE and SUBOBJ_OFFSET describe the expected struct
     value representation of the evaluation result.

     The ADDR_INFO property can be specified to override the range of
     memory addresses with the passed in buffer.  */
  value *evaluate (const gdb_byte *addr, size_t len, bool as_lval,
		   dwarf2_per_cu_data *per_cu, frame_info_ptr frame,
		   const struct property_addr_info *addr_info = nullptr,
		   struct type *type = nullptr,
		   struct type *subobj_type = nullptr,
		   LONGEST subobj_offset = 0);

private:
  /* The stack of values.  */
  std::vector<dwarf_stack_value> m_stack;

  /* Target address size in bytes.  */
  int m_addr_size = 0;

  /* The current depth of dwarf expression recursion, via DW_OP_call*,
     DW_OP_fbreg, DW_OP_push_object_address, etc., and the maximum
     depth we'll tolerate before raising an error.  */
  int m_recursion_depth = 0, m_max_recursion_depth = 0x100;

  /* Location of the value.  */
  dwarf_value_location m_location = DWARF_VALUE_MEMORY;

  /* For DWARF_VALUE_LITERAL, the current literal value's length and
     data.  For DWARF_VALUE_IMPLICIT_POINTER, LEN is the offset of the
     target DIE of sect_offset kind.  */
  ULONGEST m_len = 0;
  const gdb_byte *m_data = nullptr;

  /* Initialization status of variable: True if variable has been
     initialized; false otherwise.  */
  bool m_initialized = false;

  /* A vector of pieces.

     Each time DW_OP_piece is executed, we add a new element to the
     end of this array, recording the current top of the stack, the
     current location, and the size given as the operand to
     DW_OP_piece.  We then pop the top value from the stack, reset the
     location, and resume evaluation.

     The Dwarf spec doesn't say whether DW_OP_piece pops the top value
     from the stack.  We do, ensuring that clients of this interface
     expecting to see a value left on the top of the stack (say, code
     evaluating frame base expressions or CFA's specified with
     DW_CFA_def_cfa_expression) will get an error if the expression
     actually marks all the values it computes as pieces.

     If an expression never uses DW_OP_piece, num_pieces will be zero.
     (It would be nice to present these cases as expressions yielding
     a single piece, so that callers need not distinguish between the
     no-DW_OP_piece and one-DW_OP_piece cases.  But expressions with
     no DW_OP_piece operations have no value to place in a piece's
     'size' field; the size comes from the surrounding data.  So the
     two cases need to be handled separately.)  */
  std::vector<dwarf_expr_piece> m_pieces;

  /* We evaluate the expression in the context of this objfile.  */
  dwarf2_per_objfile *m_per_objfile;

  /* Frame information used for the evaluation.  */
  frame_info_ptr m_frame = nullptr;

  /* Compilation unit used for the evaluation.  */
  dwarf2_per_cu_data *m_per_cu = nullptr;

  /* Property address info used for the evaluation.  */
  const struct property_addr_info *m_addr_info = nullptr;

  void eval (const gdb_byte *addr, size_t len);
  struct type *address_type () const;
  void push (struct value *value, bool in_stack_memory);
  bool stack_empty_p () const;
  void add_piece (ULONGEST size, ULONGEST offset);
  void execute_stack_op (const gdb_byte *op_ptr, const gdb_byte *op_end);
  void pop ();
  struct value *fetch (int n);
  CORE_ADDR fetch_address (int n);
  bool fetch_in_stack_memory (int n);

  /* Fetch the result of the expression evaluation in a form of
     a struct value, where TYPE, SUBOBJ_TYPE and SUBOBJ_OFFSET
     describe the source level representation of that result.
     AS_LVAL defines if the fetched struct value is expected to
     be a value or a location description.  */
  value *fetch_result (struct type *type, struct type *subobj_type,
		       LONGEST subobj_offset, bool as_lval);

  /* Return the location expression for the frame base attribute, in
     START and LENGTH.  The result must be live until the current
     expression evaluation is complete.  */
  void get_frame_base (const gdb_byte **start, size_t *length);

  /* Return the base type given by the indicated DIE at DIE_CU_OFF.
     This can throw an exception if the DIE is invalid or does not
     represent a base type.  */
  struct type *get_base_type (cu_offset die_cu_off);

  /* Execute DW_AT_location expression for the DWARF expression
     subroutine in the DIE at DIE_CU_OFF in the CU.  Do not touch
     STACK while it being passed to and returned from the called DWARF
     subroutine.  */
  void dwarf_call (cu_offset die_cu_off);

  /* Push on DWARF stack an entry evaluated for DW_TAG_call_site's
     parameter matching KIND and KIND_U at the caller of specified BATON.
     If DEREF_SIZE is not -1 then use DW_AT_call_data_value instead of
     DW_AT_call_value.  */
  void push_dwarf_reg_entry_value (call_site_parameter_kind kind,
				   call_site_parameter_u kind_u,
				   int deref_size);

  /* Read LENGTH bytes at ADDR into BUF.  This method also handles the
     case where a caller of the evaluator passes in some data,
     but with the address being 0.  In this situation, we arrange for
     memory reads to come from the passed-in buffer.  */
  void read_mem (gdb_byte *buf, CORE_ADDR addr, size_t length);
};

/* Return the value of register number REG (a DWARF register number),
   read as an address in a given FRAME.  */
CORE_ADDR read_addr_from_reg (frame_info_ptr frame, int reg);

void dwarf_expr_require_composition (const gdb_byte *, const gdb_byte *,
				     const char *);

int dwarf_block_to_dwarf_reg (const gdb_byte *buf, const gdb_byte *buf_end);

int dwarf_block_to_dwarf_reg_deref (const gdb_byte *buf,
				    const gdb_byte *buf_end,
				    CORE_ADDR *deref_size_return);

int dwarf_block_to_fb_offset (const gdb_byte *buf, const gdb_byte *buf_end,
			      CORE_ADDR *fb_offset_return);

int dwarf_block_to_sp_offset (struct gdbarch *gdbarch, const gdb_byte *buf,
			      const gdb_byte *buf_end,
			      CORE_ADDR *sp_offset_return);

/* Wrappers around the leb128 reader routines to simplify them for our
   purposes.  */

static inline const gdb_byte *
gdb_read_uleb128 (const gdb_byte *buf, const gdb_byte *buf_end,
		  uint64_t *r)
{
  size_t bytes_read = read_uleb128_to_uint64 (buf, buf_end, r);

  if (bytes_read == 0)
    return NULL;
  return buf + bytes_read;
}

static inline const gdb_byte *
gdb_read_sleb128 (const gdb_byte *buf, const gdb_byte *buf_end,
		  int64_t *r)
{
  size_t bytes_read = read_sleb128_to_int64 (buf, buf_end, r);

  if (bytes_read == 0)
    return NULL;
  return buf + bytes_read;
}

static inline const gdb_byte *
gdb_skip_leb128 (const gdb_byte *buf, const gdb_byte *buf_end)
{
  size_t bytes_read = skip_leb128 (buf, buf_end);

  if (bytes_read == 0)
    return NULL;
  return buf + bytes_read;
}

extern const gdb_byte *safe_read_uleb128 (const gdb_byte *buf,
					  const gdb_byte *buf_end,
					  uint64_t *r);

extern const gdb_byte *safe_read_sleb128 (const gdb_byte *buf,
					  const gdb_byte *buf_end,
					  int64_t *r);

extern const gdb_byte *safe_skip_leb128 (const gdb_byte *buf,
					 const gdb_byte *buf_end);

#endif /* DWARF2EXPR_H */
