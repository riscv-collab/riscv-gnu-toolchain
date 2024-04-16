/* Disassembly display.

   Copyright (C) 1998-2024 Free Software Foundation, Inc.

   Contributed by Hewlett-Packard Company.

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
#include "symtab.h"
#include "breakpoint.h"
#include "frame.h"
#include "value.h"
#include "source.h"
#include "disasm.h"
#include "tui/tui.h"
#include "tui/tui-command.h"
#include "tui/tui-data.h"
#include "tui/tui-win.h"
#include "tui/tui-layout.h"
#include "tui/tui-winsource.h"
#include "tui/tui-status.h"
#include "tui/tui-file.h"
#include "tui/tui-disasm.h"
#include "tui/tui-source.h"
#include "progspace.h"
#include "objfiles.h"
#include "cli/cli-style.h"
#include "tui/tui-location.h"
#include "gdbsupport/selftest.h"
#include "inferior.h"

#include "gdb_curses.h"

struct tui_asm_line
{
  CORE_ADDR addr;
  std::string addr_string;
  size_t addr_size;
  std::string insn;
};

/* Helper function to find the number of characters in STR, skipping
   any ANSI escape sequences.  */
static size_t
len_without_escapes (const std::string &str)
{
  size_t len = 0;
  const char *ptr = str.c_str ();
  char c;

  while ((c = *ptr) != '\0')
    {
      if (c == '\033')
	{
	  ui_file_style style;
	  size_t n_read;
	  if (style.parse (ptr, &n_read))
	    ptr += n_read;
	  else
	    {
	      /* Shouldn't happen, but just skip the ESC if it somehow
		 does.  */
	      ++ptr;
	    }
	}
      else
	{
	  ++len;
	  ++ptr;
	}
    }
  return len;
}

/* Function to disassemble up to COUNT instructions starting from address
   PC into the ASM_LINES vector (which will be emptied of any previous
   contents).  Return the address of the COUNT'th instruction after pc.
   When ADDR_SIZE is non-null then place the maximum size of an address and
   label into the value pointed to by ADDR_SIZE, and set the addr_size
   field on each item in ASM_LINES, otherwise the addr_size fields within
   ASM_LINES are undefined.

   It is worth noting that ASM_LINES might not have COUNT entries when this
   function returns.  If the disassembly is truncated for some other
   reason, for example, we hit invalid memory, then ASM_LINES can have
   fewer entries than requested.  */
static CORE_ADDR
tui_disassemble (struct gdbarch *gdbarch,
		 std::vector<tui_asm_line> &asm_lines,
		 CORE_ADDR pc, int count,
		 size_t *addr_size = nullptr)
{
  bool term_out = source_styling && gdb_stdout->can_emit_style_escape ();
  string_file gdb_dis_out (term_out);

  /* Must start with an empty list.  */
  asm_lines.clear ();

  /* Now construct each line.  */
  for (int i = 0; i < count; ++i)
    {
      tui_asm_line tal;
      CORE_ADDR orig_pc = pc;

      try
	{
	  pc = pc + gdb_print_insn (gdbarch, pc, &gdb_dis_out, NULL);
	}
      catch (const gdb_exception_error &except)
	{
	  /* If PC points to an invalid address then we'll catch a
	     MEMORY_ERROR here, this should stop the disassembly, but
	     otherwise is fine.  */
	  if (except.error != MEMORY_ERROR)
	    throw;
	  return pc;
	}

      /* Capture the disassembled instruction.  */
      tal.insn = gdb_dis_out.release ();

      /* And capture the address the instruction is at.  */
      tal.addr = orig_pc;
      print_address (gdbarch, orig_pc, &gdb_dis_out);
      tal.addr_string = gdb_dis_out.release ();

      if (addr_size != nullptr)
	{
	  size_t new_size;

	  if (term_out)
	    new_size = len_without_escapes (tal.addr_string);
	  else
	    new_size = tal.addr_string.size ();
	  *addr_size = std::max (*addr_size, new_size);
	  tal.addr_size = new_size;
	}

      asm_lines.push_back (std::move (tal));
    }
  return pc;
}

/* Look backward from ADDR for an address from which we can start
   disassembling, this needs to be something we can be reasonably
   confident will fall on an instruction boundary.  We use msymbol
   addresses, or the start of a section.  */

static CORE_ADDR
tui_find_backward_disassembly_start_address (CORE_ADDR addr)
{
  struct bound_minimal_symbol msym, msym_prev;

  msym = lookup_minimal_symbol_by_pc_section (addr - 1, nullptr,
					      lookup_msym_prefer::TEXT,
					      &msym_prev);
  if (msym.minsym != nullptr)
    return msym.value_address ();
  else if (msym_prev.minsym != nullptr)
    return msym_prev.value_address ();

  /* Find the section that ADDR is in, and look for the start of the
     section.  */
  struct obj_section *section = find_pc_section (addr);
  if (section != NULL)
    return section->addr ();

  return addr;
}

/* Find the disassembly address that corresponds to FROM lines above
   or below the PC.  Variable sized instructions are taken into
   account by the algorithm.  */
static CORE_ADDR
tui_find_disassembly_address (struct gdbarch *gdbarch, CORE_ADDR pc, int from)
{
  CORE_ADDR new_low;
  int max_lines;

  max_lines = (from > 0) ? from : - from;
  if (max_lines == 0)
    return pc;

  std::vector<tui_asm_line> asm_lines;

  new_low = pc;
  if (from > 0)
    {
      /* Always disassemble 1 extra instruction here, then if the last
	 instruction fails to disassemble we will take the address of the
	 previous instruction that did disassemble as the result.  */
      tui_disassemble (gdbarch, asm_lines, pc, max_lines + 1);
      if (asm_lines.empty ())
	return pc;
      new_low = asm_lines.back ().addr;
    }
  else
    {
      /* In order to disassemble backwards we need to find a suitable
	 address to start disassembling from and then work forward until we
	 re-find the address we're currently at.  We can then figure out
	 which address will be at the top of the TUI window after our
	 backward scroll.  During our backward disassemble we need to be
	 able to distinguish between the case where the last address we
	 _can_ disassemble is ADDR, and the case where the disassembly
	 just happens to stop at ADDR, for this reason we increase
	 MAX_LINES by one.  */
      max_lines++;

      /* When we disassemble a series of instructions this will hold the
	 address of the last instruction disassembled.  */
      CORE_ADDR last_addr;

      /* And this will hold the address of the next instruction that would
	 have been disassembled.  */
      CORE_ADDR next_addr;

      /* As we search backward if we find an address that looks like a
	 promising starting point then we record it in this structure.  If
	 the next address we try is not a suitable starting point then we
	 will fall back to the address held here.  */
      std::optional<CORE_ADDR> possible_new_low;

      /* The previous value of NEW_LOW so we know if the new value is
	 different or not.  */
      CORE_ADDR prev_low;

      do
	{
	  /* Find an address from which we can start disassembling.  */
	  prev_low = new_low;
	  new_low = tui_find_backward_disassembly_start_address (new_low);

	  /* Disassemble forward.  */
	  next_addr = tui_disassemble (gdbarch, asm_lines, new_low, max_lines);
	  if (asm_lines.empty ())
	    break;
	  last_addr = asm_lines.back ().addr;

	  /* If disassembling from the current value of NEW_LOW reached PC
	     (or went past it) then this would do as a starting point if we
	     can't find anything better, so remember it.  */
	  if (last_addr >= pc && new_low != prev_low
	      && asm_lines.size () >= max_lines)
	    possible_new_low.emplace (new_low);

	  /* Continue searching until we find a value of NEW_LOW from which
	     disassembling MAX_LINES instructions doesn't reach PC.  We
	     know this means we can find the required number of previous
	     instructions then.  */
	}
      while ((last_addr > pc
	      || (last_addr == pc && asm_lines.size () < max_lines))
	     && new_low != prev_low);

      /* If we failed to disassemble the required number of lines, try to fall
	 back to a previous possible start address in POSSIBLE_NEW_LOW.  */
      if (asm_lines.size () < max_lines)
	{
	  if (!possible_new_low.has_value ())
	    return new_low;

	  /* Take the best possible match we have.  */
	  new_low = *possible_new_low;
	  next_addr = tui_disassemble (gdbarch, asm_lines, new_low, max_lines);
	}

      /* The following walk forward assumes that ASM_LINES contains exactly
	 MAX_LINES entries.  */
      gdb_assert (asm_lines.size () == max_lines);

      /* Scan forward disassembling one instruction at a time until
	 the last visible instruction of the window matches the pc.
	 We keep the disassembled instructions in the 'lines' window
	 and shift it downward (increasing its addresses).  */
      int pos = max_lines - 1;
      last_addr = asm_lines.back ().addr;
      if (last_addr < pc)
	do
	  {
	    pos++;
	    if (pos >= max_lines)
	      pos = 0;

	    CORE_ADDR old_next_addr = next_addr;
	    std::vector<tui_asm_line> single_asm_line;
	    next_addr = tui_disassemble (gdbarch, single_asm_line,
					 next_addr, 1);
	    /* If there are some problems while disassembling exit.  */
	    if (next_addr <= old_next_addr)
	      return pc;
	    gdb_assert (single_asm_line.size () == 1);
	    asm_lines[pos] = single_asm_line[0];
	  } while (next_addr <= pc);
      pos++;
      if (pos >= max_lines)
	 pos = 0;
      new_low = asm_lines[pos].addr;

      /* When scrolling backward the addresses should move backward, or at
	 the very least stay the same if we are at the first address that
	 can be disassembled.  */
      gdb_assert (new_low <= pc);
    }
  return new_low;
}

/* Function to set the disassembly window's content.  */
bool
tui_disasm_window::set_contents (struct gdbarch *arch,
				 const struct symtab_and_line &sal)
{
  int i;
  int max_lines;
  CORE_ADDR cur_pc;
  int tab_len = tui_tab_width;
  int insn_pos;

  CORE_ADDR pc = sal.pc;
  if (pc == 0)
    return false;

  m_gdbarch = arch;
  m_start_line_or_addr.loa = LOA_ADDRESS;
  m_start_line_or_addr.u.addr = pc;
  cur_pc = tui_location.addr ();

  /* Window size, excluding highlight box.  */
  max_lines = height - box_size ();

  /* Get temporary table that will hold all strings (addr & insn).  */
  std::vector<tui_asm_line> asm_lines;
  size_t addr_size = 0;
  tui_disassemble (m_gdbarch, asm_lines, pc, max_lines, &addr_size);

  /* Align instructions to the same column.  */
  insn_pos = (1 + (addr_size / tab_len)) * tab_len;

  /* Now construct each line.  */
  m_content.resize (max_lines);
  m_max_length = -1;
  for (i = 0; i < max_lines; i++)
    {
      tui_source_element *src = &m_content[i];

      std::string line;
      CORE_ADDR addr;

      if (i < asm_lines.size ())
	{
	  line
	    = (asm_lines[i].addr_string
	       + n_spaces (insn_pos - asm_lines[i].addr_size)
	       + asm_lines[i].insn);
	  addr = asm_lines[i].addr;
	}
      else
	{
	  line = "";
	  addr = 0;
	}

      const char *ptr = line.c_str ();
      int line_len;
      src->line = tui_copy_source_line (&ptr, &line_len);
      m_max_length = std::max (m_max_length, line_len);

      src->line_or_addr.loa = LOA_ADDRESS;
      src->line_or_addr.u.addr = addr;
      src->is_exec_point = (addr == cur_pc && line.size () > 0);
    }
  return true;
}


void
tui_get_begin_asm_address (struct gdbarch **gdbarch_p, CORE_ADDR *addr_p)
{
  struct gdbarch *gdbarch = get_current_arch ();
  CORE_ADDR addr = 0;

  if (tui_location.addr () == 0)
    {
      if (have_full_symbols () || have_partial_symbols ())
	{
	  set_default_source_symtab_and_line ();
	  struct symtab_and_line sal = get_current_source_symtab_and_line ();

	  if (sal.symtab != nullptr)
	    find_line_pc (sal.symtab, sal.line, &addr);
	}

      if (addr == 0)
	{
	  struct bound_minimal_symbol main_symbol
	    = lookup_minimal_symbol (main_name (), nullptr, nullptr);
	  if (main_symbol.minsym != nullptr)
	    addr = main_symbol.value_address ();
	}
    }
  else				/* The target is executing.  */
    {
      gdbarch = tui_location.gdbarch ();
      addr = tui_location.addr ();
    }

  *gdbarch_p = gdbarch;
  *addr_p = addr;
}

/* Determine what the low address will be to display in the TUI's
   disassembly window.  This may or may not be the same as the low
   address input.  */
CORE_ADDR
tui_get_low_disassembly_address (struct gdbarch *gdbarch,
				 CORE_ADDR low, CORE_ADDR pc)
{
  int pos;

  /* Determine where to start the disassembly so that the pc is about
     in the middle of the viewport.  */
  if (TUI_DISASM_WIN != NULL)
    pos = TUI_DISASM_WIN->height;
  else if (TUI_CMD_WIN == NULL)
    pos = tui_term_height () / 2 - 2;
  else
    pos = tui_term_height () - TUI_CMD_WIN->height - 2;
  pos = (pos - 2) / 2;

  pc = tui_find_disassembly_address (gdbarch, pc, -pos);

  if (pc < low)
    pc = low;
  return pc;
}

/* Scroll the disassembly forward or backward vertically.  */
void
tui_disasm_window::do_scroll_vertical (int num_to_scroll)
{
  if (!m_content.empty ())
    {
      CORE_ADDR pc;

      pc = m_start_line_or_addr.u.addr;

      symtab_and_line sal {};
      sal.pspace = current_program_space;
      sal.pc = tui_find_disassembly_address (m_gdbarch, pc, num_to_scroll);
      update_source_window_as_is (m_gdbarch, sal);
    }
}

bool
tui_disasm_window::location_matches_p (struct bp_location *loc, int line_no)
{
  return (m_content[line_no].line_or_addr.loa == LOA_ADDRESS
	  && m_content[line_no].line_or_addr.u.addr == loc->address);
}

bool
tui_disasm_window::addr_is_displayed (CORE_ADDR addr) const
{
  if (m_content.size () < SCROLL_THRESHOLD)
    return false;

  for (size_t i = 0; i < m_content.size () - SCROLL_THRESHOLD; ++i)
    {
      if (m_content[i].line_or_addr.loa == LOA_ADDRESS
	  && m_content[i].line_or_addr.u.addr == addr)
	return true;
    }

  return false;
}

void
tui_disasm_window::maybe_update (frame_info_ptr fi, symtab_and_line sal)
{
  CORE_ADDR low;

  struct gdbarch *frame_arch = get_frame_arch (fi);

  if (find_pc_partial_function (sal.pc, NULL, &low, NULL) == 0)
    {
      /* There is no symbol available for current PC.  There is no
	 safe way how to "disassemble backwards".  */
      low = sal.pc;
    }
  else
    low = tui_get_low_disassembly_address (frame_arch, low, sal.pc);

  struct tui_line_or_address a;

  a.loa = LOA_ADDRESS;
  a.u.addr = low;
  if (!addr_is_displayed (sal.pc))
    {
      sal.pc = low;
      update_source_window (frame_arch, sal);
    }
  else
    {
      a.u.addr = sal.pc;
      set_is_exec_point_at (a);
    }
}

void
tui_disasm_window::display_start_addr (struct gdbarch **gdbarch_p,
				       CORE_ADDR *addr_p)
{
  *gdbarch_p = m_gdbarch;
  *addr_p = m_start_line_or_addr.u.addr;
}

#if GDB_SELF_TEST
namespace selftests {
namespace tui {
namespace disasm {

static void
run_tests ()
{
  if (current_inferior () != nullptr)
    {
      gdbarch *gdbarch = current_inferior ()->arch ();

      /* Check that tui_find_disassembly_address robustly handles the case of
	 being passed a PC for which gdb_print_insn throws a MEMORY_ERROR.  */
      SELF_CHECK (tui_find_disassembly_address (gdbarch, 0, 1) == 0);
      SELF_CHECK (tui_find_disassembly_address (gdbarch, 0, -1) == 0);
    }
}

} /* namespace disasm */
} /* namespace tui */
} /* namespace selftests */
#endif /* GDB_SELF_TEST */

void _initialize_tui_disasm ();
void
_initialize_tui_disasm ()
{
#if GDB_SELF_TEST
  selftests::register_test ("tui-disasm", selftests::tui::disasm::run_tests);
#endif
}
