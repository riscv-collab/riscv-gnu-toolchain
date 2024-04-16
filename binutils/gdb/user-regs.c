/* User visible, per-frame registers, for GDB, the GNU debugger.

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
#include "user-regs.h"
#include "gdbtypes.h"
#include "frame.h"
#include "arch-utils.h"
#include "command.h"
#include "cli/cli-cmds.h"

/* A table of user registers.

   User registers have regnum's that live above of the range [0
   .. gdbarch_num_regs + gdbarch_num_pseudo_regs)
   (which is controlled by the target).
   The target should never see a user register's regnum value.

   Always append, never delete.  By doing this, the relative regnum
   (offset from gdbarch_num_regs + gdbarch_num_pseudo_regs)
    assigned to each user  register never changes.  */

struct user_reg
{
  const char *name;
  /* Avoid the "read" symbol name as it conflicts with a preprocessor symbol
     in the NetBSD header for Stack Smashing Protection, that wraps the read(2)
     syscall.  */
  struct value *(*xread) (frame_info_ptr frame, const void *baton);
  const void *baton;
  struct user_reg *next;
};

/* This structure is named gdb_user_regs instead of user_regs to avoid
   conflicts with any "struct user_regs" in system headers.  For instance,
   on ARM GNU/Linux native builds, nm-linux.h includes <signal.h> includes
   <sys/ucontext.h> includes <sys/procfs.h> includes <sys/user.h>, which
   declares "struct user_regs".  */

struct gdb_user_regs
{
  struct user_reg *first = nullptr;
  struct user_reg **last = nullptr;
};

static void
append_user_reg (struct gdb_user_regs *regs, const char *name,
		 user_reg_read_ftype *xread, const void *baton,
		 struct user_reg *reg)
{
  /* The caller is responsible for allocating memory needed to store
     the register.  By doing this, the function can operate on a
     register list stored in the common heap or a specific obstack.  */
  gdb_assert (reg != NULL);
  reg->name = name;
  reg->xread = xread;
  reg->baton = baton;
  reg->next = NULL;
  if (regs->last == nullptr)
    regs->last = &regs->first;
  (*regs->last) = reg;
  regs->last = &(*regs->last)->next;
}

/* An array of the builtin user registers.  */

static struct gdb_user_regs builtin_user_regs;

void
user_reg_add_builtin (const char *name, user_reg_read_ftype *xread,
		      const void *baton)
{
  append_user_reg (&builtin_user_regs, name, xread, baton,
		   XNEW (struct user_reg));
}

/* Per-architecture user registers.  Start with the builtin user
   registers and then, again, append.  */

static const registry<gdbarch>::key<gdb_user_regs> user_regs_data;

static gdb_user_regs *
get_user_regs (struct gdbarch *gdbarch)
{
  struct gdb_user_regs *regs = user_regs_data.get (gdbarch);
  if (regs == nullptr)
    {
      regs = new struct gdb_user_regs;

      struct obstack *obstack = gdbarch_obstack (gdbarch);
      regs->last = &regs->first;
      for (user_reg *reg = builtin_user_regs.first;
	   reg != NULL;
	   reg = reg->next)
	append_user_reg (regs, reg->name, reg->xread, reg->baton,
			 OBSTACK_ZALLOC (obstack, struct user_reg));
      user_regs_data.set (gdbarch, regs);
    }

  return regs;
}

void
user_reg_add (struct gdbarch *gdbarch, const char *name,
	      user_reg_read_ftype *xread, const void *baton)
{
  struct gdb_user_regs *regs = get_user_regs (gdbarch);
  gdb_assert (regs != NULL);
  append_user_reg (regs, name, xread, baton,
		   GDBARCH_OBSTACK_ZALLOC (gdbarch, struct user_reg));
}

int
user_reg_map_name_to_regnum (struct gdbarch *gdbarch, const char *name,
			     int len)
{
  /* Make life easy, set the len to something reasonable.  */
  if (len < 0)
    len = strlen (name);

  /* Search register name space first - always let an architecture
     specific register override the user registers.  */
  {
    int maxregs = gdbarch_num_cooked_regs (gdbarch);

    for (int i = 0; i < maxregs; i++)
      {
	const char *regname = gdbarch_register_name (gdbarch, i);

	if (len == strlen (regname) && strncmp (regname, name, len) == 0)
	  return i;
      }
  }

  /* Search the user name space.  */
  {
    struct gdb_user_regs *regs = get_user_regs (gdbarch);
    struct user_reg *reg;
    int nr;

    for (nr = 0, reg = regs->first; reg != NULL; reg = reg->next, nr++)
      {
	if ((len < 0 && strcmp (reg->name, name))
	    || (len == strlen (reg->name)
		&& strncmp (reg->name, name, len) == 0))
	  return gdbarch_num_cooked_regs (gdbarch) + nr;
      }
  }

  return -1;
}

static struct user_reg *
usernum_to_user_reg (struct gdbarch *gdbarch, int usernum)
{
  struct gdb_user_regs *regs = get_user_regs (gdbarch);
  struct user_reg *reg;

  for (reg = regs->first; reg != NULL; reg = reg->next)
    {
      if (usernum == 0)
	return reg;
      usernum--;
    }
  return NULL;
}

const char *
user_reg_map_regnum_to_name (struct gdbarch *gdbarch, int regnum)
{
  int maxregs = gdbarch_num_cooked_regs (gdbarch);

  if (regnum < 0)
    return NULL;
  else if (regnum < maxregs)
    return gdbarch_register_name (gdbarch, regnum);
  else
    {
      struct user_reg *reg = usernum_to_user_reg (gdbarch, regnum - maxregs);
      if (reg == NULL)
	return NULL;
      else
	return reg->name;
    }
}

struct value *
value_of_user_reg (int regnum, frame_info_ptr frame)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int maxregs = gdbarch_num_cooked_regs (gdbarch);
  struct user_reg *reg = usernum_to_user_reg (gdbarch, regnum - maxregs);

  gdb_assert (reg != NULL);
  return reg->xread (frame, reg->baton);
}

static void
maintenance_print_user_registers (const char *args, int from_tty)
{
  struct gdbarch *gdbarch = get_current_arch ();
  struct user_reg *reg;
  int regnum;

  struct gdb_user_regs *regs = get_user_regs (gdbarch);
  regnum = gdbarch_num_cooked_regs (gdbarch);

  gdb_printf (" %-11s %3s\n", "Name", "Nr");
  for (reg = regs->first; reg != NULL; reg = reg->next, ++regnum)
    gdb_printf (" %-11s %3d\n", reg->name, regnum);
}

void _initialize_user_regs ();
void
_initialize_user_regs ()
{
  add_cmd ("user-registers", class_maintenance,
	   maintenance_print_user_registers,
	   _("List the names of the current user registers."),
	   &maintenanceprintlist);
}
