/* MI Command Set - disassemble commands.
   Copyright (C) 2000-2024 Free Software Foundation, Inc.
   Contributed by Cygnus Solutions (a Red Hat company).

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
#include "mi-cmds.h"
#include "mi-getopt.h"
#include "ui-out.h"
#include "disasm.h"

/* The arguments to be passed on the command line and parsed here are
   either:

   START-ADDRESS: address to start the disassembly at.
   END-ADDRESS: address to end the disassembly at.

   or:

   FILENAME: The name of the file where we want disassemble from.
   LINE: The line around which we want to disassemble. It will
   disassemble the function that contains that line.
   HOW_MANY: Number of disassembly lines to display. With source, it
   is the number of disassembly lines only, not counting the source
   lines.  

   always required:

   MODE: 0 -- disassembly.
	 1 -- disassembly and source (with deprecated source-centric view).
	 2 -- disassembly and opcodes.
	 3 -- disassembly, source-centric and opcodes.
	 4 -- disassembly, and source (with pc-centric view).
	 5 -- disassembly, source (pc-centric) and opcodes.  */

void
mi_cmd_disassemble (const char *command, const char *const *argv, int argc)
{
  struct gdbarch *gdbarch = get_current_arch ();
  struct ui_out *uiout = current_uiout;
  CORE_ADDR start;

  int mode;
  gdb_disassembly_flags disasm_flags;
  struct symtab *s;

  /* Which options have we processed ... */
  bool file_seen = false;
  bool line_seen = false;
  bool num_seen = false;
  bool start_seen = false;
  bool end_seen = false;
  bool addr_seen = false;
  bool opcodes_seen = false;
  bool source_seen = false;

  /* ... and their corresponding value. */
  const char *file_string = NULL;
  int line_num = -1;
  int how_many = -1;
  CORE_ADDR low = 0;
  CORE_ADDR high = 0;
  CORE_ADDR addr = 0;

  /* Flags to handle the --opcodes option.  */
  enum opcodes_mode
  {
    OPCODES_DEFAULT, OPCODES_NONE, OPCODES_DISPLAY, OPCODES_BYTES
  };
  enum opcodes_mode opcodes_mode = OPCODES_DEFAULT;

  /* Handle the -source option.  */
  bool show_source = false;

  /* Options processing stuff.  */
  int oind = 0;
  const char *oarg;
  enum opt
  {
    FILE_OPT, LINE_OPT, NUM_OPT, START_OPT, END_OPT, ADDR_OPT, OPCODES_OPT,
    SHOW_SRC_OPT
  };
  static const struct mi_opt opts[] =
    {
      {"f", FILE_OPT, 1},
      {"l", LINE_OPT, 1},
      {"n", NUM_OPT, 1},
      {"s", START_OPT, 1},
      {"e", END_OPT, 1},
      {"a", ADDR_OPT, 1},
      {"-opcodes", OPCODES_OPT, 1},
      {"-source", SHOW_SRC_OPT, 0},
      { 0, 0, 0 }
    };

  /* Get the options with their arguments. Keep track of what we
     encountered.  */
  while (1)
    {
      int opt = mi_getopt ("-data-disassemble", argc, argv, opts,
			   &oind, &oarg);
      if (opt < 0)
	break;
      switch ((enum opt) opt)
	{
	case FILE_OPT:
	  file_string = oarg;
	  file_seen = true;
	  break;
	case LINE_OPT:
	  line_num = atoi (oarg);
	  line_seen = true;
	  break;
	case NUM_OPT:
	  how_many = atoi (oarg);
	  num_seen = true;
	  break;
	case START_OPT:
	  low = parse_and_eval_address (oarg);
	  start_seen = true;
	  break;
	case END_OPT:
	  high = parse_and_eval_address (oarg);
	  end_seen = true;
	  break;
	case ADDR_OPT:
	  addr = parse_and_eval_address (oarg);
	  addr_seen = true;
	  break;
	case OPCODES_OPT:
	  opcodes_seen = true;
	  if (strcmp (oarg, "none") == 0)
	    opcodes_mode = OPCODES_NONE;
	  else if (strcmp (oarg, "display") == 0)
	    opcodes_mode = OPCODES_DISPLAY;
	  else if (strcmp (oarg, "bytes") == 0)
	    opcodes_mode = OPCODES_BYTES;
	  else
	    error (_("-data-disassemble: unknown value for -opcodes argument"));
	  break;
	case SHOW_SRC_OPT:
	  source_seen = true;
	  show_source = true;
	  break;
	}
    }
  argv += oind;
  argc -= oind;

  /* Allow only filename + linenum (with how_many which is not
     required) OR start_addr + end_addr OR addr.  */

  if (!(
	  ( line_seen &&  file_seen &&              !start_seen && !end_seen
								&& !addr_seen)

       || (!line_seen && !file_seen && !num_seen &&  start_seen &&  end_seen
								&& !addr_seen)

       || (!line_seen && !file_seen && !num_seen && !start_seen && !end_seen
								&&  addr_seen))
      || argc > 1)
    error (_("-data-disassemble: Usage: "
	     "( -f filename -l linenum [-n howmany] |"
	     " -s startaddr -e endaddr | -a addr ) "
	     "[ --opcodes mode ] [ --source ] [ [--] mode ]."));

  if (argc == 1)
    {
      mode = atoi (argv[0]);
      if (mode < 0 || mode > 5)
	error (_("-data-disassemble: Mode argument must be in the range 0-5."));
    }
  else
    mode = 0;

  if (mode != 0 && (source_seen || opcodes_seen))
    error (_("-data-disassemble: --opcodes and --source can only be used with mode 0"));

  /* Convert the mode into a set of disassembly flags.  */

  disasm_flags = 0;  /* Initialize here for -Wall.  */
  switch (mode)
    {
    case 0:
      break;
    case 1:
      disasm_flags |= DISASSEMBLY_SOURCE_DEPRECATED;
      break;
    case 2:
      disasm_flags |= DISASSEMBLY_RAW_BYTES;
      break;
    case 3:
      disasm_flags |= DISASSEMBLY_SOURCE_DEPRECATED | DISASSEMBLY_RAW_BYTES;
      break;
    case 4:
      disasm_flags |= DISASSEMBLY_SOURCE;
      break;
    case 5:
      disasm_flags |= DISASSEMBLY_SOURCE | DISASSEMBLY_RAW_BYTES;
      break;
    default:
      gdb_assert_not_reached ("bad disassembly mode");
    }

  /* Now handle the (optional) --opcodes argument.  This partially
     overrides the mode value.  */
  if (opcodes_mode != OPCODES_DEFAULT)
    {
      /* Remove any existing flags related to opcodes display.  */
      disasm_flags &= ~(DISASSEMBLY_RAW_BYTES | DISASSEMBLY_RAW_INSN);

      /* Add back any required flags.  */
      if (opcodes_mode == OPCODES_DISPLAY)
	disasm_flags |= DISASSEMBLY_RAW_INSN;
      else if (opcodes_mode == OPCODES_BYTES)
	disasm_flags |= DISASSEMBLY_RAW_BYTES;
    }

  /* Handle the optional --source argument.  */
  if (show_source)
    {
      disasm_flags &= ~DISASSEMBLY_SOURCE_DEPRECATED;
      disasm_flags |= DISASSEMBLY_SOURCE;
    }

  /* We must get the function beginning and end where line_num is
     contained.  */

  if (line_seen && file_seen)
    {
      s = lookup_symtab (file_string);
      if (s == NULL)
	error (_("-data-disassemble: Invalid filename."));
      if (!find_line_pc (s, line_num, &start))
	error (_("-data-disassemble: Invalid line number"));
      if (find_pc_partial_function (start, NULL, &low, &high) == 0)
	error (_("-data-disassemble: "
		 "No function contains specified address"));
    }
  else if (addr_seen)
    {
      if (find_pc_partial_function (addr, NULL, &low, &high) == 0)
	error (_("-data-disassemble: "
		 "No function contains specified address"));
    }

  gdb_disassembly (gdbarch, uiout,
		   disasm_flags,
		   how_many, low, high);
}
