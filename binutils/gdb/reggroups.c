/* Register groupings for GDB, the GNU debugger.

   Copyright (C) 2002-2024 Free Software Foundation, Inc.

   Contributed by Red Hat.

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
#include "reggroups.h"
#include "gdbtypes.h"
#include "regcache.h"
#include "command.h"
#include "gdbcmd.h"
#include "gdbsupport/gdb_obstack.h"

/* See reggroups.h.  */

const reggroup *
reggroup_new (const char *name, enum reggroup_type type)
{
  return new reggroup (name, type);
}

/* See reggroups.h.  */

const reggroup *
reggroup_gdbarch_new (struct gdbarch *gdbarch, const char *name,
		      enum reggroup_type type)
{
  name = gdbarch_obstack_strdup (gdbarch, name);
  return obstack_new<struct reggroup> (gdbarch_obstack (gdbarch),
				       name, type);
}

/* A container holding all the register groups for a particular
   architecture.  */

struct reggroups
{
  reggroups ()
  {
    /* Add the default groups.  */
    add (general_reggroup);
    add (float_reggroup);
    add (system_reggroup);
    add (vector_reggroup);
    add (all_reggroup);
    add (save_reggroup);
    add (restore_reggroup);
  }

  DISABLE_COPY_AND_ASSIGN (reggroups);

  /* Add GROUP to the list of register groups.  */

  void add (const reggroup *group)
  {
    gdb_assert (group != nullptr);

    auto find_by_name = [group] (const reggroup *g)
      {
	return streq (group->name (), g->name ());
      };
    gdb_assert (std::find_if (m_groups.begin (), m_groups.end (), find_by_name)
		== m_groups.end ());

    m_groups.push_back (group);
  }

  /* The number of register groups.  */

  std::vector<struct reggroup *>::size_type
  size () const
  {
    return m_groups.size ();
  }

  /* Return a reference to the list of all groups.  */

  const std::vector<const struct reggroup *> &
  groups () const
  {
    return m_groups;
  }

private:
  /* The register groups.  */
  std::vector<const struct reggroup *> m_groups;
};

/* Key used to lookup register group data from a gdbarch.  */

static const registry<gdbarch>::key<reggroups> reggroups_data;

/* Get the reggroups for the architecture, creating if necessary.  */

static reggroups *
get_reggroups (struct gdbarch *gdbarch)
{
  struct reggroups *groups = reggroups_data.get (gdbarch);
  if (groups == nullptr)
    groups = reggroups_data.emplace (gdbarch);
  return groups;
}

/* See reggroups.h.  */

void
reggroup_add (struct gdbarch *gdbarch, const reggroup *group)
{
  struct reggroups *groups = get_reggroups (gdbarch);

  gdb_assert (groups != nullptr);
  gdb_assert (group != nullptr);

  groups->add (group);
}

/* See reggroups.h.  */
const std::vector<const reggroup *> &
gdbarch_reggroups (struct gdbarch *gdbarch)
{
  struct reggroups *groups = get_reggroups (gdbarch);
  gdb_assert (groups != nullptr);
  gdb_assert (groups->size () > 0);
  return groups->groups ();
}

/* See reggroups.h.  */

int
default_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
			     const struct reggroup *group)
{
  int vector_p;
  int float_p;
  int raw_p;

  if (*gdbarch_register_name (gdbarch, regnum) == '\0')
    return 0;
  if (group == all_reggroup)
    return 1;
  vector_p = register_type (gdbarch, regnum)->is_vector ();
  float_p = (register_type (gdbarch, regnum)->code () == TYPE_CODE_FLT
	     || (register_type (gdbarch, regnum)->code ()
		 == TYPE_CODE_DECFLOAT));
  raw_p = regnum < gdbarch_num_regs (gdbarch);
  if (group == float_reggroup)
    return float_p;
  if (group == vector_reggroup)
    return vector_p;
  if (group == general_reggroup)
    return (!vector_p && !float_p);
  if (group == save_reggroup || group == restore_reggroup)
    return raw_p;
  return 0;
}

/* See reggroups.h.  */

const reggroup *
reggroup_find (struct gdbarch *gdbarch, const char *name)
{
  for (const struct reggroup *group : gdbarch_reggroups (gdbarch))
    {
      if (strcmp (name, group->name ()) == 0)
	return group;
    }
  return NULL;
}

/* Dump out a table of register groups for the current architecture.  */

static void
reggroups_dump (struct gdbarch *gdbarch, struct ui_file *file)
{
  static constexpr const char *fmt = " %-10s %-10s\n";

  gdb_printf (file, fmt, "Group", "Type");

  for (const struct reggroup *group : gdbarch_reggroups (gdbarch))
    {
      /* Group name.  */
      const char *name = group->name ();

      /* Group type.  */
      const char *type;

      switch (group->type ())
	{
	case USER_REGGROUP:
	  type = "user";
	  break;
	case INTERNAL_REGGROUP:
	  type = "internal";
	  break;
	default:
	  internal_error (_("bad switch"));
	}

      /* Note: If you change this, be sure to also update the
	 documentation.  */

      gdb_printf (file, fmt, name, type);
    }
}

/* Implement 'maintenance print reggroups' command.  */

static void
maintenance_print_reggroups (const char *args, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();

  if (args == NULL)
    reggroups_dump (gdbarch, gdb_stdout);
  else
    {
      stdio_file file;

      if (!file.open (args, "w"))
	perror_with_name (_("maintenance print reggroups"));
      reggroups_dump (gdbarch, &file);
    }
}

/* Pre-defined register groups.  */
static const reggroup general_group = { "general", USER_REGGROUP };
static const reggroup float_group = { "float", USER_REGGROUP };
static const reggroup system_group = { "system", USER_REGGROUP };
static const reggroup vector_group = { "vector", USER_REGGROUP };
static const reggroup all_group = { "all", USER_REGGROUP };
static const reggroup save_group = { "save", INTERNAL_REGGROUP };
static const reggroup restore_group = { "restore", INTERNAL_REGGROUP };

const reggroup *const general_reggroup = &general_group;
const reggroup *const float_reggroup = &float_group;
const reggroup *const system_reggroup = &system_group;
const reggroup *const vector_reggroup = &vector_group;
const reggroup *const all_reggroup = &all_group;
const reggroup *const save_reggroup = &save_group;
const reggroup *const restore_reggroup = &restore_group;

void _initialize_reggroup ();
void
_initialize_reggroup ()
{
  add_cmd ("reggroups", class_maintenance,
	   maintenance_print_reggroups, _("\
Print the internal register group names.\n\
Takes an optional file parameter."),
	   &maintenanceprintlist);

}
