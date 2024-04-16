/* Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include "regcache.h"
#include "gdbsupport/def-vector.h"
#include "valprint.h"
#include "remote.h"
#include "reggroups.h"
#include "target.h"
#include "gdbarch.h"
#include "inferior.h"

/* Dump registers from regcache, used for dumping raw registers and
   cooked registers.  */

class register_dump_regcache : public register_dump
{
public:
  register_dump_regcache (regcache *regcache, bool dump_pseudo)
    : register_dump (regcache->arch ()), m_regcache (regcache),
      m_dump_pseudo (dump_pseudo)
  {
  }

protected:
  void dump_reg (ui_file *file, int regnum) override
  {
    if (regnum < 0)
      {
	if (m_dump_pseudo)
	  gdb_printf (file, "Cooked value");
	else
	  gdb_printf (file, "Raw value");
      }
    else
      {
	if (regnum < gdbarch_num_regs (m_gdbarch) || m_dump_pseudo)
	  {
	    auto size = register_size (m_gdbarch, regnum);

	    if (size == 0)
	      return;

	    gdb::byte_vector buf (size);
	    auto status = m_regcache->cooked_read (regnum, buf.data ());

	    if (status == REG_UNKNOWN)
	      gdb_printf (file, "<invalid>");
	    else if (status == REG_UNAVAILABLE)
	      gdb_printf (file, "<unavailable>");
	    else
	      {
		print_hex_chars (file, buf.data (), size,
				 gdbarch_byte_order (m_gdbarch), true);
	      }
	  }
	else
	  {
	    /* Just print "<cooked>" for pseudo register when
	       regcache_dump_raw.  */
	    gdb_printf (file, "<cooked>");
	  }
      }
  }

private:
  regcache *m_regcache;

  /* Dump pseudo registers or not.  */
  const bool m_dump_pseudo;
};

/* Dump from reg_buffer, used when there is no thread or
   registers.  */

class register_dump_reg_buffer : public register_dump, reg_buffer
{
public:
  register_dump_reg_buffer (gdbarch *gdbarch, bool dump_pseudo)
    : register_dump (gdbarch), reg_buffer (gdbarch, dump_pseudo)
  {
  }

protected:
  void dump_reg (ui_file *file, int regnum) override
  {
    if (regnum < 0)
      {
	if (m_has_pseudo)
	  gdb_printf (file, "Cooked value");
	else
	  gdb_printf (file, "Raw value");
      }
    else
      {
	if (regnum < gdbarch_num_regs (m_gdbarch) || m_has_pseudo)
	  {
	    auto size = register_size (m_gdbarch, regnum);

	    if (size == 0)
	      return;

	    auto status = get_register_status (regnum);

	    gdb_assert (status != REG_VALID);

	    if (status == REG_UNKNOWN)
	      gdb_printf (file, "<invalid>");
	    else
	      gdb_printf (file, "<unavailable>");
	  }
	else
	  {
	    /* Just print "<cooked>" for pseudo register when
	       regcache_dump_raw.  */
	    gdb_printf (file, "<cooked>");
	  }
      }
  }
};

/* For "maint print registers".  */

class register_dump_none : public register_dump
{
public:
  register_dump_none (gdbarch *arch)
    : register_dump (arch)
  {}

protected:
  void dump_reg (ui_file *file, int regnum) override
  {}
};

/* For "maint print remote-registers".  */

class register_dump_remote : public register_dump
{
public:
  register_dump_remote (gdbarch *arch)
    : register_dump (arch)
  {}

protected:
  void dump_reg (ui_file *file, int regnum) override
  {
    if (regnum < 0)
      {
	gdb_printf (file, "Rmt Nr  g/G Offset");
      }
    else if (regnum < gdbarch_num_regs (m_gdbarch))
      {
	int pnum, poffset;

	if (remote_register_number_and_offset (m_gdbarch, regnum,
					       &pnum, &poffset))
	  gdb_printf (file, "%7d %11d", pnum, poffset);
      }
  }
};

/* For "maint print register-groups".  */

class register_dump_groups : public register_dump
{
public:
  register_dump_groups (gdbarch *arch)
    : register_dump (arch)
  {}

protected:
  void dump_reg (ui_file *file, int regnum) override
  {
    if (regnum < 0)
      gdb_printf (file, "Groups");
    else
      {
	const char *sep = "";
	for (const struct reggroup *group : gdbarch_reggroups (m_gdbarch))
	  {
	    if (gdbarch_register_reggroup_p (m_gdbarch, regnum, group))
	      {
		gdb_printf (file, "%s%s", sep, group->name ());
		sep = ",";
	      }
	  }
      }
  }
};

enum regcache_dump_what
{
  regcache_dump_none, regcache_dump_raw,
  regcache_dump_cooked, regcache_dump_groups,
  regcache_dump_remote
};

static void
regcache_print (const char *args, enum regcache_dump_what what_to_dump)
{
  /* Where to send output.  */
  stdio_file file;
  ui_file *out;

  if (args == NULL)
    out = gdb_stdout;
  else
    {
      if (!file.open (args, "w"))
	perror_with_name (_("maintenance print architecture"));
      out = &file;
    }

  std::unique_ptr<register_dump> dump;
  std::unique_ptr<regcache> regs;
  gdbarch *gdbarch;

  if (target_has_registers ())
    gdbarch = get_thread_regcache (inferior_thread ())->arch ();
  else
    gdbarch = current_inferior ()->arch ();

  switch (what_to_dump)
    {
    case regcache_dump_none:
      dump.reset (new register_dump_none (gdbarch));
      break;
    case regcache_dump_remote:
      dump.reset (new register_dump_remote (gdbarch));
      break;
    case regcache_dump_groups:
      dump.reset (new register_dump_groups (gdbarch));
      break;
    case regcache_dump_raw:
    case regcache_dump_cooked:
      {
	auto dump_pseudo = (what_to_dump == regcache_dump_cooked);

	if (target_has_registers ())
	  dump.reset (new register_dump_regcache (get_thread_regcache
						    (inferior_thread ()),
						  dump_pseudo));
	else
	  {
	    /* For the benefit of "maint print registers" & co when
	       debugging an executable, allow dumping a regcache even when
	       there is no thread selected / no registers.  */
	    dump.reset (new register_dump_reg_buffer (gdbarch, dump_pseudo));
	  }
      }
      break;
    }

  dump->dump (out);
}

static void
maintenance_print_registers (const char *args, int from_tty)
{
  regcache_print (args, regcache_dump_none);
}

static void
maintenance_print_raw_registers (const char *args, int from_tty)
{
  regcache_print (args, regcache_dump_raw);
}

static void
maintenance_print_cooked_registers (const char *args, int from_tty)
{
  regcache_print (args, regcache_dump_cooked);
}

static void
maintenance_print_register_groups (const char *args, int from_tty)
{
  regcache_print (args, regcache_dump_groups);
}

static void
maintenance_print_remote_registers (const char *args, int from_tty)
{
  regcache_print (args, regcache_dump_remote);
}

void _initialize_regcache_dump ();
void
_initialize_regcache_dump ()
{
  add_cmd ("registers", class_maintenance, maintenance_print_registers,
	   _("Print the internal register configuration.\n"
	     "Takes an optional file parameter."), &maintenanceprintlist);
  add_cmd ("raw-registers", class_maintenance,
	   maintenance_print_raw_registers,
	   _("Print the internal register configuration "
	     "including raw values.\n"
	     "Takes an optional file parameter."), &maintenanceprintlist);
  add_cmd ("cooked-registers", class_maintenance,
	   maintenance_print_cooked_registers,
	   _("Print the internal register configuration "
	     "including cooked values.\n"
	     "Takes an optional file parameter."), &maintenanceprintlist);
  add_cmd ("register-groups", class_maintenance,
	   maintenance_print_register_groups,
	   _("Print the internal register configuration "
	     "including each register's group.\n"
	     "Takes an optional file parameter."),
	   &maintenanceprintlist);
  add_cmd ("remote-registers", class_maintenance,
	   maintenance_print_remote_registers, _("\
Print the internal register configuration including remote register number "
"and g/G packets offset.\n\
Takes an optional file parameter."),
	   &maintenanceprintlist);
}
