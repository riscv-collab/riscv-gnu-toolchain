/* Disassemble support for GDB.

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

#include "defs.h"
#include "arch-utils.h"
#include "target.h"
#include "value.h"
#include "ui-out.h"
#include "disasm.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "dis-asm.h"
#include "source.h"
#include "gdbsupport/gdb-safe-ctype.h"
#include <algorithm>
#include <optional>
#include "valprint.h"
#include "cli/cli-style.h"
#include "objfiles.h"
#include "inferior.h"

/* Disassemble functions.
   FIXME: We should get rid of all the duplicate code in gdb that does
   the same thing: disassemble_command() and the gdbtk variation.  */

/* This variable is used to hold the prospective disassembler_options value
   which is set by the "set disassembler_options" command.  */
static std::string prospective_options;

/* When this is true we will try to use libopcodes to provide styling to
   the disassembler output.  */

static bool use_libopcodes_styling = true;

/* To support the set_use_libopcodes_styling function we have a second
   variable which is connected to the actual set/show option.  */

static bool use_libopcodes_styling_option = use_libopcodes_styling;

/* The "maint show libopcodes-styling enabled" command.  */

static void
show_use_libopcodes_styling  (struct ui_file *file, int from_tty,
			      struct cmd_list_element *c,
			      const char *value)
{
  gdbarch *arch = current_inferior ()->arch ();
  gdb_non_printing_memory_disassembler dis (arch);
  bool supported = dis.disasm_info ()->created_styled_output;

  if (supported || !use_libopcodes_styling)
    gdb_printf (file, _("Use of libopcodes styling support is \"%s\".\n"),
		value);
  else
    {
      /* Use of libopcodes styling is not supported, and the user has this
	 turned on!  */
      gdb_printf (file, _("Use of libopcodes styling support is \"off\""
			  " (not supported on architecture \"%s\")\n"),
		  gdbarch_bfd_arch_info (arch)->printable_name);
    }
}

/* The "maint set libopcodes-styling enabled" command.  */

static void
set_use_libopcodes_styling (const char *args, int from_tty,
			    struct cmd_list_element *c)
{
  gdbarch *arch = current_inferior ()->arch ();
  gdb_non_printing_memory_disassembler dis (arch);
  bool supported = dis.disasm_info ()->created_styled_output;

  /* If the current architecture doesn't support libopcodes styling then we
     give an error here, but leave the underlying setting enabled.  This
     means that if the user switches to an architecture that does support
     libopcodes styling the setting will be enabled.  */

  if (use_libopcodes_styling_option && !supported)
    {
      use_libopcodes_styling_option = use_libopcodes_styling;
      error (_("Use of libopcodes styling not supported on architecture \"%s\"."),
	     gdbarch_bfd_arch_info (arch)->printable_name);
    }
  else
    use_libopcodes_styling = use_libopcodes_styling_option;
}

/* This structure is used to store line number information for the
   deprecated /m option.
   We need a different sort of line table from the normal one cuz we can't
   depend upon implicit line-end pc's for lines to do the
   reordering in this function.  */

struct deprecated_dis_line_entry
{
  int line;
  CORE_ADDR start_pc;
  CORE_ADDR end_pc;
};

/* This Structure is used to store line number information.
   We need a different sort of line table from the normal one cuz we can't
   depend upon implicit line-end pc's for lines to do the
   reordering in this function.  */

struct dis_line_entry
{
  struct symtab *symtab;
  int line;
};

/* Hash function for dis_line_entry.  */

static hashval_t
hash_dis_line_entry (const void *item)
{
  const struct dis_line_entry *dle = (const struct dis_line_entry *) item;

  return htab_hash_pointer (dle->symtab) + dle->line;
}

/* Equal function for dis_line_entry.  */

static int
eq_dis_line_entry (const void *item_lhs, const void *item_rhs)
{
  const struct dis_line_entry *lhs = (const struct dis_line_entry *) item_lhs;
  const struct dis_line_entry *rhs = (const struct dis_line_entry *) item_rhs;

  return (lhs->symtab == rhs->symtab
	  && lhs->line == rhs->line);
}

/* Create the table to manage lines for mixed source/disassembly.  */

static htab_t
allocate_dis_line_table (void)
{
  return htab_create_alloc (41,
			    hash_dis_line_entry, eq_dis_line_entry,
			    xfree, xcalloc, xfree);
}

/* Add a new dis_line_entry containing SYMTAB and LINE to TABLE.  */

static void
add_dis_line_entry (htab_t table, struct symtab *symtab, int line)
{
  void **slot;
  struct dis_line_entry dle, *dlep;

  dle.symtab = symtab;
  dle.line = line;
  slot = htab_find_slot (table, &dle, INSERT);
  if (*slot == NULL)
    {
      dlep = XNEW (struct dis_line_entry);
      dlep->symtab = symtab;
      dlep->line = line;
      *slot = dlep;
    }
}

/* Return non-zero if SYMTAB, LINE are in TABLE.  */

static int
line_has_code_p (htab_t table, struct symtab *symtab, int line)
{
  struct dis_line_entry dle;

  dle.symtab = symtab;
  dle.line = line;
  return htab_find (table, &dle) != NULL;
}

/* Wrapper of target_read_code.  */

int
gdb_disassembler_memory_reader::dis_asm_read_memory
  (bfd_vma memaddr, gdb_byte *myaddr, unsigned int len,
   struct disassemble_info *info) noexcept
{
  return target_read_code (memaddr, myaddr, len);
}

/* Wrapper of memory_error.  */

void
gdb_disassembler::dis_asm_memory_error
  (int err, bfd_vma memaddr, struct disassemble_info *info) noexcept
{
  gdb_disassembler *self
    = static_cast<gdb_disassembler *>(info->application_data);

  self->m_err_memaddr.emplace (memaddr);
}

/* Wrapper of print_address.  */

void
gdb_disassembler::dis_asm_print_address
  (bfd_vma addr, struct disassemble_info *info) noexcept
{
  gdb_disassembler *self
    = static_cast<gdb_disassembler *>(info->application_data);

  if (self->in_comment_p ())
    {
      /* Calling 'print_address' might add styling to the output (based on
	 the properties of the stream we're writing too).  This is usually
	 fine, but if we are in an assembler comment then we'd prefer to
	 have the comment style, rather than the default address style.

	 Print the address into a temporary buffer which doesn't support
	 styling, then reprint this unstyled address with the default text
	 style.

	 As we are inside a comment right now, the standard print routine
	 will ensure that the comment is printed to the user with a
	 suitable comment style.  */
      string_file tmp;
      print_address (self->arch (), addr, &tmp);
      self->fprintf_styled_func (self, dis_style_text, "%s", tmp.c_str ());
    }
  else
    print_address (self->arch (), addr, self->stream ());
}

/* See disasm.h.  */

ui_file *
gdb_printing_disassembler::stream_from_gdb_disassemble_info (void *dis_info)
{
  gdb_disassemble_info *di = (gdb_disassemble_info *) dis_info;
  gdb_printing_disassembler *dis
    = gdb::checked_static_cast<gdb_printing_disassembler *> (di);
  ui_file *stream = dis->stream ();
  gdb_assert (stream != nullptr);
  return stream;
}

/* Format disassembler output to STREAM.  */

int
gdb_printing_disassembler::fprintf_func (void *dis_info,
					 const char *format, ...) noexcept
{
  ui_file *stream = stream_from_gdb_disassemble_info (dis_info);

  va_list args;
  va_start (args, format);
  gdb_vprintf (stream, format, args);
  va_end (args);

  /* Something non -ve.  */
  return 0;
}

/* See disasm.h.  */

int
gdb_printing_disassembler::fprintf_styled_func
  (void *dis_info, enum disassembler_style style,
   const char *format, ...) noexcept
{
  ui_file *stream = stream_from_gdb_disassemble_info (dis_info);
  gdb_printing_disassembler *dis = (gdb_printing_disassembler *) dis_info;

  va_list args;
  va_start (args, format);
  std::string content = string_vprintf (format, args);
  va_end (args);

  /* Once in a comment then everything should be styled as a comment.  */
  if (style == dis_style_comment_start)
    dis->set_in_comment (true);
  if (dis->in_comment_p ())
    style = dis_style_comment_start;

  /* Now print the content with the correct style.  */
  const char *txt = content.c_str ();
  switch (style)
    {
    case dis_style_mnemonic:
    case dis_style_sub_mnemonic:
    case dis_style_assembler_directive:
      fputs_styled (txt, disasm_mnemonic_style.style (), stream);
      break;

    case dis_style_register:
      fputs_styled (txt, disasm_register_style.style (), stream);
      break;

    case dis_style_immediate:
    case dis_style_address_offset:
      fputs_styled (txt, disasm_immediate_style.style (), stream);
      break;

    case dis_style_address:
      fputs_styled (txt, address_style.style (), stream);
      break;

    case dis_style_symbol:
      fputs_styled (txt, function_name_style.style (), stream);
      break;

    case dis_style_comment_start:
      fputs_styled (txt, disasm_comment_style.style (), stream);
      break;

    case dis_style_text:
      gdb_puts (txt, stream);
      break;
    }

  /* Something non -ve.  */
  return 0;
}

static bool
line_is_less_than (const deprecated_dis_line_entry &mle1,
		   const deprecated_dis_line_entry &mle2)
{
  bool val;

  /* End of sequence markers have a line number of 0 but don't want to
     be sorted to the head of the list, instead sort by PC.  */
  if (mle1.line == 0 || mle2.line == 0)
    {
      if (mle1.start_pc != mle2.start_pc)
	val = mle1.start_pc < mle2.start_pc;
      else
	val = mle1.line < mle2.line;
    }
  else
    {
      if (mle1.line != mle2.line)
	val = mle1.line < mle2.line;
      else
	val = mle1.start_pc < mle2.start_pc;
    }
  return val;
}

/* See disasm.h.  */

int
gdb_pretty_print_disassembler::pretty_print_insn (const struct disasm_insn *insn,
						  gdb_disassembly_flags flags)
{
  /* parts of the symbolic representation of the address */
  int unmapped;
  int offset;
  int line;
  int size;
  CORE_ADDR pc;
  struct gdbarch *gdbarch = arch ();

  {
    ui_out_emit_tuple tuple_emitter (m_uiout, NULL);
    pc = insn->addr;

    if (insn->number != 0)
      {
	m_uiout->field_unsigned ("insn-number", insn->number);
	m_uiout->text ("\t");
      }

    if ((flags & DISASSEMBLY_SPECULATIVE) != 0)
      {
	if (insn->is_speculative)
	  {
	    m_uiout->field_string ("is-speculative", "?");

	    /* The speculative execution indication overwrites the first
	       character of the PC prefix.
	       We assume a PC prefix length of 3 characters.  */
	    if ((flags & DISASSEMBLY_OMIT_PC) == 0)
	      m_uiout->text (pc_prefix (pc) + 1);
	    else
	      m_uiout->text ("  ");
	  }
	else if ((flags & DISASSEMBLY_OMIT_PC) == 0)
	  m_uiout->text (pc_prefix (pc));
	else
	  m_uiout->text ("   ");
      }
    else if ((flags & DISASSEMBLY_OMIT_PC) == 0)
      m_uiout->text (pc_prefix (pc));
    m_uiout->field_core_addr ("address", gdbarch, pc);

    std::string name, filename;
    bool omit_fname = ((flags & DISASSEMBLY_OMIT_FNAME) != 0);
    if (!build_address_symbolic (gdbarch, pc, false, omit_fname, &name,
				 &offset, &filename, &line, &unmapped))
      {
	/* We don't care now about line, filename and unmapped.  But we might in
	   the future.  */
	m_uiout->text (" <");
	if (!omit_fname)
	  m_uiout->field_string ("func-name", name,
				 function_name_style.style ());
	/* For negative offsets, avoid displaying them as +-N; the sign of
	   the offset takes the place of the "+" here.  */
	if (offset >= 0)
	  m_uiout->text ("+");
	m_uiout->field_signed ("offset", offset);
	m_uiout->text (">:\t");
      }
    else
      m_uiout->text (":\t");

    /* Clear the buffer into which we will disassemble the instruction.  */
    m_insn_stb.clear ();

    /* A helper function to write the M_INSN_STB buffer, followed by a
       newline.  This can be called in a couple of situations.  */
    auto write_out_insn_buffer = [&] ()
    {
      m_uiout->field_stream ("inst", m_insn_stb);
      m_uiout->text ("\n");
    };

    try
      {
	/* Now we can disassemble the instruction.  If the disassembler
	   returns a negative value this indicates an error and is handled
	   within the print_insn call, resulting in an exception being
	   thrown.  Returning zero makes no sense, as this indicates we
	   disassembled something successfully, but it was something of no
	   size?  */
	size = m_di.print_insn (pc);
	gdb_assert (size > 0);
      }
    catch (const gdb_exception &)
      {
	/* An exception was thrown while disassembling the instruction.
	   However, the disassembler might still have written something
	   out, so ensure that we flush the instruction buffer before
	   rethrowing the exception.  We can't perform this write from an
	   object destructor as the write itself might throw an exception
	   if the pager kicks in, and the user selects quit.  */
	write_out_insn_buffer ();
	throw;
      }

    if ((flags & (DISASSEMBLY_RAW_INSN | DISASSEMBLY_RAW_BYTES)) != 0)
      {
	/* Build the opcodes using a temporary stream so we can
	   write them out in a single go for the MI.  */
	m_opcode_stb.clear ();

	/* Read the instruction opcode data.  */
	m_opcode_data.resize (size);
	read_code (pc, m_opcode_data.data (), size);

	/* The disassembler provides information about the best way to
	   display the instruction bytes to the user.  We provide some sane
	   defaults in case the disassembler gets it wrong.  */
	const struct disassemble_info *di = m_di.disasm_info ();
	int bytes_per_line = std::max (di->bytes_per_line, size);
	int bytes_per_chunk = std::max (di->bytes_per_chunk, 1);

	/* If the user has requested the instruction bytes be displayed
	   byte at a time, then handle that here.  Also, if the instruction
	   is not a multiple of the chunk size (which probably indicates a
	   disassembler problem) then avoid that causing display problems
	   by switching to byte at a time mode.  */
	if ((flags & DISASSEMBLY_RAW_BYTES) != 0
	    || (size % bytes_per_chunk) != 0)
	  bytes_per_chunk = 1;

	/* Print the instruction opcodes bytes, grouped into chunks.  */
	for (int i = 0; i < size; i += bytes_per_chunk)
	  {
	    if (i > 0)
	      m_opcode_stb.puts (" ");

	    if (di->display_endian == BFD_ENDIAN_LITTLE)
	      {
		for (int k = bytes_per_chunk; k-- != 0; )
		  m_opcode_stb.printf ("%02x", (unsigned) m_opcode_data[i + k]);
	      }
	    else
	      {
		for (int k = 0; k < bytes_per_chunk; k++)
		  m_opcode_stb.printf ("%02x", (unsigned) m_opcode_data[i + k]);
	      }
	  }

	/* Calculate required padding.  */
	int nspaces = 0;
	for (int i = size; i < bytes_per_line; i += bytes_per_chunk)
	  {
	    if (i > size)
	      nspaces++;
	    nspaces += bytes_per_chunk * 2;
	  }

	m_uiout->field_stream ("opcodes", m_opcode_stb);
	m_uiout->spaces (nspaces);
	m_uiout->text ("\t");
      }

    /* Disassembly was a success, write out the instruction buffer.  */
    write_out_insn_buffer ();
  }

  return size;
}

static int
dump_insns (struct gdbarch *gdbarch,
	    struct ui_out *uiout, CORE_ADDR low, CORE_ADDR high,
	    int how_many, gdb_disassembly_flags flags, CORE_ADDR *end_pc)
{
  struct disasm_insn insn;
  int num_displayed = 0;

  memset (&insn, 0, sizeof (insn));
  insn.addr = low;

  gdb_pretty_print_disassembler disasm (gdbarch, uiout);

  while (insn.addr < high && (how_many < 0 || num_displayed < how_many))
    {
      int size;

      size = disasm.pretty_print_insn (&insn, flags);
      if (size <= 0)
	break;

      ++num_displayed;
      insn.addr += size;

      /* Allow user to bail out with ^C.  */
      QUIT;
    }

  if (end_pc != NULL)
    *end_pc = insn.addr;

  return num_displayed;
}

/* The idea here is to present a source-O-centric view of a
   function to the user.  This means that things are presented
   in source order, with (possibly) out of order assembly
   immediately following.

   N.B. This view is deprecated.  */

static void
do_mixed_source_and_assembly_deprecated
  (struct gdbarch *gdbarch, struct ui_out *uiout,
   struct symtab *symtab,
   CORE_ADDR low, CORE_ADDR high,
   int how_many, gdb_disassembly_flags flags)
{
  int newlines = 0;
  int nlines;
  const struct linetable_entry *le;
  struct deprecated_dis_line_entry *mle;
  struct symtab_and_line sal;
  int i;
  int out_of_order = 0;
  int next_line = 0;
  int num_displayed = 0;
  print_source_lines_flags psl_flags = 0;

  gdb_assert (symtab != nullptr && symtab->linetable () != nullptr);

  nlines = symtab->linetable ()->nitems;
  le = symtab->linetable ()->item;

  if (flags & DISASSEMBLY_FILENAME)
    psl_flags |= PRINT_SOURCE_LINES_FILENAME;

  mle = (struct deprecated_dis_line_entry *)
    alloca (nlines * sizeof (struct deprecated_dis_line_entry));

  struct objfile *objfile = symtab->compunit ()->objfile ();

  unrelocated_addr unrel_low
    = unrelocated_addr (low - objfile->text_section_offset ());
  unrelocated_addr unrel_high
    = unrelocated_addr (high - objfile->text_section_offset ());

  /* Copy linetable entries for this function into our data
     structure, creating end_pc's and setting out_of_order as
     appropriate.  */

  /* First, skip all the preceding functions.  */

  for (i = 0; i < nlines - 1 && le[i].unrelocated_pc () < unrel_low; i++);

  /* Now, copy all entries before the end of this function.  */

  for (; i < nlines - 1 && le[i].unrelocated_pc () < unrel_high; i++)
    {
      if (le[i] == le[i + 1])
	continue;		/* Ignore duplicates.  */

      /* Skip any end-of-function markers.  */
      if (le[i].line == 0)
	continue;

      mle[newlines].line = le[i].line;
      if (le[i].line > le[i + 1].line)
	out_of_order = 1;
      mle[newlines].start_pc = le[i].pc (objfile);
      mle[newlines].end_pc = le[i + 1].pc (objfile);
      newlines++;
    }

  /* If we're on the last line, and it's part of the function,
     then we need to get the end pc in a special way.  */

  if (i == nlines - 1 && le[i].unrelocated_pc () < unrel_high)
    {
      mle[newlines].line = le[i].line;
      mle[newlines].start_pc = le[i].pc (objfile);
      sal = find_pc_line (le[i].pc (objfile), 0);
      mle[newlines].end_pc = sal.end;
      newlines++;
    }

  /* Now, sort mle by line #s (and, then by addresses within lines).  */

  if (out_of_order)
    std::sort (mle, mle + newlines, line_is_less_than);

  /* Now, for each line entry, emit the specified lines (unless
     they have been emitted before), followed by the assembly code
     for that line.  */

  ui_out_emit_list asm_insns_list (uiout, "asm_insns");

  std::optional<ui_out_emit_tuple> outer_tuple_emitter;
  std::optional<ui_out_emit_list> inner_list_emitter;

  for (i = 0; i < newlines; i++)
    {
      /* Print out everything from next_line to the current line.  */
      if (mle[i].line >= next_line)
	{
	  if (next_line != 0)
	    {
	      /* Just one line to print.  */
	      if (next_line == mle[i].line)
		{
		  outer_tuple_emitter.emplace (uiout, "src_and_asm_line");
		  print_source_lines (symtab, next_line, mle[i].line + 1, psl_flags);
		}
	      else
		{
		  /* Several source lines w/o asm instructions associated.  */
		  for (; next_line < mle[i].line; next_line++)
		    {
		      ui_out_emit_tuple tuple_emitter (uiout,
						       "src_and_asm_line");
		      print_source_lines (symtab, next_line, next_line + 1,
					  psl_flags);
		      ui_out_emit_list temp_list_emitter (uiout,
							  "line_asm_insn");
		    }
		  /* Print the last line and leave list open for
		     asm instructions to be added.  */
		  outer_tuple_emitter.emplace (uiout, "src_and_asm_line");
		  print_source_lines (symtab, next_line, mle[i].line + 1, psl_flags);
		}
	    }
	  else
	    {
	      outer_tuple_emitter.emplace (uiout, "src_and_asm_line");
	      print_source_lines (symtab, mle[i].line, mle[i].line + 1, psl_flags);
	    }

	  next_line = mle[i].line + 1;
	  inner_list_emitter.emplace (uiout, "line_asm_insn");
	}

      num_displayed += dump_insns (gdbarch, uiout,
				   mle[i].start_pc, mle[i].end_pc,
				   how_many, flags, NULL);

      /* When we've reached the end of the mle array, or we've seen the last
	 assembly range for this source line, close out the list/tuple.  */
      if (i == (newlines - 1) || mle[i + 1].line > mle[i].line)
	{
	  inner_list_emitter.reset ();
	  outer_tuple_emitter.reset ();
	  uiout->text ("\n");
	}
      if (how_many >= 0 && num_displayed >= how_many)
	break;
    }
}

/* The idea here is to present a source-O-centric view of a
   function to the user.  This means that things are presented
   in source order, with (possibly) out of order assembly
   immediately following.  */

static void
do_mixed_source_and_assembly (struct gdbarch *gdbarch,
			      struct ui_out *uiout,
			      struct symtab *main_symtab,
			      CORE_ADDR low, CORE_ADDR high,
			      int how_many, gdb_disassembly_flags flags)
{
  const struct linetable_entry *le, *first_le;
  int i, nlines;
  int num_displayed = 0;
  print_source_lines_flags psl_flags = 0;
  CORE_ADDR pc;
  struct symtab *last_symtab;
  int last_line;

  gdb_assert (main_symtab != NULL && main_symtab->linetable () != NULL);

  /* First pass: collect the list of all source files and lines.
     We do this so that we can only print lines containing code once.
     We try to print the source text leading up to the next instruction,
     but if that text is for code that will be disassembled later, then
     we'll want to defer printing it until later with its associated code.  */

  htab_up dis_line_table (allocate_dis_line_table ());

  struct objfile *objfile = main_symtab->compunit ()->objfile ();

  unrelocated_addr unrel_low
    = unrelocated_addr (low - objfile->text_section_offset ());
  unrelocated_addr unrel_high
    = unrelocated_addr (high - objfile->text_section_offset ());

  pc = low;

  /* The prologue may be empty, but there may still be a line number entry
     for the opening brace which is distinct from the first line of code.
     If the prologue has been eliminated find_pc_line may return the source
     line after the opening brace.  We still want to print this opening brace.
     first_le is used to implement this.  */

  nlines = main_symtab->linetable ()->nitems;
  le = main_symtab->linetable ()->item;
  first_le = NULL;

  /* Skip all the preceding functions.  */
  for (i = 0; i < nlines && le[i].unrelocated_pc () < unrel_low; i++)
    continue;

  if (i < nlines && le[i].unrelocated_pc () < unrel_high)
    first_le = &le[i];

  /* Add lines for every pc value.  */
  while (pc < high)
    {
      struct symtab_and_line sal;
      int length;

      sal = find_pc_line (pc, 0);
      length = gdb_insn_length (gdbarch, pc);
      pc += length;

      if (sal.symtab != NULL)
	add_dis_line_entry (dis_line_table.get (), sal.symtab, sal.line);
    }

  /* Second pass: print the disassembly.

     Output format, from an MI perspective:
       The result is a ui_out list, field name "asm_insns", where elements have
       name "src_and_asm_line".
       Each element is a tuple of source line specs (field names line, file,
       fullname), and field "line_asm_insn" which contains the disassembly.
       Field "line_asm_insn" is a list of tuples: address, func-name, offset,
       opcodes, inst.

     CLI output works on top of this because MI ignores ui_out_text output,
     which is where we put file name and source line contents output.

     Emitter usage:
     asm_insns_emitter
       Handles the outer "asm_insns" list.
     tuple_emitter
       The tuples for each group of consecutive disassemblies.
     list_emitter
       List of consecutive source lines or disassembled insns.  */

  if (flags & DISASSEMBLY_FILENAME)
    psl_flags |= PRINT_SOURCE_LINES_FILENAME;

  ui_out_emit_list asm_insns_emitter (uiout, "asm_insns");

  std::optional<ui_out_emit_tuple> tuple_emitter;
  std::optional<ui_out_emit_list> list_emitter;

  last_symtab = NULL;
  last_line = 0;
  pc = low;

  while (pc < high)
    {
      struct symtab_and_line sal;
      CORE_ADDR end_pc;
      int start_preceding_line_to_display = 0;
      int end_preceding_line_to_display = 0;
      int new_source_line = 0;

      sal = find_pc_line (pc, 0);

      if (sal.symtab != last_symtab)
	{
	  /* New source file.  */
	  new_source_line = 1;

	  /* If this is the first line of output, check for any preceding
	     lines.  */
	  if (last_line == 0
	      && first_le != NULL
	      && first_le->line < sal.line)
	    {
	      start_preceding_line_to_display = first_le->line;
	      end_preceding_line_to_display = sal.line;
	    }
	}
      else
	{
	  /* Same source file as last time.  */
	  if (sal.symtab != NULL)
	    {
	      if (sal.line > last_line + 1 && last_line != 0)
		{
		  int l;

		  /* Several preceding source lines.  Print the trailing ones
		     not associated with code that we'll print later.  */
		  for (l = sal.line - 1; l > last_line; --l)
		    {
		      if (line_has_code_p (dis_line_table.get (),
					   sal.symtab, l))
			break;
		    }
		  if (l < sal.line - 1)
		    {
		      start_preceding_line_to_display = l + 1;
		      end_preceding_line_to_display = sal.line;
		    }
		}
	      if (sal.line != last_line)
		new_source_line = 1;
	      else
		{
		  /* Same source line as last time.  This can happen, depending
		     on the debug info.  */
		}
	    }
	}

      if (new_source_line)
	{
	  /* Skip the newline if this is the first instruction.  */
	  if (pc > low)
	    uiout->text ("\n");
	  if (tuple_emitter.has_value ())
	    {
	      gdb_assert (list_emitter.has_value ());
	      list_emitter.reset ();
	      tuple_emitter.reset ();
	    }
	  if (sal.symtab != last_symtab
	      && !(flags & DISASSEMBLY_FILENAME))
	    {
	      /* Remember MI ignores ui_out_text.
		 We don't have to do anything here for MI because MI
		 output includes the source specs for each line.  */
	      if (sal.symtab != NULL)
		{
		  uiout->text (symtab_to_filename_for_display (sal.symtab));
		}
	      else
		uiout->text ("unknown");
	      uiout->text (":\n");
	    }
	  if (start_preceding_line_to_display > 0)
	    {
	      /* Several source lines w/o asm instructions associated.
		 We need to preserve the structure of the output, so output
		 a bunch of line tuples with no asm entries.  */
	      int l;

	      gdb_assert (sal.symtab != NULL);
	      for (l = start_preceding_line_to_display;
		   l < end_preceding_line_to_display;
		   ++l)
		{
		  ui_out_emit_tuple line_tuple_emitter (uiout,
							"src_and_asm_line");
		  print_source_lines (sal.symtab, l, l + 1, psl_flags);
		  ui_out_emit_list chain_line_emitter (uiout, "line_asm_insn");
		}
	    }
	  tuple_emitter.emplace (uiout, "src_and_asm_line");
	  if (sal.symtab != NULL)
	    print_source_lines (sal.symtab, sal.line, sal.line + 1, psl_flags);
	  else
	    uiout->text (_("--- no source info for this pc ---\n"));
	  list_emitter.emplace (uiout, "line_asm_insn");
	}
      else
	{
	  /* Here we're appending instructions to an existing line.
	     By construction the very first insn will have a symtab
	     and follow the new_source_line path above.  */
	  gdb_assert (tuple_emitter.has_value ());
	  gdb_assert (list_emitter.has_value ());
	}

      if (sal.end != 0)
	end_pc = std::min (sal.end, high);
      else
	end_pc = pc + 1;
      num_displayed += dump_insns (gdbarch, uiout, pc, end_pc,
				   how_many, flags, &end_pc);
      pc = end_pc;

      if (how_many >= 0 && num_displayed >= how_many)
	break;

      last_symtab = sal.symtab;
      last_line = sal.line;
    }
}

static void
do_assembly_only (struct gdbarch *gdbarch, struct ui_out *uiout,
		  CORE_ADDR low, CORE_ADDR high,
		  int how_many, gdb_disassembly_flags flags)
{
  ui_out_emit_list list_emitter (uiout, "asm_insns");

  dump_insns (gdbarch, uiout, low, high, how_many, flags, NULL);
}

/* Combine implicit and user disassembler options and return them
   in a newly-created string.  */

static std::string
get_all_disassembler_options (struct gdbarch *gdbarch)
{
  const char *implicit = gdbarch_disassembler_options_implicit (gdbarch);
  const char *options = get_disassembler_options (gdbarch);
  const char *comma = ",";

  if (implicit == nullptr)
    {
      implicit = "";
      comma = "";
    }

  if (options == nullptr)
    {
      options = "";
      comma = "";
    }

  return string_printf ("%s%s%s", implicit, comma, options);
}

gdb_disassembler::gdb_disassembler (struct gdbarch *gdbarch,
				    struct ui_file *file,
				    read_memory_ftype func)
  : gdb_printing_disassembler (gdbarch, &m_buffer, func,
			       dis_asm_memory_error, dis_asm_print_address),
    m_dest (file),
    m_buffer (!use_ext_lang_for_styling () && use_libopcodes_for_styling ())
{ /* Nothing.  */ }

/* See disasm.h.  */

bool
gdb_disassembler::use_ext_lang_for_styling () const
{
  /* The use of m_di.created_styled_output here is a bit of a cheat, but
     it works fine for now.

     This function is called in situations after m_di has been initialized,
     but before the instruction has been disassembled.

     Currently, every target that supports libopcodes styling sets the
     created_styled_output field in disassemble_init_for_target, which was
     called as part of the initialization of gdb_printing_disassembler.

     This means that we are OK to check the created_styled_output field
     here.

     If, in the future, there's ever a target that only sets the
     created_styled_output field during the actual instruction disassembly
     phase, then we will need to update this code.  */
  return (disassembler_styling
	  && (!m_di.created_styled_output || !use_libopcodes_styling)
	  && use_ext_lang_colorization_p
	  && m_dest->can_emit_style_escape ());
}

/* See disasm.h.  */

bool
gdb_disassembler::use_libopcodes_for_styling () const
{
  /* See the comment on the use of m_di.created_styled_output in the
     gdb_disassembler::use_ext_lang_for_styling function.  */
  return (disassembler_styling
	  && m_di.created_styled_output
	  && use_libopcodes_styling
	  && m_dest->can_emit_style_escape ());
}

/* See disasm.h.  */

gdb_disassemble_info::gdb_disassemble_info
  (struct gdbarch *gdbarch,
   read_memory_ftype read_memory_func, memory_error_ftype memory_error_func,
   print_address_ftype print_address_func, fprintf_ftype fprintf_func,
   fprintf_styled_ftype fprintf_styled_func)
    : m_gdbarch (gdbarch)
{
  gdb_assert (fprintf_func != nullptr);
  gdb_assert (fprintf_styled_func != nullptr);
  init_disassemble_info (&m_di, (void *) this, fprintf_func,
			 fprintf_styled_func);
  m_di.flavour = bfd_target_unknown_flavour;

  /* The memory_error_func, print_address_func, and read_memory_func are
     all initialized to a default (non-nullptr) value by the call to
     init_disassemble_info above.  If the user is overriding these fields
     (by passing non-nullptr values) then do that now, otherwise, leave
     these fields as the defaults.  */
  if (memory_error_func != nullptr)
    m_di.memory_error_func = memory_error_func;
  if (print_address_func != nullptr)
    m_di.print_address_func = print_address_func;
  if (read_memory_func != nullptr)
    m_di.read_memory_func = read_memory_func;

  m_di.arch = gdbarch_bfd_arch_info (gdbarch)->arch;
  m_di.mach = gdbarch_bfd_arch_info (gdbarch)->mach;
  m_di.endian = gdbarch_byte_order (gdbarch);
  m_di.endian_code = gdbarch_byte_order_for_code (gdbarch);
  m_di.application_data = this;
  m_disassembler_options_holder = get_all_disassembler_options (gdbarch);
  if (!m_disassembler_options_holder.empty ())
    m_di.disassembler_options = m_disassembler_options_holder.c_str ();
  disassemble_init_for_target (&m_di);
}

/* See disasm.h.  */

gdb_disassemble_info::~gdb_disassemble_info ()
{
  disassemble_free_target (&m_di);
}

/* Wrapper around calling gdbarch_print_insn.  This function takes care of
   first calling the extension language hooks for print_insn, and, if none
   of the extension languages can print this instruction, calls
   gdbarch_print_insn to do the work.

   GDBARCH is the architecture to disassemble in, VMA is the address of the
   instruction being disassembled, and INFO is the libopcodes disassembler
   related information.  */

static int
gdb_print_insn_1 (struct gdbarch *gdbarch, CORE_ADDR vma,
		  struct disassemble_info *info)
{
  /* Call into the extension languages to do the disassembly.  */
  std::optional<int> length = ext_lang_print_insn (gdbarch, vma, info);
  if (length.has_value ())
    return *length;

  /* No extension language wanted to do the disassembly, so do it
     manually.  */
  return gdbarch_print_insn (gdbarch, vma, info);
}

/* See disasm.h.  */

bool gdb_disassembler::use_ext_lang_colorization_p = true;

/* See disasm.h.  */

int
gdb_disassembler::print_insn (CORE_ADDR memaddr,
			      int *branch_delay_insns)
{
  m_err_memaddr.reset ();
  m_buffer.clear ();
  this->set_in_comment (false);

  int length = gdb_print_insn_1 (arch (), memaddr, &m_di);

  /* If we have successfully disassembled an instruction, disassembler
     styling using the extension language is on, and libopcodes hasn't
     already styled the output for us, and, if the destination can support
     styling, then lets call into the extension languages in order to style
     this output.  */
  if (length > 0 && use_ext_lang_for_styling ())
    {
      std::optional<std::string> ext_contents;
      ext_contents = ext_lang_colorize_disasm (m_buffer.string (), arch ());
      if (ext_contents.has_value ())
	m_buffer = std::move (*ext_contents);
      else
	{
	  /* The extension language failed to add styling to the
	     disassembly output.  Set the static flag so that next time we
	     disassemble we don't even bother attempting to use the
	     extension language for styling.  */
	  use_ext_lang_colorization_p = false;

	  /* We're about to disassemble this instruction again, reset the
	     in-comment state.  */
	  this->set_in_comment (false);

	  /* The instruction we just disassembled, and the extension
	     languages failed to style, might have otherwise had some
	     minimal styling applied by GDB.  To regain that styling we
	     need to recreate m_buffer, but this time with styling support.

	     To do this we perform an in-place new, but this time turn on
	     the styling support, then we can re-disassembly the
	     instruction, and gain any minimal styling GDB might add.  */
	  static_assert ((std::is_same<decltype (m_buffer),
			      string_file>::value));
	  gdb_assert (!m_buffer.term_out ());
	  m_buffer.~string_file ();
	  new (&m_buffer) string_file (use_libopcodes_for_styling ());
	  length = gdb_print_insn_1 (arch (), memaddr, &m_di);
	  gdb_assert (length > 0);
	}
    }

  /* Push any disassemble output to the real destination stream.  We do
     this even if the disassembler reported failure (-1) as the
     disassembler may have printed something to its output stream.  */
  gdb_printf (m_dest, "%s", m_buffer.c_str ());

  /* If the disassembler failed then report an appropriate error.  */
  if (length < 0)
    {
      if (m_err_memaddr.has_value ())
	memory_error (TARGET_XFER_E_IO, *m_err_memaddr);
      else
	error (_("unknown disassembler error (error = %d)"), length);
    }

  if (branch_delay_insns != NULL)
    {
      if (m_di.insn_info_valid)
	*branch_delay_insns = m_di.branch_delay_insns;
      else
	*branch_delay_insns = 0;
    }
  return length;
}

void
gdb_disassembly (struct gdbarch *gdbarch, struct ui_out *uiout,
		 gdb_disassembly_flags flags, int how_many,
		 CORE_ADDR low, CORE_ADDR high)
{
  struct symtab *symtab;
  int nlines = -1;

  /* Assume symtab is valid for whole PC range.  */
  symtab = find_pc_line_symtab (low);

  if (symtab != NULL && symtab->linetable () != NULL)
    nlines = symtab->linetable ()->nitems;

  if (!(flags & (DISASSEMBLY_SOURCE_DEPRECATED | DISASSEMBLY_SOURCE))
      || nlines <= 0)
    do_assembly_only (gdbarch, uiout, low, high, how_many, flags);

  else if (flags & DISASSEMBLY_SOURCE)
    do_mixed_source_and_assembly (gdbarch, uiout, symtab, low, high,
				  how_many, flags);

  else if (flags & DISASSEMBLY_SOURCE_DEPRECATED)
    do_mixed_source_and_assembly_deprecated (gdbarch, uiout, symtab,
					     low, high, how_many, flags);

  gdb_flush (gdb_stdout);
}

/* Print the instruction at address MEMADDR in debugged memory,
   on STREAM.  Returns the length of the instruction, in bytes,
   and, if requested, the number of branch delay slot instructions.  */

int
gdb_print_insn (struct gdbarch *gdbarch, CORE_ADDR memaddr,
		struct ui_file *stream, int *branch_delay_insns)
{

  gdb_disassembler di (gdbarch, stream);

  return di.print_insn (memaddr, branch_delay_insns);
}

/* Return the length in bytes of the instruction at address MEMADDR in
   debugged memory.  */

int
gdb_insn_length (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return gdb_print_insn (gdbarch, addr, &null_stream, NULL);
}

/* See disasm.h.  */

int
gdb_non_printing_disassembler::null_fprintf_func
  (void *stream, const char *format, ...) noexcept
{
  return 0;
}

/* See disasm.h.  */

int
gdb_non_printing_disassembler::null_fprintf_styled_func
  (void *stream, enum disassembler_style style,
   const char *format, ...) noexcept
{
  return 0;
}

/* A non-printing disassemble_info management class.  The disassemble_info
   setup by this class will not print anything to the output stream (there
   is no output stream), and the instruction to be disassembled will be
   read from a buffer passed to the constructor.  */

struct gdb_non_printing_buffer_disassembler
  : public gdb_non_printing_disassembler
{
  /* Constructor.  GDBARCH is the architecture to disassemble for, BUFFER
     contains the instruction to disassemble, and INSN_ADDRESS is the
     address (in target memory) of the instruction to disassemble.  */
  gdb_non_printing_buffer_disassembler (struct gdbarch *gdbarch,
					gdb::array_view<const gdb_byte> buffer,
					CORE_ADDR insn_address)
    : gdb_non_printing_disassembler (gdbarch, nullptr)
  {
    /* The cast is necessary until disassemble_info is const-ified.  */
    m_di.buffer = (gdb_byte *) buffer.data ();
    m_di.buffer_length = buffer.size ();
    m_di.buffer_vma = insn_address;
  }
};

/* Return the length in bytes of INSN.  MAX_LEN is the size of the
   buffer containing INSN.  */

int
gdb_buffered_insn_length (struct gdbarch *gdbarch,
			  const gdb_byte *insn, int max_len, CORE_ADDR addr)
{
  gdb::array_view<const gdb_byte> buffer
    = gdb::make_array_view (insn, max_len);
  gdb_non_printing_buffer_disassembler dis (gdbarch, buffer, addr);
  int result = gdb_print_insn_1 (gdbarch, addr, dis.disasm_info ());
  return result;
}

char *
get_disassembler_options (struct gdbarch *gdbarch)
{
  char **disassembler_options = gdbarch_disassembler_options (gdbarch);
  if (disassembler_options == NULL)
    return NULL;
  return *disassembler_options;
}

void
set_disassembler_options (const char *prospective_options)
{
  struct gdbarch *gdbarch = get_current_arch ();
  char **disassembler_options = gdbarch_disassembler_options (gdbarch);
  const disasm_options_and_args_t *valid_options_and_args;
  const disasm_options_t *valid_options;
  gdb::unique_xmalloc_ptr<char> prospective_options_local
    = make_unique_xstrdup (prospective_options);
  char *options = remove_whitespace_and_extra_commas
    (prospective_options_local.get ());
  const char *opt;

  /* Allow all architectures, even ones that do not support 'set disassembler',
     to reset their disassembler options to NULL.  */
  if (options == NULL)
    {
      if (disassembler_options != NULL)
	{
	  free (*disassembler_options);
	  *disassembler_options = NULL;
	}
      return;
    }

  valid_options_and_args = gdbarch_valid_disassembler_options (gdbarch);
  if (valid_options_and_args == NULL)
    {
      gdb_printf (gdb_stderr, _("\
'set disassembler-options ...' is not supported on this architecture.\n"));
      return;
    }

  valid_options = &valid_options_and_args->options;

  /* Verify we have valid disassembler options.  */
  FOR_EACH_DISASSEMBLER_OPTION (opt, options)
    {
      size_t i;
      for (i = 0; valid_options->name[i] != NULL; i++)
	if (valid_options->arg != NULL && valid_options->arg[i] != NULL)
	  {
	    size_t len = strlen (valid_options->name[i]);
	    bool found = false;
	    const char *arg;
	    size_t j;

	    if (memcmp (opt, valid_options->name[i], len) != 0)
	      continue;
	    arg = opt + len;
	    if (valid_options->arg[i]->values == NULL)
	      break;
	    for (j = 0; valid_options->arg[i]->values[j] != NULL; j++)
	      if (disassembler_options_cmp
		    (arg, valid_options->arg[i]->values[j]) == 0)
		{
		  found = true;
		  break;
		}
	    if (found)
	      break;
	  }
	else if (disassembler_options_cmp (opt, valid_options->name[i]) == 0)
	  break;
      if (valid_options->name[i] == NULL)
	{
	  gdb_printf (gdb_stderr,
		      _("Invalid disassembler option value: '%s'.\n"),
		      opt);
	  return;
	}
    }

  free (*disassembler_options);
  *disassembler_options = xstrdup (options);
}

static void
set_disassembler_options_sfunc (const char *args, int from_tty,
				struct cmd_list_element *c)
{
  set_disassembler_options (prospective_options.c_str ());
}

static void
show_disassembler_options_sfunc (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c, const char *value)
{
  struct gdbarch *gdbarch = get_current_arch ();
  const disasm_options_and_args_t *valid_options_and_args;
  const disasm_option_arg_t *valid_args;
  const disasm_options_t *valid_options;

  const char *options = get_disassembler_options (gdbarch);
  if (options == NULL)
    options = "";

  gdb_printf (file, _("The current disassembler options are '%s'\n\n"),
	      options);

  valid_options_and_args = gdbarch_valid_disassembler_options (gdbarch);

  if (valid_options_and_args == NULL)
    {
      gdb_puts (_("There are no disassembler options available "
		  "for this architecture.\n"),
		file);
      return;
    }

  valid_options = &valid_options_and_args->options;

  gdb_printf (file, _("\
The following disassembler options are supported for use with the\n\
'set disassembler-options OPTION [,OPTION]...' command:\n"));

  if (valid_options->description != NULL)
    {
      size_t i, max_len = 0;

      gdb_printf (file, "\n");

      /* Compute the length of the longest option name.  */
      for (i = 0; valid_options->name[i] != NULL; i++)
	{
	  size_t len = strlen (valid_options->name[i]);

	  if (valid_options->arg != NULL && valid_options->arg[i] != NULL)
	    len += strlen (valid_options->arg[i]->name);
	  if (max_len < len)
	    max_len = len;
	}

      for (i = 0, max_len++; valid_options->name[i] != NULL; i++)
	{
	  gdb_printf (file, "  %s", valid_options->name[i]);
	  if (valid_options->arg != NULL && valid_options->arg[i] != NULL)
	    gdb_printf (file, "%s", valid_options->arg[i]->name);
	  if (valid_options->description[i] != NULL)
	    {
	      size_t len = strlen (valid_options->name[i]);

	      if (valid_options->arg != NULL && valid_options->arg[i] != NULL)
		len += strlen (valid_options->arg[i]->name);
	      gdb_printf (file, "%*c %s", (int) (max_len - len), ' ',
			  valid_options->description[i]);
	    }
	  gdb_printf (file, "\n");
	}
    }
  else
    {
      size_t i;
      gdb_printf (file, "  ");
      for (i = 0; valid_options->name[i] != NULL; i++)
	{
	  gdb_printf (file, "%s", valid_options->name[i]);
	  if (valid_options->arg != NULL && valid_options->arg[i] != NULL)
	    gdb_printf (file, "%s", valid_options->arg[i]->name);
	  if (valid_options->name[i + 1] != NULL)
	    gdb_printf (file, ", ");
	  file->wrap_here (2);
	}
      gdb_printf (file, "\n");
    }

  valid_args = valid_options_and_args->args;
  if (valid_args != NULL)
    {
      size_t i, j;

      for (i = 0; valid_args[i].name != NULL; i++)
	{
	  if (valid_args[i].values == NULL)
	    continue;
	  gdb_printf (file, _("\n\
  For the options above, the following values are supported for \"%s\":\n   "),
		      valid_args[i].name);
	  for (j = 0; valid_args[i].values[j] != NULL; j++)
	    {
	      gdb_printf (file, " %s", valid_args[i].values[j]);
	      file->wrap_here (3);
	    }
	  gdb_printf (file, "\n");
	}
    }
}

/* A completion function for "set disassembler".  */

static void
disassembler_options_completer (struct cmd_list_element *ignore,
				completion_tracker &tracker,
				const char *text, const char *word)
{
  struct gdbarch *gdbarch = get_current_arch ();
  const disasm_options_and_args_t *opts_and_args
    = gdbarch_valid_disassembler_options (gdbarch);

  if (opts_and_args != NULL)
    {
      const disasm_options_t *opts = &opts_and_args->options;

      /* Only attempt to complete on the last option text.  */
      const char *separator = strrchr (text, ',');
      if (separator != NULL)
	text = separator + 1;
      text = skip_spaces (text);
      complete_on_enum (tracker, opts->name, text, word);
    }
}


/* Initialization code.  */

void _initialize_disasm ();
void
_initialize_disasm ()
{
  /* Add the command that controls the disassembler options.  */
  set_show_commands set_show_disas_opts
    = add_setshow_string_noescape_cmd ("disassembler-options", no_class,
				       &prospective_options, _("\
Set the disassembler options.\n\
Usage: set disassembler-options OPTION [,OPTION]...\n\n\
See: 'show disassembler-options' for valid option values."), _("\
Show the disassembler options."), NULL,
					 set_disassembler_options_sfunc,
					 show_disassembler_options_sfunc,
					 &setlist, &showlist);
  set_cmd_completer (set_show_disas_opts.set, disassembler_options_completer);


  /* All the 'maint set|show libopcodes-styling' sub-commands.  */
  static struct cmd_list_element *maint_set_libopcodes_styling_cmdlist;
  static struct cmd_list_element *maint_show_libopcodes_styling_cmdlist;

  /* Adds 'maint set|show libopcodes-styling'.  */
  add_setshow_prefix_cmd ("libopcodes-styling", class_maintenance,
			  _("Set libopcodes-styling specific variables."),
			  _("Show libopcodes-styling specific variables."),
			  &maint_set_libopcodes_styling_cmdlist,
			  &maint_show_libopcodes_styling_cmdlist,
			  &maintenance_set_cmdlist,
			  &maintenance_show_cmdlist);

  /* Adds 'maint set|show gnu-source-highlight enabled'.  */
  add_setshow_boolean_cmd ("enabled", class_maintenance,
			   &use_libopcodes_styling_option, _("\
Set whether the libopcodes styling support should be used."), _("\
Show whether the libopcodes styling support should be used."),_("\
When enabled, GDB will try to make use of the builtin libopcodes styling\n\
support, to style the disassembler output.  Not every architecture has\n\
styling support within libopcodes, so enabling this is not a guarantee\n\
that libopcodes styling will be available.\n\
\n\
When this option is disabled, GDB will make use of the Python Pygments\n\
package (if available) to style the disassembler output.\n\
\n\
All disassembler styling can be disabled with:\n\
\n\
  set style disassembler enabled off"),
			   set_use_libopcodes_styling,
			   show_use_libopcodes_styling,
			   &maint_set_libopcodes_styling_cmdlist,
			   &maint_show_libopcodes_styling_cmdlist);
}
