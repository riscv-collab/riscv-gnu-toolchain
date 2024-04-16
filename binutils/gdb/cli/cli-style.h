/* CLI stylizing

   Copyright (C) 2018-2024 Free Software Foundation, Inc.

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

#ifndef CLI_CLI_STYLE_H
#define CLI_CLI_STYLE_H

#include "ui-file.h"
#include "command.h"
#include "gdbsupport/observable.h"

/* A single CLI style option.  */
class cli_style_option
{
public:

  /* Construct a CLI style option with a foreground color.  */
  cli_style_option (const char *name, ui_file_style::basic_color fg,
		    ui_file_style::intensity = ui_file_style::NORMAL);

  /* Construct a CLI style option with an intensity.  */
  cli_style_option (const char *name, ui_file_style::intensity i);

  /* Return a ui_file_style corresponding to the settings in this CLI
     style.  */
  ui_file_style style () const;

  /* Return the style name.  */
  const char *name () { return m_name; };

  /* Call once to register this CLI style with the CLI engine.  Returns
     the set/show prefix commands for the style.  */
  set_show_commands add_setshow_commands (enum command_class theclass,
					  const char *prefix_doc,
					  struct cmd_list_element **set_list,
					  struct cmd_list_element **show_list,
					  bool skip_intensity);

  /* Return the 'set style NAME' command list, that can be used
     to build a lambda DO_SET to call add_setshow_commands.  */
  struct cmd_list_element *set_list () { return m_set_list; };

  /* Same as SET_LIST but for the show command list.  */
  struct cmd_list_element *show_list () { return m_show_list; };

  /* This style can be observed for any changes.  */
  gdb::observers::observable<> changed;

private:

  /* The style name.  */
  const char *m_name;

  /* The foreground.  */
  const char *m_foreground;
  /* The background.  */
  const char *m_background;
  /* The intensity.  */
  const char *m_intensity;

  /* Storage for command lists needed when registering
     subcommands.  */
  struct cmd_list_element *m_set_list = nullptr;
  struct cmd_list_element *m_show_list = nullptr;

  /* Callback to notify the observable.  */
  static void do_set_value (const char *ignore, int from_tty,
			    struct cmd_list_element *cmd);

  /* Callback to show the foreground.  */
  static void do_show_foreground (struct ui_file *file, int from_tty,
				  struct cmd_list_element *cmd,
				  const char *value);
  /* Callback to show the background.  */
  static void do_show_background (struct ui_file *file, int from_tty,
				  struct cmd_list_element *cmd,
				  const char *value);
  /* Callback to show the intensity.  */
  static void do_show_intensity (struct ui_file *file, int from_tty,
				 struct cmd_list_element *cmd,
				 const char *value);
};

/* The file name style.  */
extern cli_style_option file_name_style;

/* The function name style.  */
extern cli_style_option function_name_style;

/* The variable name style.  */
extern cli_style_option variable_name_style;

/* The address style.  */
extern cli_style_option address_style;

/* The highlight style.  */
extern cli_style_option highlight_style;

/* The title style.  */
extern cli_style_option title_style;

/* The metadata style.  */
extern cli_style_option metadata_style;

/* The disassembler style for mnemonics or assembler directives
   (e.g. '.byte', etc).  */
extern cli_style_option disasm_mnemonic_style;

/* The disassembler style for register names.  */
extern cli_style_option disasm_register_style;

/* The disassembler style for numerical values that are not addresses, this
   includes immediate operands (e.g. in, an add instruction), but also
   address offsets (e.g. in a load instruction).  */
extern cli_style_option disasm_immediate_style;

/* The disassembler style for comments.  */
extern cli_style_option disasm_comment_style;

/* The border style of a TUI window that does not have the focus.  */
extern cli_style_option tui_border_style;

/* The border style of a TUI window that does have the focus.  */
extern cli_style_option tui_active_border_style;

/* The style to use for the GDB version string.  */
extern cli_style_option version_style;

/* True if source styling is enabled.  */
extern bool source_styling;

/* True if disassembler styling is enabled.  */
extern bool disassembler_styling;

/* True if styling is enabled.  */
extern bool cli_styling;

#endif /* CLI_CLI_STYLE_H */
