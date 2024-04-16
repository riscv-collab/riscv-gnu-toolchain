/* Disassemble support for GDB.
   Copyright (C) 2002-2024 Free Software Foundation, Inc.

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

#ifndef DISASM_H
#define DISASM_H

#include "dis-asm.h"
#include "disasm-flags.h"

struct gdbarch;
struct ui_out;
struct ui_file;

/* A wrapper around a disassemble_info and a gdbarch.  This is the core
   set of data that all disassembler sub-classes will need.  This class
   doesn't actually implement the disassembling process, that is something
   that sub-classes will do, with each sub-class doing things slightly
   differently.

   The constructor of this class is protected, you should not create
   instances of this class directly, instead create an instance of an
   appropriate sub-class.  */

struct gdb_disassemble_info
{
  DISABLE_COPY_AND_ASSIGN (gdb_disassemble_info);

  /* Return the gdbarch we are disassembling for.  */
  struct gdbarch *arch ()
  { return m_gdbarch; }

  /* Return a pointer to the disassemble_info, this will be needed for
     passing into the libopcodes disassembler.  */
  struct disassemble_info *disasm_info ()
  { return &m_di; }

protected:

  /* Types for the function callbacks within m_di.  The actual function
     signatures here are taken from include/dis-asm.h.  */
  using read_memory_ftype
    = int (*) (bfd_vma, bfd_byte *, unsigned int, struct disassemble_info *)
	noexcept;
  using memory_error_ftype
    = void (*) (int, bfd_vma, struct disassemble_info *) noexcept;
  using print_address_ftype
    = void (*) (bfd_vma, struct disassemble_info *) noexcept;
  using fprintf_ftype
    = int (*) (void *, const char *, ...) noexcept;
  using fprintf_styled_ftype
    = int (*) (void *, enum disassembler_style, const char *, ...) noexcept;

  /* Constructor, many fields in m_di are initialized from GDBARCH.  The
     remaining arguments are function callbacks that are written into m_di.
     Of these function callbacks FPRINTF_FUNC and FPRINTF_STYLED_FUNC must
     not be nullptr.  If READ_MEMORY_FUNC, MEMORY_ERROR_FUNC, or
     PRINT_ADDRESS_FUNC are nullptr, then that field within m_di is left
     with its default value (see the libopcodes function
     init_disassemble_info for the defaults).  */
  gdb_disassemble_info (struct gdbarch *gdbarch,
			read_memory_ftype read_memory_func,
			memory_error_ftype memory_error_func,
			print_address_ftype print_address_func,
			fprintf_ftype fprintf_func,
			fprintf_styled_ftype fprintf_styled_func);

  /* Destructor.  */
  virtual ~gdb_disassemble_info ();

  /* Stores data required for disassembling instructions in
     opcodes.  */
  struct disassemble_info m_di;

private:
  /* The architecture we are disassembling for.  */
  struct gdbarch *m_gdbarch;

  /* If we own the string in `m_di.disassembler_options', we do so
     using this field.  */
  std::string m_disassembler_options_holder;
};

/* A wrapper around gdb_disassemble_info.  This class adds default
   print functions that are supplied to the disassemble_info within the
   parent class.  These default print functions write to the stream, which
   is also contained in the parent class.

   As with the parent class, the constructor for this class is protected,
   you should not create instances of this class, but create an
   appropriate sub-class instead.  */

struct gdb_printing_disassembler : public gdb_disassemble_info
{
  DISABLE_COPY_AND_ASSIGN (gdb_printing_disassembler);

  /* The stream that disassembler output is being written too.  */
  struct ui_file *stream ()
  { return m_stream; }

protected:

  /* Constructor.  All the arguments are just passed to the parent class.
     We also add the two print functions to the arguments passed to the
     parent.  See gdb_disassemble_info for a description of how the
     arguments are handled.  */
  gdb_printing_disassembler (struct gdbarch *gdbarch,
			     struct ui_file *stream,
			     read_memory_ftype read_memory_func,
			     memory_error_ftype memory_error_func,
			     print_address_ftype print_address_func)
    : gdb_disassemble_info (gdbarch, read_memory_func,
			    memory_error_func, print_address_func,
			    fprintf_func, fprintf_styled_func),
      m_stream (stream)
  {
    gdb_assert (stream != nullptr);
  }

  /* Callback used as the disassemble_info's fprintf_func callback.  The
     DIS_INFO pointer is a pointer to a gdb_printing_disassembler object.
     Content is written to the m_stream extracted from DIS_INFO.  */
  static int fprintf_func (void *dis_info, const char *format, ...) noexcept
    ATTRIBUTE_PRINTF (2, 3);

  /* Callback used as the disassemble_info's fprintf_styled_func callback.
     The DIS_INFO pointer is a pointer to a gdb_printing_disassembler
     object.  Content is written to the m_stream extracted from DIS_INFO.  */
  static int fprintf_styled_func (void *dis_info,
				  enum disassembler_style style,
				  const char *format, ...) noexcept
    ATTRIBUTE_PRINTF(3,4);

  /* Return true if the disassembler is considered inside a comment, false
     otherwise.  */
  bool in_comment_p () const
  { return m_in_comment; }

  /* Set whether the disassembler should be considered as within comment
     text or not.  */
  void set_in_comment (bool c)
  { m_in_comment = c; }

private:

  /* When libopcodes calls the fprintf_func and fprintf_styled_func
     callbacks, a 'void *' argument is passed.  We arrange, through our
     call to init_disassemble_info that this argument will be a pointer to
     a gdb_disassemble_info sub-class, specifically, a
     gdb_printing_disassembler pointer.  This helper function casts
     DIS_INFO to the correct type (with some asserts), and then returns the
     m_stream member variable.  */
  static ui_file *stream_from_gdb_disassemble_info (void *dis_info);

  /* The stream to which output should be sent.  */
  struct ui_file *m_stream;

  /* Are we inside a comment?  This will be set true if the disassembler
     uses styled output and emits a start of comment character.  It is up
     to the code that uses this disassembler class to reset this flag back
     to false at a suitable time (e.g. at the end of every line).  */
  bool m_in_comment = false;
};

/* A basic disassembler that doesn't actually print anything.  */

struct gdb_non_printing_disassembler : public gdb_disassemble_info
{
  gdb_non_printing_disassembler (struct gdbarch *gdbarch,
				 read_memory_ftype read_memory_func)
    : gdb_disassemble_info (gdbarch,
			    read_memory_func,
			    nullptr /* memory_error_func */,
			    nullptr /* print_address_func */,
			    null_fprintf_func,
			    null_fprintf_styled_func)
  { /* Nothing.  */ }

private:

  /* Callback used as the disassemble_info's fprintf_func callback, this
     doesn't write anything to STREAM, but just returns 0.  */
  static int null_fprintf_func (void *stream, const char *format, ...) noexcept
    ATTRIBUTE_PRINTF(2,3);

  /* Callback used as the disassemble_info's fprintf_styled_func callback,
     , this doesn't write anything to STREAM, but just returns 0.  */
  static int null_fprintf_styled_func (void *stream,
				       enum disassembler_style style,
				       const char *format, ...) noexcept
    ATTRIBUTE_PRINTF(3,4);
};

/* This is a helper class, for use as an additional base-class, by some of
   the disassembler classes below.  This class just defines a static method
   for reading from target memory, which can then be used by the various
   disassembler sub-classes.  */

struct gdb_disassembler_memory_reader
{
  /* Implements the read_memory_func disassemble_info callback.  */
  static int dis_asm_read_memory (bfd_vma memaddr, gdb_byte *myaddr,
				  unsigned int len,
				  struct disassemble_info *info) noexcept;
};

/* A non-printing disassemble_info management class.  The disassemble_info
   setup by this class will not print anything to the output stream (there
   is no output stream), and the instruction to be disassembled will be
   read from target memory.  */

struct gdb_non_printing_memory_disassembler
  : public gdb_non_printing_disassembler,
    private gdb_disassembler_memory_reader
{
  /* Constructor.  GDBARCH is the architecture to disassemble for.  */
  gdb_non_printing_memory_disassembler (struct gdbarch *gdbarch)
    :gdb_non_printing_disassembler (gdbarch, dis_asm_read_memory)
  { /* Nothing.  */ }
};

/* A disassembler class that provides 'print_insn', a method for
   disassembling a single instruction to the output stream.  */

struct gdb_disassembler : public gdb_printing_disassembler,
			  private gdb_disassembler_memory_reader
{
  gdb_disassembler (struct gdbarch *gdbarch, struct ui_file *file)
    : gdb_disassembler (gdbarch, file, dis_asm_read_memory)
  { /* Nothing.  */ }

  DISABLE_COPY_AND_ASSIGN (gdb_disassembler);

  /* Disassemble a single instruction at MEMADDR to the ui_file* that was
     passed to the constructor.  If a memory error occurs while
     disassembling this instruction then an error will be thrown.  */
  int print_insn (CORE_ADDR memaddr, int *branch_delay_insns = NULL);

protected:
  gdb_disassembler (struct gdbarch *gdbarch, struct ui_file *file,
		    read_memory_ftype func);

private:
  /* This member variable is given a value by calling dis_asm_memory_error.
     If after calling into the libopcodes disassembler we get back a
     negative value (which indicates an error), then, if this variable has
     a value, we report a memory error to the user, otherwise, we report a
     non-memory error.  */
  std::optional<CORE_ADDR> m_err_memaddr;

  /* The stream to which disassembler output will be written.  */
  ui_file *m_dest;

  /* Disassembler output is built up into this buffer.  Whether this
     string_file is created with styling support or not depends on the
     value of use_ext_lang_colorization_p, as well as whether disassembler
     styling in general is turned on, and also, whether *m_dest supports
     styling or not.  */
  string_file m_buffer;

  /* When true, m_buffer will be created without styling support,
     otherwise, m_buffer will be created with styling support.

     This field will initially be true, but will be set to false if
     ext_lang_colorize_disasm fails to add styling at any time.

     If the extension language is going to add the styling then m_buffer
     should be created without styling support, the extension language will
     then add styling at the end of the disassembly process.

     If the extension language is not going to add the styling, then we
     create m_buffer with styling support, and GDB will add minimal styling
     (currently just to addresses and symbols) as it goes.  */
  static bool use_ext_lang_colorization_p;

  static void dis_asm_memory_error (int err, bfd_vma memaddr,
				    struct disassemble_info *info) noexcept;
  static void dis_asm_print_address (bfd_vma addr,
				     struct disassemble_info *info) noexcept;

  /* Return true if we should use the extension language to apply
     disassembler styling.  This requires disassembler styling to be on
     (i.e. 'set style disassembler enabled on'), the output stream needs to
     support styling, and libopcode styling needs to be either off, or not
     supported for the current architecture (libopcodes is used in
     preference to the extension language method).  */
  bool use_ext_lang_for_styling () const;

  /* Return true if we should use libopcodes to apply disassembler styling.
     This requires disassembler styling to be on (i.e. 'set style
     disassembler enabled on'), the output stream needs to support styling,
     and libopcodes styling needs to be supported for the current
     architecture, and not disabled by the user.  */
  bool use_libopcodes_for_styling () const;
};

/* An instruction to be disassembled.  */

struct disasm_insn
{
  /* The address of the memory containing the instruction.  */
  CORE_ADDR addr;

  /* An optional instruction number.  If non-zero, it is printed first.  */
  unsigned int number;

  /* True if the instruction was executed speculatively.  */
  unsigned int is_speculative:1;
};

extern void gdb_disassembly (struct gdbarch *gdbarch, struct ui_out *uiout,
			     gdb_disassembly_flags flags, int how_many,
			     CORE_ADDR low, CORE_ADDR high);

/* Print the instruction at address MEMADDR in debugged memory,
   on STREAM.  Returns the length of the instruction, in bytes,
   and, if requested, the number of branch delay slot instructions.  */

extern int gdb_print_insn (struct gdbarch *gdbarch, CORE_ADDR memaddr,
			   struct ui_file *stream, int *branch_delay_insns);

/* Class used to pretty-print instructions.  */

class gdb_pretty_print_disassembler
{
public:
  explicit gdb_pretty_print_disassembler (struct gdbarch *gdbarch,
					  struct ui_out *uiout)
    : m_uiout (uiout),
      m_insn_stb (uiout->can_emit_style_escape ()),
      m_di (gdbarch, &m_insn_stb)
  {}

  /* Prints the instruction INSN into the saved ui_out and returns the
     length of the printed instruction in bytes.  */
  int pretty_print_insn (const struct disasm_insn *insn,
			 gdb_disassembly_flags flags);

private:
  /* Returns the architecture used for disassembling.  */
  struct gdbarch *arch () { return m_di.arch (); }

  /* The ui_out that is used by pretty_print_insn.  */
  struct ui_out *m_uiout;

  /* The buffer used to build the instruction string.  The
     disassembler is initialized with this stream.  */
  string_file m_insn_stb;

  /* The disassembler used for instruction printing.  */
  gdb_disassembler m_di;

  /* The buffer used to build the raw opcodes string.  */
  string_file m_opcode_stb;

  /* The buffer used to hold the opcode bytes (if required).  */
  gdb::byte_vector m_opcode_data;
};

/* Return the length in bytes of the instruction at address MEMADDR in
   debugged memory.  */

extern int gdb_insn_length (struct gdbarch *gdbarch, CORE_ADDR memaddr);

/* Return the length in bytes of INSN, originally at MEMADDR.  MAX_LEN
   is the size of the buffer containing INSN.  */

extern int gdb_buffered_insn_length (struct gdbarch *gdbarch,
				     const gdb_byte *insn, int max_len,
				     CORE_ADDR memaddr);

/* Returns GDBARCH's disassembler options.  */

extern char *get_disassembler_options (struct gdbarch *gdbarch);

/* Sets the active gdbarch's disassembler options to OPTIONS.  */

extern void set_disassembler_options (const char *options);

#endif
