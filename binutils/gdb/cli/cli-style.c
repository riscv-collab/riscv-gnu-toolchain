/* CLI colorizing

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

#include "defs.h"
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "cli/cli-setshow.h"
#include "cli/cli-style.h"
#include "source-cache.h"
#include "observable.h"

/* True if styling is enabled.  */

#if defined (__MSDOS__)
bool cli_styling = false;
#else
bool cli_styling = true;
#endif

/* True if source styling is enabled.  Note that this is only
   consulted when cli_styling is true.  */

bool source_styling = true;

/* True if disassembler styling is enabled.  Note that this is only
   consulted when cli_styling is true.  */

bool disassembler_styling = true;

/* Name of colors; must correspond to ui_file_style::basic_color.  */
static const char * const cli_colors[] = {
  "none",
  "black",
  "red",
  "green",
  "yellow",
  "blue",
  "magenta",
  "cyan",
  "white",
  nullptr
};

/* Names of intensities; must correspond to
   ui_file_style::intensity.  */
static const char * const cli_intensities[] = {
  "normal",
  "bold",
  "dim",
  nullptr
};

/* See cli-style.h.  */

cli_style_option file_name_style ("filename", ui_file_style::GREEN);

/* See cli-style.h.  */

cli_style_option function_name_style ("function", ui_file_style::YELLOW);

/* See cli-style.h.  */

cli_style_option variable_name_style ("variable", ui_file_style::CYAN);

/* See cli-style.h.  */

cli_style_option address_style ("address", ui_file_style::BLUE);

/* See cli-style.h.  */

cli_style_option highlight_style ("highlight", ui_file_style::RED);

/* See cli-style.h.  */

cli_style_option title_style ("title", ui_file_style::BOLD);

/* See cli-style.h.  */

cli_style_option tui_border_style ("tui-border", ui_file_style::CYAN);

/* See cli-style.h.  */

cli_style_option tui_active_border_style ("tui-active-border",
					  ui_file_style::CYAN);

/* See cli-style.h.  */

cli_style_option metadata_style ("metadata", ui_file_style::DIM);

/* See cli-style.h.  */

cli_style_option version_style ("version", ui_file_style::MAGENTA,
				ui_file_style::BOLD);

/* See cli-style.h.  */

cli_style_option disasm_mnemonic_style ("mnemonic", ui_file_style::GREEN);

/* See cli-style.h.  */

cli_style_option disasm_register_style ("register", ui_file_style::RED);

/* See cli-style.h.  */

cli_style_option disasm_immediate_style ("immediate", ui_file_style::BLUE);

/* See cli-style.h.  */

cli_style_option disasm_comment_style ("comment", ui_file_style::WHITE,
				       ui_file_style::DIM);

/* See cli-style.h.  */

cli_style_option::cli_style_option (const char *name,
				    ui_file_style::basic_color fg,
				    ui_file_style::intensity intensity)
  : changed (name),
    m_name (name),
    m_foreground (cli_colors[fg - ui_file_style::NONE]),
    m_background (cli_colors[0]),
    m_intensity (cli_intensities[intensity])
{
}

/* See cli-style.h.  */

cli_style_option::cli_style_option (const char *name,
				    ui_file_style::intensity i)
  : changed (name),
    m_name (name),
    m_foreground (cli_colors[0]),
    m_background (cli_colors[0]),
    m_intensity (cli_intensities[i])
{
}

/* Return the color number corresponding to COLOR.  */

static int
color_number (const char *color)
{
  for (int i = 0; i < ARRAY_SIZE (cli_colors); ++i)
    {
      if (color == cli_colors[i])
	return i - 1;
    }
  gdb_assert_not_reached ("color not found");
}

/* See cli-style.h.  */

ui_file_style
cli_style_option::style () const
{
  int fg = color_number (m_foreground);
  int bg = color_number (m_background);
  ui_file_style::intensity intensity = ui_file_style::NORMAL;

  for (int i = 0; i < ARRAY_SIZE (cli_intensities); ++i)
    {
      if (m_intensity == cli_intensities[i])
	{
	  intensity = (ui_file_style::intensity) i;
	  break;
	}
    }

  return ui_file_style (fg, bg, intensity);
}

/* See cli-style.h.  */

void
cli_style_option::do_set_value (const char *ignore, int from_tty,
				struct cmd_list_element *cmd)
{
  cli_style_option *cso = (cli_style_option *) cmd->context ();
  cso->changed.notify ();
}

/* Implements the cli_style_option::do_show_* functions.
   WHAT and VALUE are the property and value to show.
   The style for which WHAT is shown is retrieved from CMD context.  */

static void
do_show (const char *what, struct ui_file *file,
	 struct cmd_list_element *cmd,
	 const char *value)
{
  cli_style_option *cso = (cli_style_option *) cmd->context ();
  gdb_puts (_("The "), file);
  fprintf_styled (file, cso->style (), _("\"%s\" style"), cso->name ());
  gdb_printf (file, _(" %s is: %s\n"), what, value);
}

/* See cli-style.h.  */

void
cli_style_option::do_show_foreground (struct ui_file *file, int from_tty,
				      struct cmd_list_element *cmd,
				      const char *value)
{
  do_show (_("foreground color"), file, cmd, value);
}

/* See cli-style.h.  */

void
cli_style_option::do_show_background (struct ui_file *file, int from_tty,
				      struct cmd_list_element *cmd,
				      const char *value)
{
  do_show (_("background color"), file, cmd, value);
}

/* See cli-style.h.  */

void
cli_style_option::do_show_intensity (struct ui_file *file, int from_tty,
				     struct cmd_list_element *cmd,
				     const char *value)
{
  do_show (_("display intensity"), file, cmd, value);
}

/* See cli-style.h.  */

set_show_commands
cli_style_option::add_setshow_commands (enum command_class theclass,
					const char *prefix_doc,
					struct cmd_list_element **set_list,
					struct cmd_list_element **show_list,
					bool skip_intensity)
{
  set_show_commands prefix_cmds
    = add_setshow_prefix_cmd (m_name, theclass, prefix_doc, prefix_doc,
			      &m_set_list, &m_show_list, set_list, show_list);

  set_show_commands commands;

  commands = add_setshow_enum_cmd
    ("foreground", theclass, cli_colors,
     &m_foreground,
     _("Set the foreground color for this property."),
     _("Show the foreground color for this property."),
     nullptr,
     do_set_value,
     do_show_foreground,
     &m_set_list, &m_show_list);
  commands.set->set_context (this);
  commands.show->set_context (this);

  commands = add_setshow_enum_cmd
    ("background", theclass, cli_colors,
     &m_background,
     _("Set the background color for this property."),
     _("Show the background color for this property."),
     nullptr,
     do_set_value,
     do_show_background,
     &m_set_list, &m_show_list);
  commands.set->set_context (this);
  commands.show->set_context (this);

  if (!skip_intensity)
    {
      commands = add_setshow_enum_cmd
	("intensity", theclass, cli_intensities,
	 &m_intensity,
	 _("Set the display intensity for this property."),
	 _("Show the display intensity for this property."),
	 nullptr,
	 do_set_value,
	 do_show_intensity,
	 &m_set_list, &m_show_list);
      commands.set->set_context (this);
      commands.show->set_context (this);
    }

  return prefix_cmds;
}

cmd_list_element *style_set_list;
cmd_list_element *style_show_list;

/* The command list for 'set style disassembler'.  */

static cmd_list_element *style_disasm_set_list;

/* The command list for 'show style disassembler'.  */

static cmd_list_element *style_disasm_show_list;

static void
set_style_enabled  (const char *args, int from_tty, struct cmd_list_element *c)
{
  g_source_cache.clear ();
  gdb::observers::styling_changed.notify ();
}

static void
show_style_enabled (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  if (cli_styling)
    gdb_printf (file, _("CLI output styling is enabled.\n"));
  else
    gdb_printf (file, _("CLI output styling is disabled.\n"));
}

static void
show_style_sources (struct ui_file *file, int from_tty,
		    struct cmd_list_element *c, const char *value)
{
  if (source_styling)
    gdb_printf (file, _("Source code styling is enabled.\n"));
  else
    gdb_printf (file, _("Source code styling is disabled.\n"));
}

/* Implement 'show style disassembler'.  */

static void
show_style_disassembler (struct ui_file *file, int from_tty,
			 struct cmd_list_element *c, const char *value)
{
  if (disassembler_styling)
    gdb_printf (file, _("Disassembler output styling is enabled.\n"));
  else
    gdb_printf (file, _("Disassembler output styling is disabled.\n"));
}

void _initialize_cli_style ();
void
_initialize_cli_style ()
{
  add_setshow_prefix_cmd ("style", no_class,
			  _("\
Style-specific settings.\n\
Configure various style-related variables, such as colors"),
			  _("\
Style-specific settings.\n\
Configure various style-related variables, such as colors"),
			  &style_set_list, &style_show_list,
			  &setlist, &showlist);

  add_setshow_boolean_cmd ("enabled", no_class, &cli_styling, _("\
Set whether CLI styling is enabled."), _("\
Show whether CLI is enabled."), _("\
If enabled, output to the terminal is styled."),
			   set_style_enabled, show_style_enabled,
			   &style_set_list, &style_show_list);

  add_setshow_boolean_cmd ("sources", no_class, &source_styling, _("\
Set whether source code styling is enabled."), _("\
Show whether source code styling is enabled."), _("\
If enabled, source code is styled.\n"
#ifdef HAVE_SOURCE_HIGHLIGHT
"Note that source styling only works if styling in general is enabled,\n\
see \"show style enabled\"."
#else
"Source highlighting may be disabled in this installation of gdb, because\n\
it was not linked against GNU Source Highlight.  However, it might still be\n\
available if the appropriate extension is available at runtime."
#endif
			   ), set_style_enabled, show_style_sources,
			   &style_set_list, &style_show_list);

  add_setshow_prefix_cmd ("disassembler", no_class,
			  _("\
Style-specific settings for the disassembler.\n\
Configure various disassembler style-related variables."),
			  _("\
Style-specific settings for the disassembler.\n\
Configure various disassembler style-related variables."),
			  &style_disasm_set_list, &style_disasm_show_list,
			  &style_set_list, &style_show_list);

  add_setshow_boolean_cmd ("enabled", no_class, &disassembler_styling, _("\
Set whether disassembler output styling is enabled."), _("\
Show whether disassembler output styling is enabled."), _("\
If enabled, disassembler output is styled.  Disassembler highlighting\n\
requires the Python Pygments library, if this library is not available\n\
then disassembler highlighting will not be possible."
			   ), set_style_enabled, show_style_disassembler,
			   &style_disasm_set_list, &style_disasm_show_list);

  file_name_style.add_setshow_commands (no_class, _("\
Filename display styling.\n\
Configure filename colors and display intensity."),
					&style_set_list, &style_show_list,
					false);

  set_show_commands function_prefix_cmds
    = function_name_style.add_setshow_commands (no_class, _("\
Function name display styling.\n\
Configure function name colors and display intensity"),
						&style_set_list,
						&style_show_list,
						false);

  variable_name_style.add_setshow_commands (no_class, _("\
Variable name display styling.\n\
Configure variable name colors and display intensity"),
					    &style_set_list, &style_show_list,
					    false);

  set_show_commands address_prefix_cmds
    = address_style.add_setshow_commands (no_class, _("\
Address display styling.\n\
Configure address colors and display intensity"),
					  &style_set_list, &style_show_list,
					  false);

  title_style.add_setshow_commands (no_class, _("\
Title display styling.\n\
Configure title colors and display intensity\n\
Some commands (such as \"apropos -v REGEXP\") use the title style to improve\n\
readability."),
				    &style_set_list, &style_show_list,
				    false);

  highlight_style.add_setshow_commands (no_class, _("\
Highlight display styling.\n\
Configure highlight colors and display intensity\n\
Some commands use the highlight style to draw the attention to a part\n\
of their output."),
					&style_set_list, &style_show_list,
					false);

  metadata_style.add_setshow_commands (no_class, _("\
Metadata display styling.\n\
Configure metadata colors and display intensity\n\
The \"metadata\" style is used when GDB displays information about\n\
your data, for example \"<unavailable>\""),
				       &style_set_list, &style_show_list,
				       false);

  tui_border_style.add_setshow_commands (no_class, _("\
TUI border display styling.\n\
Configure TUI border colors\n\
The \"tui-border\" style is used when GDB displays the border of a\n\
TUI window that does not have the focus."),
					 &style_set_list, &style_show_list,
					 true);

  tui_active_border_style.add_setshow_commands (no_class, _("\
TUI active border display styling.\n\
Configure TUI active border colors\n\
The \"tui-active-border\" style is used when GDB displays the border of a\n\
TUI window that does have the focus."),
						&style_set_list,
						&style_show_list,
						true);

  version_style.add_setshow_commands (no_class, _("\
Version string display styling.\n\
Configure colors used to display the GDB version string."),
				      &style_set_list, &style_show_list,
				      false);

  disasm_mnemonic_style.add_setshow_commands (no_class, _("\
Disassembler mnemonic display styling.\n\
Configure the colors and display intensity for instruction mnemonics\n\
in the disassembler output.  The \"disassembler mnemonic\" style is\n\
used to display instruction mnemonics as well as any assembler\n\
directives, e.g. \".byte\", \".word\", etc.\n\
\n\
This style will only be used for targets that support libopcodes based\n\
disassembler styling.  When Python Pygments based styling is used\n\
then this style has no effect."),
					      &style_disasm_set_list,
					      &style_disasm_show_list,
					      false);

  disasm_register_style.add_setshow_commands (no_class, _("\
Disassembler register display styling.\n\
Configure the colors and display intensity for registers in the\n\
disassembler output.\n\
\n\
This style will only be used for targets that support libopcodes based\n\
disassembler styling.  When Python Pygments based styling is used\n\
then this style has no effect."),
					      &style_disasm_set_list,
					      &style_disasm_show_list,
					      false);

  disasm_immediate_style.add_setshow_commands (no_class, _("\
Disassembler immediate display styling.\n\
Configure the colors and display intensity for immediates in the\n\
disassembler output.  The \"disassembler immediate\" style is used for\n\
any number that is not an address, this includes constants in arithmetic\n\
instructions, as well as address offsets in memory access instructions.\n\
\n\
This style will only be used for targets that support libopcodes based\n\
disassembler styling.  When Python Pygments based styling is used\n\
then this style has no effect."),
					       &style_disasm_set_list,
					       &style_disasm_show_list,
					       false);

  disasm_comment_style.add_setshow_commands (no_class, _("\
Disassembler comment display styling.\n\
Configure the colors and display intensity for comments in the\n\
disassembler output.  The \"disassembler comment\" style is used for\n\
the comment character, and everything after the comment character up to\n\
the end of the line.  The comment style overrides any other styling,\n\
e.g. a register name in a comment will use the comment styling.\n\
\n\
This style will only be used for targets that support libopcodes based\n\
disassembler styling.  When Python Pygments based styling is used\n\
then this style has no effect."),
					     &style_disasm_set_list,
					     &style_disasm_show_list,
					     false);

  /* Setup 'disassembler address' style and 'disassembler symbol' style,
     these are aliases for 'address' and 'function' styles respectively.  */
  add_alias_cmd ("address", address_prefix_cmds.set, no_class, 0,
		 &style_disasm_set_list);
  add_alias_cmd ("address", address_prefix_cmds.show, no_class, 0,
		 &style_disasm_show_list);
  add_alias_cmd ("symbol", function_prefix_cmds.set, no_class, 0,
		 &style_disasm_set_list);
  add_alias_cmd ("symbol", function_prefix_cmds.show, no_class, 0,
		 &style_disasm_show_list);
}
