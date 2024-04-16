/* Self tests for GDB command definitions for GDB, the GNU debugger.

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
#include "cli/cli-cmds.h"
#include "cli/cli-decode.h"
#include "gdbsupport/selftest.h"

#include <map>

namespace selftests {

/* Verify some invariants of GDB commands documentation.  */

namespace help_doc_tests {

static unsigned int nr_failed_invariants;

/* Report a broken invariant and increments nr_failed_invariants.  */

static void
broken_doc_invariant (const char *prefix, const char *name, const char *msg)
{
  gdb_printf ("help doc broken invariant: command '%s%s' help doc %s\n",
	      prefix, name, msg);
  nr_failed_invariants++;
}

/* Recursively walk the commandlist structures, and check doc invariants:
   - The first line of the doc must end with a '.'.
   - the doc must not end with a new line.
  If an invariant is not respected, produce a message and increment
  nr_failed_invariants.
  Note that we do not call SELF_CHECK in this function, as we want
  all commands to be checked before making the test fail.  */

static void
check_doc (struct cmd_list_element *commandlist, const char *prefix)
{
  struct cmd_list_element *c;

  /* Walk through the commands.  */
  for (c = commandlist; c; c = c->next)
    {
      /* Checks the doc has a first line terminated with a '.'.  */
      const char *p = c->doc;

      /* Position p on the first LF, or on terminating null byte.  */
      while (*p && *p != '\n')
	p++;
      if (p == c->doc)
	broken_doc_invariant
	  (prefix, c->name,
	   "is missing the first line terminated with a '.' character");
      else if (*(p-1) != '.')
	broken_doc_invariant
	  (prefix, c->name,
	   "first line is not terminated with a '.' character");

      /* Checks the doc is not terminated with a new line.  */
      if (c->doc[strlen (c->doc) - 1] == '\n')
	broken_doc_invariant
	  (prefix, c->name,
	   "has a superfluous trailing end of line");

      /* Check if this command has subcommands and is not an
	 abbreviation.  We skip checking subcommands of abbreviations
	 in order to avoid duplicates in the output.  */
      if (c->is_prefix () && !c->abbrev_flag)
	{
	  /* Recursively call ourselves on the subcommand list,
	     passing the right prefix in.  */
	  check_doc (*c->subcommands, c->prefixname ().c_str ());
	}
    }
}

static void
help_doc_invariants_tests ()
{
  nr_failed_invariants = 0;
  check_doc (cmdlist, "");
  SELF_CHECK (nr_failed_invariants == 0);
}

} /* namespace help_doc_tests */

/* Verify some invariants of GDB command structure.  */

namespace command_structure_tests {

/* Nr of commands in which a duplicated list is found.  */
static unsigned int nr_duplicates = 0;
/* Nr of commands in a list having no valid prefix cmd.  */
static unsigned int nr_invalid_prefixcmd = 0;

/* A map associating a list with the prefix leading to it.  */

static std::map<cmd_list_element **, const char *> lists;

/* Store each command list in lists, associated with the prefix to reach it.  A
   list must only be found once.

   Verifies that all elements of the list have the same non-null prefix
   command.  */

static void
traverse_command_structure (struct cmd_list_element **list,
			    const char *prefix)
{
  struct cmd_list_element *c, *prefixcmd;

  auto dupl = lists.find (list);
  if (dupl != lists.end ())
    {
      gdb_printf ("list %p duplicated,"
		  " reachable via prefix '%s' and '%s'."
		  "  Duplicated list first command is '%s'\n",
		  list,
		  prefix, dupl->second,
		  (*list)->name);
      nr_duplicates++;
      return;
    }

  lists.insert ({list, prefix});

  /* All commands of *list must have a prefix command equal to PREFIXCMD,
     the prefix command of the first command.  */
  if (*list == nullptr)
    prefixcmd = nullptr; /* A prefix command with an empty subcommand list.  */
  else
    prefixcmd = (*list)->prefix;

  /* Walk through the commands.  */
  for (c = *list; c; c = c->next)
    {
      /* If this command has subcommands and is not an alias,
	 traverse the subcommands.  */
      if (c->is_prefix () && !c->is_alias ())
	{
	  /* Recursively call ourselves on the subcommand list,
	     passing the right prefix in.  */
	  traverse_command_structure (c->subcommands, c->prefixname ().c_str ());
	}
      if (prefixcmd != c->prefix
	  || (prefixcmd == nullptr && *list != cmdlist))
	{
	  if (c->prefix == nullptr)
	    gdb_printf ("list %p reachable via prefix '%s'."
			"  command '%s' has null prefixcmd\n",
			list,
			prefix, c->name);
	  else
	    gdb_printf ("list %p reachable via prefix '%s'."
			"  command '%s' has a different prefixcmd\n",
			list,
			prefix, c->name);
	  nr_invalid_prefixcmd++;
	}
    }
}

/* Verify that a list of commands is present in the tree only once.  */

static void
command_structure_invariants_tests ()
{
  nr_duplicates = 0;
  nr_invalid_prefixcmd = 0;

  traverse_command_structure (&cmdlist, "");

  /* Release memory, be ready to be re-run.  */
  lists.clear ();

  SELF_CHECK (nr_duplicates == 0);
  SELF_CHECK (nr_invalid_prefixcmd == 0);
}

}

} /* namespace selftests */

void _initialize_command_def_selftests ();
void
_initialize_command_def_selftests ()
{
  selftests::register_test
    ("help_doc_invariants",
     selftests::help_doc_tests::help_doc_invariants_tests);

  selftests::register_test
    ("command_structure_invariants",
     selftests::command_structure_tests::command_structure_invariants_tests);
}
