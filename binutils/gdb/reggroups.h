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

#ifndef REGGROUPS_H
#define REGGROUPS_H

struct gdbarch;

/* The different register group types.  */
enum reggroup_type {
  /* Used for any register group that should be visible to the user.
     Architecture specific register groups, as well as most of the default
     groups will have this type.  */
  USER_REGGROUP,

  /* Used for a few groups that GDB uses while managing machine state.
     These groups are mostly hidden from the user.  */
  INTERNAL_REGGROUP
};

/* Individual register group.  */

struct reggroup
{
  /* Create a new register group object.  The NAME is not owned by the new
     reggroup object, so must outlive the object.  */
  reggroup (const char *name, enum reggroup_type type)
    : m_name (name),
      m_type (type)
  { /* Nothing.  */ }

  /* Return the name for this register group.  */
  const char *name () const
  { return m_name; }

  /* Return the type of this register group.  */
  enum reggroup_type type () const
  { return m_type; }

private:
  /* The name of this register group.  */
  const char *m_name;

  /* The type of this register group.  */
  enum reggroup_type m_type;
};

/* Pre-defined, user visible, register groups.  */
extern const reggroup *const general_reggroup;
extern const reggroup *const float_reggroup;
extern const reggroup *const system_reggroup;
extern const reggroup *const vector_reggroup;
extern const reggroup *const all_reggroup;

/* Pre-defined, internal, register groups.  */
extern const reggroup *const save_reggroup;
extern const reggroup *const restore_reggroup;

/* Create a new local register group.  */
extern const reggroup *reggroup_new (const char *name,
				     enum reggroup_type type);

/* Create a new register group allocated onto the gdbarch obstack.  */
extern const reggroup *reggroup_gdbarch_new (struct gdbarch *gdbarch,
					     const char *name,
					     enum reggroup_type type);

/* Add register group GROUP to the list of register groups for GDBARCH.  */
extern void reggroup_add (struct gdbarch *gdbarch, const reggroup *group);

/* Return the list of all register groups for GDBARCH.  */
extern const std::vector<const reggroup *> &
	gdbarch_reggroups (struct gdbarch *gdbarch);

/* Find a reggroup by name.  */
extern const reggroup *reggroup_find (struct gdbarch *gdbarch,
				      const char *name);

/* Is REGNUM a member of REGGROUP?  */
extern int default_register_reggroup_p (struct gdbarch *gdbarch, int regnum,
					const struct reggroup *reggroup);

#endif
