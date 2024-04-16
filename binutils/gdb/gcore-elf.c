/* Copyright (C) 2021-2024 Free Software Foundation, Inc.

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
#include "gcore-elf.h"
#include "elf-bfd.h"
#include "target.h"
#include "regcache.h"
#include "gdbarch.h"
#include "gdbthread.h"
#include "inferior.h"
#include "regset.h"
#include "gdbsupport/tdesc.h"

/* Structure for passing information from GCORE_COLLECT_THREAD_REGISTERS
   via an iterator to GCORE_COLLECT_REGSET_SECTION_CB. */

struct gcore_elf_collect_regset_section_cb_data
{
  gcore_elf_collect_regset_section_cb_data
	(struct gdbarch *gdbarch, const struct regcache *regcache,
	 bfd *obfd, ptid_t ptid, gdb_signal stop_signal,
	 gdb::unique_xmalloc_ptr<char> *note_data, int *note_size)
    : gdbarch (gdbarch), regcache (regcache), obfd (obfd),
      note_data (note_data), note_size (note_size),
      stop_signal (stop_signal)
  {
    /* The LWP is often not available for bare metal target, in which case
       use the tid instead.  */
    if (ptid.lwp_p ())
      lwp = ptid.lwp ();
    else
      lwp = ptid.tid ();
  }

  struct gdbarch *gdbarch;
  const struct regcache *regcache;
  bfd *obfd;
  gdb::unique_xmalloc_ptr<char> *note_data;
  int *note_size;
  unsigned long lwp;
  enum gdb_signal stop_signal;
  bool abort_iteration = false;
};

/* Callback for ITERATE_OVER_REGSET_SECTIONS that records a single
   regset in the core file note section.  */

static void
gcore_elf_collect_regset_section_cb (const char *sect_name,
				     int supply_size, int collect_size,
				     const struct regset *regset,
				     const char *human_name, void *cb_data)
{
  struct gcore_elf_collect_regset_section_cb_data *data
    = (struct gcore_elf_collect_regset_section_cb_data *) cb_data;
  bool variable_size_section = (regset != nullptr
				&& regset->flags & REGSET_VARIABLE_SIZE);

  gdb_assert (variable_size_section || supply_size == collect_size);

  if (data->abort_iteration)
    return;

  gdb_assert (regset != nullptr && regset->collect_regset != nullptr);

  /* This is intentionally zero-initialized by using std::vector, so
     that any padding bytes in the core file will show as 0.  */
  std::vector<gdb_byte> buf (collect_size);

  regset->collect_regset (regset, data->regcache, -1, buf.data (),
			  collect_size);

  /* PRSTATUS still needs to be treated specially.  */
  if (strcmp (sect_name, ".reg") == 0)
    data->note_data->reset (elfcore_write_prstatus
			    (data->obfd, data->note_data->release (),
			     data->note_size, data->lwp,
			     gdb_signal_to_host (data->stop_signal),
			     buf.data ()));
  else
    data->note_data->reset (elfcore_write_register_note
			    (data->obfd, data->note_data->release (),
			     data->note_size, sect_name, buf.data (),
			     collect_size));

  if (*data->note_data == nullptr)
    data->abort_iteration = true;
}

/* Records the register state of thread PTID out of REGCACHE into the note
   buffer represented by *NOTE_DATA and NOTE_SIZE.  OBFD is the bfd into
   which the core file is being created, and STOP_SIGNAL is the signal that
   cause thread PTID to stop.  */

static void
gcore_elf_collect_thread_registers
	(const struct regcache *regcache, ptid_t ptid, bfd *obfd,
	 gdb::unique_xmalloc_ptr<char> *note_data, int *note_size,
	 enum gdb_signal stop_signal)
{
  struct gdbarch *gdbarch = regcache->arch ();
  gcore_elf_collect_regset_section_cb_data data (gdbarch, regcache, obfd,
						 ptid, stop_signal,
						 note_data, note_size);
  gdbarch_iterate_over_regset_sections
    (gdbarch, gcore_elf_collect_regset_section_cb, &data, regcache);
}

/* See gcore-elf.h.  */

void
gcore_elf_build_thread_register_notes
  (struct gdbarch *gdbarch, struct thread_info *info, gdb_signal stop_signal,
   bfd *obfd, gdb::unique_xmalloc_ptr<char> *note_data, int *note_size)
{
  regcache *regcache
    = get_thread_arch_regcache (info->inf, info->ptid, gdbarch);
  target_fetch_registers (regcache, -1);
  gcore_elf_collect_thread_registers (regcache, info->ptid, obfd,
				      note_data, note_size, stop_signal);
}

/* See gcore-elf.h.  */

void
gcore_elf_make_tdesc_note (struct gdbarch *gdbarch, bfd *obfd,
			   gdb::unique_xmalloc_ptr<char> *note_data,
			   int *note_size)
{
  /* Append the target description to the core file.  */
  const struct target_desc *tdesc = gdbarch_target_desc (gdbarch);
  const char *tdesc_xml
    = tdesc == nullptr ? nullptr : tdesc_get_features_xml (tdesc);
  if (tdesc_xml != nullptr && *tdesc_xml != '\0')
    {
      /* Skip the leading '@'.  */
      if (*tdesc_xml == '@')
	++tdesc_xml;

      /* Include the null terminator in the length.  */
      size_t tdesc_len = strlen (tdesc_xml) + 1;

      /* Now add the target description into the core file.  */
      note_data->reset (elfcore_write_register_note (obfd,
						     note_data->release (),
						     note_size,
						     ".gdb-tdesc", tdesc_xml,
						     tdesc_len));
    }
}
