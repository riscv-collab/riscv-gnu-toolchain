/* Maintenance commands for testing the settings framework.

   Copyright (C) 2019-2024 Free Software Foundation, Inc.

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
#include "gdbcmd.h"

/* Command list for "maint set test-settings".  */
static cmd_list_element *maintenance_set_test_settings_list;

/* Command list for "maint show test-settings".  */
static cmd_list_element *maintenance_show_test_settings_list;

/* Control variables for all the "maintenance set/show test-settings
   xxx" commands.  */

static bool maintenance_test_settings_boolean;

static auto_boolean maintenance_test_settings_auto_boolean = AUTO_BOOLEAN_AUTO;

static unsigned int maintenance_test_settings_uinteger;

static int maintenance_test_settings_integer;

static int maintenance_test_settings_zinteger;

static unsigned int maintenance_test_settings_zuinteger;

static int maintenance_test_settings_zuinteger_unlimited;

static std::string maintenance_test_settings_string;

static std::string maintenance_test_settings_string_noescape;

static std::string maintenance_test_settings_optional_filename;

static std::string maintenance_test_settings_filename;

/* Enum values for the "maintenance set/show test-settings boolean"
   commands.  */
static const char maintenance_test_settings_xxx[] = "xxx";
static const char maintenance_test_settings_yyy[] = "yyy";
static const char maintenance_test_settings_zzz[] = "zzz";

static const char *const maintenance_test_settings_enums[] = {
  maintenance_test_settings_xxx,
  maintenance_test_settings_yyy,
  maintenance_test_settings_zzz,
  nullptr
};

static const char *maintenance_test_settings_enum
  = maintenance_test_settings_xxx;

/* The "maintenance show test-settings xxx" commands.  */

static void
maintenance_show_test_settings_value_cmd
  (struct ui_file *file, int from_tty,
   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, (("%s\n")), value);
}


void _initialize_maint_test_settings ();
void
_initialize_maint_test_settings ()
{
  maintenance_test_settings_filename = "/foo/bar";

  add_setshow_prefix_cmd ("test-settings", class_maintenance,
			  _("\
Set GDB internal variables used for set/show command infrastructure testing."),
			  _("\
Show GDB internal variables used for set/show command infrastructure testing."),
			  &maintenance_set_test_settings_list,
			  &maintenance_show_test_settings_list,
			  &maintenance_set_cmdlist, &maintenance_show_cmdlist);

  add_setshow_boolean_cmd ("boolean", class_maintenance,
			   &maintenance_test_settings_boolean, _("\
command used for internal testing."), _("\
command used for internal testing."),
			   nullptr, /* help_doc */
			   nullptr, /* set_cmd */
			   maintenance_show_test_settings_value_cmd,
			   &maintenance_set_test_settings_list,
			   &maintenance_show_test_settings_list);

  add_setshow_auto_boolean_cmd ("auto-boolean", class_maintenance,
				&maintenance_test_settings_auto_boolean, _("\
command used for internal testing."), _("\
command used for internal testing."),
				nullptr, /* help_doc */
				nullptr, /* set_cmd */
				maintenance_show_test_settings_value_cmd,
				&maintenance_set_test_settings_list,
				&maintenance_show_test_settings_list);

  add_setshow_uinteger_cmd ("uinteger", class_maintenance,
			   &maintenance_test_settings_uinteger, _("\
command used for internal testing."), _("\
command used for internal testing."),
			    nullptr, /* help_doc */
			    nullptr, /* set_cmd */
			    maintenance_show_test_settings_value_cmd,
			    &maintenance_set_test_settings_list,
			    &maintenance_show_test_settings_list);

  add_setshow_integer_cmd ("integer", class_maintenance,
			   &maintenance_test_settings_integer, _("\
command used for internal testing."), _("\
command used for internal testing."),
			   nullptr, /* help_doc */
			   nullptr, /* set_cmd */
			   maintenance_show_test_settings_value_cmd,
			   &maintenance_set_test_settings_list,
			   &maintenance_show_test_settings_list);

  add_setshow_string_cmd ("string", class_maintenance,
     &maintenance_test_settings_string, _("\
command used for internal testing."), _("\
command used for internal testing."),
     nullptr, /* help_doc */
     nullptr, /* set_cmd */
			  maintenance_show_test_settings_value_cmd,
			  &maintenance_set_test_settings_list,
			  &maintenance_show_test_settings_list);

  add_setshow_string_noescape_cmd
    ("string-noescape", class_maintenance,
     &maintenance_test_settings_string_noescape, _("\
command used for internal testing."), _("\
command used for internal testing."),
     nullptr, /* help_doc */
     nullptr, /* set_cmd */
     maintenance_show_test_settings_value_cmd,
     &maintenance_set_test_settings_list,
     &maintenance_show_test_settings_list);

  add_setshow_optional_filename_cmd
    ("optional-filename", class_maintenance,
     &maintenance_test_settings_optional_filename, _("\
command used for internal testing."), _("\
command used for internal testing."),
     nullptr, /* help_doc */
     nullptr, /* set_cmd */
     maintenance_show_test_settings_value_cmd,
     &maintenance_set_test_settings_list,
     &maintenance_show_test_settings_list);

  add_setshow_filename_cmd ("filename", class_maintenance,
			    &maintenance_test_settings_filename, _("\
command used for internal testing."), _("\
command used for internal testing."),
			    nullptr, /* help_doc */
			    nullptr, /* set_cmd */
			    maintenance_show_test_settings_value_cmd,
			    &maintenance_set_test_settings_list,
			    &maintenance_show_test_settings_list);

  add_setshow_zinteger_cmd ("zinteger", class_maintenance,
			    &maintenance_test_settings_zinteger, _("\
command used for internal testing."), _("\
command used for internal testing."),
			    nullptr, /* help_doc */
			    nullptr, /* set_cmd */
			    maintenance_show_test_settings_value_cmd,
			    &maintenance_set_test_settings_list,
			    &maintenance_show_test_settings_list);

  add_setshow_zuinteger_cmd ("zuinteger", class_maintenance,
			     &maintenance_test_settings_zuinteger, _("\
command used for internal testing."), _("\
command used for internal testing."),
			     nullptr, /* help_doc */
			     nullptr, /* set_cmd */
			     maintenance_show_test_settings_value_cmd,
			     &maintenance_set_test_settings_list,
			     &maintenance_show_test_settings_list);

  add_setshow_zuinteger_unlimited_cmd
    ("zuinteger-unlimited", class_maintenance,
     &maintenance_test_settings_zuinteger_unlimited, _("\
command used for internal testing."), _("\
command used for internal testing."),
     nullptr, /* help_doc */
     nullptr, /* set_cmd */
     maintenance_show_test_settings_value_cmd,
     &maintenance_set_test_settings_list,
     &maintenance_show_test_settings_list);

  add_setshow_enum_cmd ("enum", class_maintenance,
			maintenance_test_settings_enums,
			&maintenance_test_settings_enum, _("\
command used for internal testing."), _("\
command used for internal testing."),
			nullptr, /* help_doc */
			nullptr, /* set_cmd */
			maintenance_show_test_settings_value_cmd,
			&maintenance_set_test_settings_list,
			&maintenance_show_test_settings_list);
}
