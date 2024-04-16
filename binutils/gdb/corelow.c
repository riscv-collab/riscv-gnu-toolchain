/* Core dump and executable file functions below target vector, for GDB.

   Copyright (C) 1986-2024 Free Software Foundation, Inc.

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
#include <signal.h>
#include <fcntl.h>
#include "frame.h"
#include "inferior.h"
#include "infrun.h"
#include "symtab.h"
#include "command.h"
#include "bfd.h"
#include "target.h"
#include "process-stratum-target.h"
#include "gdbcore.h"
#include "gdbthread.h"
#include "regcache.h"
#include "regset.h"
#include "symfile.h"
#include "exec.h"
#include "readline/tilde.h"
#include "solib.h"
#include "solist.h"
#include "filenames.h"
#include "progspace.h"
#include "objfiles.h"
#include "gdb_bfd.h"
#include "completer.h"
#include "gdbsupport/filestuff.h"
#include "build-id.h"
#include "gdbsupport/pathstuff.h"
#include "gdbsupport/scoped_fd.h"
#include "gdbsupport/x86-xstate.h"
#include "debuginfod-support.h"
#include <unordered_map>
#include <unordered_set>
#include "gdbcmd.h"
#include "xml-tdesc.h"
#include "memtag.h"

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

/* The core file target.  */

static const target_info core_target_info = {
  "core",
  N_("Local core dump file"),
  N_("Use a core file as a target.\n\
Specify the filename of the core file.")
};

class core_target final : public process_stratum_target
{
public:
  core_target ();

  const target_info &info () const override
  { return core_target_info; }

  void close () override;
  void detach (inferior *, int) override;
  void fetch_registers (struct regcache *, int) override;

  enum target_xfer_status xfer_partial (enum target_object object,
					const char *annex,
					gdb_byte *readbuf,
					const gdb_byte *writebuf,
					ULONGEST offset, ULONGEST len,
					ULONGEST *xfered_len) override;
  void files_info () override;

  bool thread_alive (ptid_t ptid) override;
  const struct target_desc *read_description () override;

  std::string pid_to_str (ptid_t) override;

  const char *thread_name (struct thread_info *) override;

  bool has_all_memory () override { return true; }
  bool has_memory () override;
  bool has_stack () override;
  bool has_registers () override;
  bool has_execution (inferior *inf) override { return false; }

  bool info_proc (const char *, enum info_proc_what) override;

  bool supports_memory_tagging () override;

  /* Core file implementation of fetch_memtags.  Fetch the memory tags from
     core file notes.  */
  bool fetch_memtags (CORE_ADDR address, size_t len,
		      gdb::byte_vector &tags, int type) override;

  x86_xsave_layout fetch_x86_xsave_layout () override;

  /* A few helpers.  */

  /* Getter, see variable definition.  */
  struct gdbarch *core_gdbarch ()
  {
    return m_core_gdbarch;
  }

  /* See definition.  */
  void get_core_register_section (struct regcache *regcache,
				  const struct regset *regset,
				  const char *name,
				  int section_min_size,
				  const char *human_name,
				  bool required);

  /* See definition.  */
  void info_proc_mappings (struct gdbarch *gdbarch);

private: /* per-core data */

  /* Get rid of the core inferior.  */
  void clear_core ();

  /* The core's section table.  Note that these target sections are
     *not* mapped in the current address spaces' set of target
     sections --- those should come only from pure executable or
     shared library bfds.  The core bfd sections are an implementation
     detail of the core target, just like ptrace is for unix child
     targets.  */
  std::vector<target_section> m_core_section_table;

  /* File-backed address space mappings: some core files include
     information about memory mapped files.  */
  std::vector<target_section> m_core_file_mappings;

  /* Unavailable mappings.  These correspond to pathnames which either
     weren't found or could not be opened.  Knowing these addresses can
     still be useful.  */
  std::vector<mem_range> m_core_unavailable_mappings;

  /* Build m_core_file_mappings.  Called from the constructor.  */
  void build_file_mappings ();

  /* Helper method for xfer_partial.  */
  enum target_xfer_status xfer_memory_via_mappings (gdb_byte *readbuf,
						    const gdb_byte *writebuf,
						    ULONGEST offset,
						    ULONGEST len,
						    ULONGEST *xfered_len);

  /* FIXME: kettenis/20031023: Eventually this field should
     disappear.  */
  struct gdbarch *m_core_gdbarch = NULL;
};

core_target::core_target ()
{
  /* Find a first arch based on the BFD.  We need the initial gdbarch so
     we can setup the hooks to find a target description.  */
  m_core_gdbarch = gdbarch_from_bfd (core_bfd);

  /* If the arch is able to read a target description from the core, it
     could yield a more specific gdbarch.  */
  const struct target_desc *tdesc = read_description ();

  if (tdesc != nullptr)
    {
      struct gdbarch_info info;
      info.abfd = core_bfd;
      info.target_desc = tdesc;
      m_core_gdbarch = gdbarch_find_by_info (info);
    }

  if (!m_core_gdbarch
      || !gdbarch_iterate_over_regset_sections_p (m_core_gdbarch))
    error (_("\"%s\": Core file format not supported"),
	   bfd_get_filename (core_bfd));

  /* Find the data section */
  m_core_section_table = build_section_table (core_bfd);

  build_file_mappings ();
}

/* Construct the table for file-backed mappings if they exist.

   For each unique path in the note, we'll open a BFD with a bfd
   target of "binary".  This is an unstructured bfd target upon which
   we'll impose a structure from the mappings in the architecture-specific
   mappings note.  A BFD section is allocated and initialized for each
   file-backed mapping.

   We take care to not share already open bfds with other parts of
   GDB; in particular, we don't want to add new sections to existing
   BFDs.  We do, however, ensure that the BFDs that we allocate here
   will go away (be deallocated) when the core target is detached.  */

void
core_target::build_file_mappings ()
{
  std::unordered_map<std::string, struct bfd *> bfd_map;
  std::unordered_set<std::string> unavailable_paths;

  /* See linux_read_core_file_mappings() in linux-tdep.c for an example
     read_core_file_mappings method.  */
  gdbarch_read_core_file_mappings (m_core_gdbarch, core_bfd,

    /* After determining the number of mappings, read_core_file_mappings
       will invoke this lambda.  */
    [&] (ULONGEST)
      {
      },

    /* read_core_file_mappings will invoke this lambda for each mapping
       that it finds.  */
    [&] (int num, ULONGEST start, ULONGEST end, ULONGEST file_ofs,
	 const char *filename, const bfd_build_id *build_id)
      {
	/* Architecture-specific read_core_mapping methods are expected to
	   weed out non-file-backed mappings.  */
	gdb_assert (filename != nullptr);

	if (unavailable_paths.find (filename) != unavailable_paths.end ())
	  {
	    /* We have already seen some mapping for FILENAME but failed to
	       find/open the file.  There is no point in trying the same
	       thing again so just record that the range [start, end) is
	       unavailable.  */
	    m_core_unavailable_mappings.emplace_back (start, end - start);
	    return;
	  }

	struct bfd *bfd = bfd_map[filename];
	if (bfd == nullptr)
	  {
	    /* Use exec_file_find() to do sysroot expansion.  It'll
	       also strip the potential sysroot "target:" prefix.  If
	       there is no sysroot, an equivalent (possibly more
	       canonical) pathname will be provided.  */
	    gdb::unique_xmalloc_ptr<char> expanded_fname
	      = exec_file_find (filename, NULL);

	    if (expanded_fname == nullptr && build_id != nullptr)
	      debuginfod_exec_query (build_id->data, build_id->size,
				     filename, &expanded_fname);

	    if (expanded_fname == nullptr)
	      {
		m_core_unavailable_mappings.emplace_back (start, end - start);
		unavailable_paths.insert (filename);
		warning (_("Can't open file %s during file-backed mapping "
			   "note processing"),
			 filename);
		return;
	      }

	    bfd = bfd_openr (expanded_fname.get (), "binary");

	    if (bfd == nullptr || !bfd_check_format (bfd, bfd_object))
	      {
		m_core_unavailable_mappings.emplace_back (start, end - start);
		unavailable_paths.insert (filename);
		warning (_("Can't open file %s which was expanded to %s "
			   "during file-backed mapping note processing"),
			 filename, expanded_fname.get ());

		if (bfd != nullptr)
		  bfd_close (bfd);
		return;
	      }
	    /* Ensure that the bfd will be closed when core_bfd is closed. 
	       This can be checked before/after a core file detach via
	       "maint info bfds".  */
	    gdb_bfd_record_inclusion (core_bfd, bfd);
	    bfd_map[filename] = bfd;
	  }

	/* Make new BFD section.  All sections have the same name,
	   which is permitted by bfd_make_section_anyway().  */
	asection *sec = bfd_make_section_anyway (bfd, "load");
	if (sec == nullptr)
	  error (_("Can't make section"));
	sec->filepos = file_ofs;
	bfd_set_section_flags (sec, SEC_READONLY | SEC_HAS_CONTENTS);
	bfd_set_section_size (sec, end - start);
	bfd_set_section_vma (sec, start);
	bfd_set_section_lma (sec, start);
	bfd_set_section_alignment (sec, 2);

	/* Set target_section fields.  */
	m_core_file_mappings.emplace_back (start, end, sec);

	/* If this is a bfd of a shared library, record its soname
	   and build id.  */
	if (build_id != nullptr)
	  {
	    gdb::unique_xmalloc_ptr<char> soname
	      = gdb_bfd_read_elf_soname (bfd->filename);
	    if (soname != nullptr)
	      set_cbfd_soname_build_id (current_program_space->cbfd,
					soname.get (), build_id);
	  }
      });

  normalize_mem_ranges (&m_core_unavailable_mappings);
}

/* An arbitrary identifier for the core inferior.  */
#define CORELOW_PID 1

void
core_target::clear_core ()
{
  if (core_bfd)
    {
      switch_to_no_thread ();    /* Avoid confusion from thread
				    stuff.  */
      exit_inferior (current_inferior ());

      /* Clear out solib state while the bfd is still open.  See
	 comments in clear_solib in solib.c.  */
      clear_solib ();

      current_program_space->cbfd.reset (nullptr);
    }
}

/* Close the core target.  */

void
core_target::close ()
{
  clear_core ();

  /* Core targets are heap-allocated (see core_target_open), so here
     we delete ourselves.  */
  delete this;
}

/* Look for sections whose names start with `.reg/' so that we can
   extract the list of threads in a core file.  */

/* If ASECT is a section whose name begins with '.reg/' then extract the
   lwpid after the '/' and create a new thread in INF.

   If REG_SECT is not nullptr, and the both ASECT and REG_SECT point at the
   same position in the parent bfd object then switch to the newly created
   thread, otherwise, the selected thread is left unchanged.  */

static void
add_to_thread_list (asection *asect, asection *reg_sect, inferior *inf)
{
  if (!startswith (bfd_section_name (asect), ".reg/"))
    return;

  int lwpid = atoi (bfd_section_name (asect) + 5);
  ptid_t ptid (inf->pid, lwpid);
  thread_info *thr = add_thread (inf->process_target (), ptid);

  /* Warning, Will Robinson, looking at BFD private data! */

  if (reg_sect != NULL
      && asect->filepos == reg_sect->filepos)	/* Did we find .reg?  */
    switch_to_thread (thr);			/* Yes, make it current.  */
}

/* Issue a message saying we have no core to debug, if FROM_TTY.  */

static void
maybe_say_no_core_file_now (int from_tty)
{
  if (from_tty)
    gdb_printf (_("No core file now.\n"));
}

/* Backward compatibility with old way of specifying core files.  */

void
core_file_command (const char *filename, int from_tty)
{
  dont_repeat ();		/* Either way, seems bogus.  */

  if (filename == NULL)
    {
      if (core_bfd != NULL)
	{
	  target_detach (current_inferior (), from_tty);
	  gdb_assert (core_bfd == NULL);
	}
      else
	maybe_say_no_core_file_now (from_tty);
    }
  else
    core_target_open (filename, from_tty);
}

/* A vmcore file is a core file created by the Linux kernel at the point of
   a crash.  Each thread in the core file represents a real CPU core, and
   the lwpid for each thread is the pid of the process that was running on
   that core at the moment of the crash.

   However, not every CPU core will have been running a process, some cores
   will be idle.  For these idle cores the CPU writes an lwpid of 0.  And
   of course, multiple cores might be idle, so there could be multiple
   threads with an lwpid of 0.

   The problem is GDB doesn't really like threads with an lwpid of 0; GDB
   presents such a thread as a process rather than a thread.  And GDB
   certainly doesn't like multiple threads having the same lwpid, each time
   a new thread is seen with the same lwpid the earlier thread (with the
   same lwpid) will be deleted.

   This function addresses both of these problems by assigning a fake lwpid
   to any thread with an lwpid of 0.

   GDB finds the lwpid information by looking at the bfd section names
   which include the lwpid, e.g. .reg/NN where NN is the lwpid.  This
   function looks though all the section names looking for sections named
   .reg/NN.  If any sections are found where NN == 0, then we assign a new
   unique value of NN.  Then, in a second pass, any sections ending /0 are
   assigned their new number.

   Remember, a core file may contain multiple register sections for
   different register sets, but the sets are always grouped by thread, so
   we can figure out which registers should be assigned the same new
   lwpid.  For example, consider a core file containing:

     .reg/0, .reg2/0, .reg/0, .reg2/0

   This represents two threads, each thread contains a .reg and .reg2
   register set.  The .reg represents the start of each thread.  After
   renaming the sections will now look like this:

     .reg/1, .reg2/1, .reg/2, .reg2/2

   After calling this function the rest of the core file handling code can
   treat this core file just like any other core file.  */

static void
rename_vmcore_idle_reg_sections (bfd *abfd, inferior *inf)
{
  /* Map from the bfd section to its lwpid (the /NN number).  */
  std::vector<std::pair<asection *, int>> sections_and_lwpids;

  /* The set of all /NN numbers found.  Needed so we can easily find unused
     numbers in the case that we need to rename some sections.  */
  std::unordered_set<int> all_lwpids;

  /* A count of how many sections called .reg/0 we have found.  */
  unsigned zero_lwpid_count = 0;

  /* Look for all the .reg sections.  Record the section object and the
     lwpid which is extracted from the section name.  Spot if any have an
     lwpid of zero.  */
  for (asection *sect : gdb_bfd_sections (core_bfd))
    {
      if (startswith (bfd_section_name (sect), ".reg/"))
	{
	  int lwpid = atoi (bfd_section_name (sect) + 5);
	  sections_and_lwpids.emplace_back (sect, lwpid);
	  all_lwpids.insert (lwpid);
	  if (lwpid == 0)
	    zero_lwpid_count++;
	}
    }

  /* If every ".reg/NN" section has a non-zero lwpid then we don't need to
     do any renaming.  */
  if (zero_lwpid_count == 0)
    return;

  /* Assign a new number to any .reg sections with an lwpid of 0.  */
  int new_lwpid = 1;
  for (auto &sect_and_lwpid : sections_and_lwpids)
    if (sect_and_lwpid.second == 0)
      {
	while (all_lwpids.find (new_lwpid) != all_lwpids.end ())
	  new_lwpid++;
	sect_and_lwpid.second = new_lwpid;
	new_lwpid++;
      }

  /* Now update the names of any sections with an lwpid of 0.  This is
     more than just the .reg sections we originally found.  */
  std::string replacement_lwpid_str;
  auto iter = sections_and_lwpids.begin ();
  int replacement_lwpid = 0;
  for (asection *sect : gdb_bfd_sections (core_bfd))
    {
      if (iter != sections_and_lwpids.end () && sect == iter->first)
	{
	  gdb_assert (startswith (bfd_section_name (sect), ".reg/"));

	  int lwpid = atoi (bfd_section_name (sect) + 5);
	  if (lwpid == iter->second)
	    {
	      /* This section was not given a new number.  */
	      gdb_assert (lwpid != 0);
	      replacement_lwpid = 0;
	    }
	  else
	    {
	      replacement_lwpid = iter->second;
	      ptid_t ptid (inf->pid, replacement_lwpid);
	      if (!replacement_lwpid_str.empty ())
		replacement_lwpid_str += ", ";
	      replacement_lwpid_str += target_pid_to_str (ptid);
	    }

	  iter++;
	}

      if (replacement_lwpid != 0)
	{
	  const char *name = bfd_section_name (sect);
	  size_t len = strlen (name);

	  if (strncmp (name + len - 2, "/0", 2) == 0)
	    {
	      /* This section needs a new name.  */
	      std::string name_str
		= string_printf ("%.*s/%d",
				 static_cast<int> (len - 2),
				 name, replacement_lwpid);
	      char *name_buf
		= static_cast<char *> (bfd_alloc (abfd, name_str.size () + 1));
	      if (name_buf == nullptr)
		error (_("failed to allocate space for section name '%s'"),
		       name_str.c_str ());
	      memcpy (name_buf, name_str.c_str(), name_str.size () + 1);
	      bfd_rename_section (sect, name_buf);
	    }
	}
    }

  if (zero_lwpid_count == 1)
    warning (_("found thread with pid 0, assigned replacement Target Id: %s"),
	     replacement_lwpid_str.c_str ());
  else
    warning (_("found threads with pid 0, assigned replacement Target Ids: %s"),
	     replacement_lwpid_str.c_str ());
}

/* Locate (and load) an executable file (and symbols) given the core file
   BFD ABFD.  */

static void
locate_exec_from_corefile_build_id (bfd *abfd, int from_tty)
{
  const bfd_build_id *build_id = build_id_bfd_get (abfd);
  if (build_id == nullptr)
    return;

  gdb_bfd_ref_ptr execbfd
    = build_id_to_exec_bfd (build_id->size, build_id->data);

  if (execbfd == nullptr)
    {
      /* Attempt to query debuginfod for the executable.  */
      gdb::unique_xmalloc_ptr<char> execpath;
      scoped_fd fd = debuginfod_exec_query (build_id->data, build_id->size,
					    abfd->filename, &execpath);

      if (fd.get () >= 0)
	{
	  execbfd = gdb_bfd_open (execpath.get (), gnutarget);

	  if (execbfd == nullptr)
	    warning (_("\"%s\" from debuginfod cannot be opened as bfd: %s"),
		     execpath.get (),
		     gdb_bfd_errmsg (bfd_get_error (), nullptr).c_str ());
	  else if (!build_id_verify (execbfd.get (), build_id->size,
				     build_id->data))
	    execbfd.reset (nullptr);
	}
    }

  if (execbfd != nullptr)
    {
      exec_file_attach (bfd_get_filename (execbfd.get ()), from_tty);
      symbol_file_add_main (bfd_get_filename (execbfd.get ()),
			    symfile_add_flag (from_tty ? SYMFILE_VERBOSE : 0));
    }
}

/* See gdbcore.h.  */

void
core_target_open (const char *arg, int from_tty)
{
  const char *p;
  int siggy;
  int scratch_chan;
  int flags;

  target_preopen (from_tty);
  if (!arg)
    {
      if (core_bfd)
	error (_("No core file specified.  (Use `detach' "
		 "to stop debugging a core file.)"));
      else
	error (_("No core file specified."));
    }

  gdb::unique_xmalloc_ptr<char> filename (tilde_expand (arg));
  if (strlen (filename.get ()) != 0
      && !IS_ABSOLUTE_PATH (filename.get ()))
    filename = make_unique_xstrdup (gdb_abspath (filename.get ()).c_str ());

  flags = O_BINARY | O_LARGEFILE;
  if (write_files)
    flags |= O_RDWR;
  else
    flags |= O_RDONLY;
  scratch_chan = gdb_open_cloexec (filename.get (), flags, 0).release ();
  if (scratch_chan < 0)
    perror_with_name (filename.get ());

  gdb_bfd_ref_ptr temp_bfd (gdb_bfd_fopen (filename.get (), gnutarget,
					   write_files ? FOPEN_RUB : FOPEN_RB,
					   scratch_chan));
  if (temp_bfd == NULL)
    perror_with_name (filename.get ());

  if (!bfd_check_format (temp_bfd.get (), bfd_core))
    {
      /* Do it after the err msg */
      /* FIXME: should be checking for errors from bfd_close (for one
	 thing, on error it does not free all the storage associated
	 with the bfd).  */
      error (_("\"%s\" is not a core dump: %s"),
	     filename.get (), bfd_errmsg (bfd_get_error ()));
    }

  current_program_space->cbfd = std::move (temp_bfd);

  core_target *target = new core_target ();

  /* Own the target until it is successfully pushed.  */
  target_ops_up target_holder (target);

  validate_files ();

  /* If we have no exec file, try to set the architecture from the
     core file.  We don't do this unconditionally since an exec file
     typically contains more information that helps us determine the
     architecture than a core file.  */
  if (!current_program_space->exec_bfd ())
    set_gdbarch_from_file (core_bfd);

  current_inferior ()->push_target (std::move (target_holder));

  switch_to_no_thread ();

  /* Need to flush the register cache (and the frame cache) from a
     previous debug session.  If inferior_ptid ends up the same as the
     last debug session --- e.g., b foo; run; gcore core1; step; gcore
     core2; core core1; core core2 --- then there's potential for
     get_current_regcache to return the cached regcache of the
     previous session, and the frame cache being stale.  */
  registers_changed ();

  /* Find (or fake) the pid for the process in this core file, and
     initialise the current inferior with that pid.  */
  bool fake_pid_p = false;
  int pid = bfd_core_file_pid (core_bfd);
  if (pid == 0)
    {
      fake_pid_p = true;
      pid = CORELOW_PID;
    }

  inferior *inf = current_inferior ();
  gdb_assert (inf->pid == 0);
  inferior_appeared (inf, pid);
  inf->fake_pid_p = fake_pid_p;

  /* Rename any .reg/0 sections, giving them each a fake lwpid.  */
  rename_vmcore_idle_reg_sections (core_bfd, inf);

  /* Build up thread list from BFD sections, and possibly set the
     current thread to the .reg/NN section matching the .reg
     section.  */
  asection *reg_sect = bfd_get_section_by_name (core_bfd, ".reg");
  for (asection *sect : gdb_bfd_sections (core_bfd))
    add_to_thread_list (sect, reg_sect, inf);

  if (inferior_ptid == null_ptid)
    {
      /* Either we found no .reg/NN section, and hence we have a
	 non-threaded core (single-threaded, from gdb's perspective),
	 or for some reason add_to_thread_list couldn't determine
	 which was the "main" thread.  The latter case shouldn't
	 usually happen, but we're dealing with input here, which can
	 always be broken in different ways.  */
      thread_info *thread = first_thread_of_inferior (inf);

      if (thread == NULL)
	thread = add_thread_silent (target, ptid_t (CORELOW_PID));

      switch_to_thread (thread);
    }

  if (current_program_space->exec_bfd () == nullptr)
    locate_exec_from_corefile_build_id (core_bfd, from_tty);

  post_create_inferior (from_tty);

  /* Now go through the target stack looking for threads since there
     may be a thread_stratum target loaded on top of target core by
     now.  The layer above should claim threads found in the BFD
     sections.  */
  try
    {
      target_update_thread_list ();
    }

  catch (const gdb_exception_error &except)
    {
      exception_print (gdb_stderr, except);
    }

  p = bfd_core_file_failing_command (core_bfd);
  if (p)
    gdb_printf (_("Core was generated by `%s'.\n"), p);

  /* Clearing any previous state of convenience variables.  */
  clear_exit_convenience_vars ();

  siggy = bfd_core_file_failing_signal (core_bfd);
  if (siggy > 0)
    {
      gdbarch *core_gdbarch = target->core_gdbarch ();

      /* If we don't have a CORE_GDBARCH to work with, assume a native
	 core (map gdb_signal from host signals).  If we do have
	 CORE_GDBARCH to work with, but no gdb_signal_from_target
	 implementation for that gdbarch, as a fallback measure,
	 assume the host signal mapping.  It'll be correct for native
	 cores, but most likely incorrect for cross-cores.  */
      enum gdb_signal sig = (core_gdbarch != NULL
			     && gdbarch_gdb_signal_from_target_p (core_gdbarch)
			     ? gdbarch_gdb_signal_from_target (core_gdbarch,
							       siggy)
			     : gdb_signal_from_host (siggy));

      gdb_printf (_("Program terminated with signal %s, %s"),
		  gdb_signal_to_name (sig), gdb_signal_to_string (sig));
      if (gdbarch_report_signal_info_p (core_gdbarch))
	gdbarch_report_signal_info (core_gdbarch, current_uiout, sig);
      gdb_printf (_(".\n"));

      /* Set the value of the internal variable $_exitsignal,
	 which holds the signal uncaught by the inferior.  */
      set_internalvar_integer (lookup_internalvar ("_exitsignal"),
			       siggy);
    }

  /* Fetch all registers from core file.  */
  target_fetch_registers (get_thread_regcache (inferior_thread ()), -1);

  /* Now, set up the frame cache, and print the top of stack.  */
  reinit_frame_cache ();
  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);

  /* Current thread should be NUM 1 but the user does not know that.
     If a program is single threaded gdb in general does not mention
     anything about threads.  That is why the test is >= 2.  */
  if (thread_count (target) >= 2)
    {
      try
	{
	  thread_command (NULL, from_tty);
	}
      catch (const gdb_exception_error &except)
	{
	  exception_print (gdb_stderr, except);
	}
    }
}

void
core_target::detach (inferior *inf, int from_tty)
{
  /* Get rid of the core.  Don't rely on core_target::close doing it,
     because target_detach may be called with core_target's refcount > 1,
     meaning core_target::close may not be called yet by the
     unpush_target call below.  */
  clear_core ();

  /* Note that 'this' may be dangling after this call.  unpush_target
     closes the target if the refcount reaches 0, and our close
     implementation deletes 'this'.  */
  inf->unpush_target (this);

  /* Clear the register cache and the frame cache.  */
  registers_changed ();
  reinit_frame_cache ();
  maybe_say_no_core_file_now (from_tty);
}

/* Try to retrieve registers from a section in core_bfd, and supply
   them to REGSET.

   If ptid's lwp member is zero, do the single-threaded
   thing: look for a section named NAME.  If ptid's lwp
   member is non-zero, do the multi-threaded thing: look for a section
   named "NAME/LWP", where LWP is the shortest ASCII decimal
   representation of ptid's lwp member.

   HUMAN_NAME is a human-readable name for the kind of registers the
   NAME section contains, for use in error messages.

   If REQUIRED is true, print an error if the core file doesn't have a
   section by the appropriate name.  Otherwise, just do nothing.  */

void
core_target::get_core_register_section (struct regcache *regcache,
					const struct regset *regset,
					const char *name,
					int section_min_size,
					const char *human_name,
					bool required)
{
  gdb_assert (regset != nullptr);

  struct bfd_section *section;
  bfd_size_type size;
  bool variable_size_section = (regset->flags & REGSET_VARIABLE_SIZE);

  thread_section_name section_name (name, regcache->ptid ());

  section = bfd_get_section_by_name (core_bfd, section_name.c_str ());
  if (! section)
    {
      if (required)
	warning (_("Couldn't find %s registers in core file."),
		 human_name);
      return;
    }

  size = bfd_section_size (section);
  if (size < section_min_size)
    {
      warning (_("Section `%s' in core file too small."),
	       section_name.c_str ());
      return;
    }
  if (size != section_min_size && !variable_size_section)
    {
      warning (_("Unexpected size of section `%s' in core file."),
	       section_name.c_str ());
    }

  gdb::byte_vector contents (size);
  if (!bfd_get_section_contents (core_bfd, section, contents.data (),
				 (file_ptr) 0, size))
    {
      warning (_("Couldn't read %s registers from `%s' section in core file."),
	       human_name, section_name.c_str ());
      return;
    }

  regset->supply_regset (regset, regcache, -1, contents.data (), size);
}

/* Data passed to gdbarch_iterate_over_regset_sections's callback.  */
struct get_core_registers_cb_data
{
  core_target *target;
  struct regcache *regcache;
};

/* Callback for get_core_registers that handles a single core file
   register note section. */

static void
get_core_registers_cb (const char *sect_name, int supply_size, int collect_size,
		       const struct regset *regset,
		       const char *human_name, void *cb_data)
{
  gdb_assert (regset != nullptr);

  auto *data = (get_core_registers_cb_data *) cb_data;
  bool required = false;
  bool variable_size_section = (regset->flags & REGSET_VARIABLE_SIZE);

  if (!variable_size_section)
    gdb_assert (supply_size == collect_size);

  if (strcmp (sect_name, ".reg") == 0)
    {
      required = true;
      if (human_name == NULL)
	human_name = "general-purpose";
    }
  else if (strcmp (sect_name, ".reg2") == 0)
    {
      if (human_name == NULL)
	human_name = "floating-point";
    }

  data->target->get_core_register_section (data->regcache, regset, sect_name,
					   supply_size, human_name, required);
}

/* Get the registers out of a core file.  This is the machine-
   independent part.  Fetch_core_registers is the machine-dependent
   part, typically implemented in the xm-file for each
   architecture.  */

/* We just get all the registers, so we don't use regno.  */

void
core_target::fetch_registers (struct regcache *regcache, int regno)
{
  if (!(m_core_gdbarch != nullptr
	&& gdbarch_iterate_over_regset_sections_p (m_core_gdbarch)))
    {
      gdb_printf (gdb_stderr,
		  "Can't fetch registers from this type of core file\n");
      return;
    }

  struct gdbarch *gdbarch = regcache->arch ();
  get_core_registers_cb_data data = { this, regcache };
  gdbarch_iterate_over_regset_sections (gdbarch,
					get_core_registers_cb,
					(void *) &data, NULL);

  /* Mark all registers not found in the core as unavailable.  */
  for (int i = 0; i < gdbarch_num_regs (regcache->arch ()); i++)
    if (regcache->get_register_status (i) == REG_UNKNOWN)
      regcache->raw_supply (i, NULL);
}

void
core_target::files_info ()
{
  print_section_info (&m_core_section_table, core_bfd);
}

/* Helper method for core_target::xfer_partial.  */

enum target_xfer_status
core_target::xfer_memory_via_mappings (gdb_byte *readbuf,
				       const gdb_byte *writebuf,
				       ULONGEST offset, ULONGEST len,
				       ULONGEST *xfered_len)
{
  enum target_xfer_status xfer_status;

  xfer_status = (section_table_xfer_memory_partial
		   (readbuf, writebuf,
		    offset, len, xfered_len,
		    m_core_file_mappings));

  if (xfer_status == TARGET_XFER_OK || m_core_unavailable_mappings.empty ())
    return xfer_status;

  /* There are instances - e.g. when debugging within a docker
     container using the AUFS storage driver - where the pathnames
     obtained from the note section are incorrect.  Despite the path
     being wrong, just knowing the start and end addresses of the
     mappings is still useful; we can attempt an access of the file
     stratum constrained to the address ranges corresponding to the
     unavailable mappings.  */

  ULONGEST memaddr = offset;
  ULONGEST memend = offset + len;

  for (const auto &mr : m_core_unavailable_mappings)
    {
      if (address_in_mem_range (memaddr, &mr))
	{
	  if (!address_in_mem_range (memend, &mr))
	    len = mr.start + mr.length - memaddr;

	  xfer_status = this->beneath ()->xfer_partial (TARGET_OBJECT_MEMORY,
							NULL,
							readbuf,
							writebuf,
							offset,
							len,
							xfered_len);
	  break;
	}
    }

  return xfer_status;
}

enum target_xfer_status
core_target::xfer_partial (enum target_object object, const char *annex,
			   gdb_byte *readbuf, const gdb_byte *writebuf,
			   ULONGEST offset, ULONGEST len, ULONGEST *xfered_len)
{
  switch (object)
    {
    case TARGET_OBJECT_MEMORY:
      {
	enum target_xfer_status xfer_status;

	/* Try accessing memory contents from core file data,
	   restricting consideration to those sections for which
	   the BFD section flag SEC_HAS_CONTENTS is set.  */
	auto has_contents_cb = [] (const struct target_section *s)
	  {
	    return ((s->the_bfd_section->flags & SEC_HAS_CONTENTS) != 0);
	  };
	xfer_status = section_table_xfer_memory_partial
			(readbuf, writebuf,
			 offset, len, xfered_len,
			 m_core_section_table,
			 has_contents_cb);
	if (xfer_status == TARGET_XFER_OK)
	  return TARGET_XFER_OK;

	/* Check file backed mappings.  If they're available, use
	   core file provided mappings (e.g. from .note.linuxcore.file
	   or the like) as this should provide a more accurate
	   result.  If not, check the stratum beneath us, which should
	   be the file stratum.

	   We also check unavailable mappings due to Docker/AUFS driver
	   issues.  */
	if (!m_core_file_mappings.empty ()
	    || !m_core_unavailable_mappings.empty ())
	  {
	    xfer_status = xfer_memory_via_mappings (readbuf, writebuf, offset,
						    len, xfered_len);
	  }
	else
	  xfer_status = this->beneath ()->xfer_partial (object, annex, readbuf,
							writebuf, offset, len,
							xfered_len);
	if (xfer_status == TARGET_XFER_OK)
	  return TARGET_XFER_OK;

	/* Finally, attempt to access data in core file sections with
	   no contents.  These will typically read as all zero.  */
	auto no_contents_cb = [&] (const struct target_section *s)
	  {
	    return !has_contents_cb (s);
	  };
	xfer_status = section_table_xfer_memory_partial
			(readbuf, writebuf,
			 offset, len, xfered_len,
			 m_core_section_table,
			 no_contents_cb);

	return xfer_status;
      }
    case TARGET_OBJECT_AUXV:
      if (readbuf)
	{
	  /* When the aux vector is stored in core file, BFD
	     represents this with a fake section called ".auxv".  */

	  struct bfd_section *section;
	  bfd_size_type size;

	  section = bfd_get_section_by_name (core_bfd, ".auxv");
	  if (section == NULL)
	    return TARGET_XFER_E_IO;

	  size = bfd_section_size (section);
	  if (offset >= size)
	    return TARGET_XFER_EOF;
	  size -= offset;
	  if (size > len)
	    size = len;

	  if (size == 0)
	    return TARGET_XFER_EOF;
	  if (!bfd_get_section_contents (core_bfd, section, readbuf,
					 (file_ptr) offset, size))
	    {
	      warning (_("Couldn't read NT_AUXV note in core file."));
	      return TARGET_XFER_E_IO;
	    }

	  *xfered_len = (ULONGEST) size;
	  return TARGET_XFER_OK;
	}
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_WCOOKIE:
      if (readbuf)
	{
	  /* When the StackGhost cookie is stored in core file, BFD
	     represents this with a fake section called
	     ".wcookie".  */

	  struct bfd_section *section;
	  bfd_size_type size;

	  section = bfd_get_section_by_name (core_bfd, ".wcookie");
	  if (section == NULL)
	    return TARGET_XFER_E_IO;

	  size = bfd_section_size (section);
	  if (offset >= size)
	    return TARGET_XFER_EOF;
	  size -= offset;
	  if (size > len)
	    size = len;

	  if (size == 0)
	    return TARGET_XFER_EOF;
	  if (!bfd_get_section_contents (core_bfd, section, readbuf,
					 (file_ptr) offset, size))
	    {
	      warning (_("Couldn't read StackGhost cookie in core file."));
	      return TARGET_XFER_E_IO;
	    }

	  *xfered_len = (ULONGEST) size;
	  return TARGET_XFER_OK;

	}
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_LIBRARIES:
      if (m_core_gdbarch != nullptr
	  && gdbarch_core_xfer_shared_libraries_p (m_core_gdbarch))
	{
	  if (writebuf)
	    return TARGET_XFER_E_IO;
	  else
	    {
	      *xfered_len = gdbarch_core_xfer_shared_libraries (m_core_gdbarch,
								readbuf,
								offset, len);

	      if (*xfered_len == 0)
		return TARGET_XFER_EOF;
	      else
		return TARGET_XFER_OK;
	    }
	}
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_LIBRARIES_AIX:
      if (m_core_gdbarch != nullptr
	  && gdbarch_core_xfer_shared_libraries_aix_p (m_core_gdbarch))
	{
	  if (writebuf)
	    return TARGET_XFER_E_IO;
	  else
	    {
	      *xfered_len
		= gdbarch_core_xfer_shared_libraries_aix (m_core_gdbarch,
							  readbuf, offset,
							  len);

	      if (*xfered_len == 0)
		return TARGET_XFER_EOF;
	      else
		return TARGET_XFER_OK;
	    }
	}
      return TARGET_XFER_E_IO;

    case TARGET_OBJECT_SIGNAL_INFO:
      if (readbuf)
	{
	  if (m_core_gdbarch != nullptr
	      && gdbarch_core_xfer_siginfo_p (m_core_gdbarch))
	    {
	      LONGEST l = gdbarch_core_xfer_siginfo  (m_core_gdbarch, readbuf,
						      offset, len);

	      if (l >= 0)
		{
		  *xfered_len = l;
		  if (l == 0)
		    return TARGET_XFER_EOF;
		  else
		    return TARGET_XFER_OK;
		}
	    }
	}
      return TARGET_XFER_E_IO;

    default:
      return this->beneath ()->xfer_partial (object, annex, readbuf,
					     writebuf, offset, len,
					     xfered_len);
    }
}



/* Okay, let's be honest: threads gleaned from a core file aren't
   exactly lively, are they?  On the other hand, if we don't claim
   that each & every one is alive, then we don't get any of them
   to appear in an "info thread" command, which is quite a useful
   behaviour.
 */
bool
core_target::thread_alive (ptid_t ptid)
{
  return true;
}

/* Ask the current architecture what it knows about this core file.
   That will be used, in turn, to pick a better architecture.  This
   wrapper could be avoided if targets got a chance to specialize
   core_target.  */

const struct target_desc *
core_target::read_description ()
{
  /* First check whether the target wants us to use the corefile target
     description notes.  */
  if (gdbarch_use_target_description_from_corefile_notes (m_core_gdbarch,
							  core_bfd))
    {
      /* If the core file contains a target description note then go ahead and
	 use that.  */
      bfd_size_type tdesc_note_size = 0;
      struct bfd_section *tdesc_note_section
	= bfd_get_section_by_name (core_bfd, ".gdb-tdesc");
      if (tdesc_note_section != nullptr)
	tdesc_note_size = bfd_section_size (tdesc_note_section);
      if (tdesc_note_size > 0)
	{
	  gdb::char_vector contents (tdesc_note_size + 1);
	  if (bfd_get_section_contents (core_bfd, tdesc_note_section,
					contents.data (), (file_ptr) 0,
					tdesc_note_size))
	    {
	      /* Ensure we have a null terminator.  */
	      contents[tdesc_note_size] = '\0';
	      const struct target_desc *result
		= string_read_description_xml (contents.data ());
	      if (result != nullptr)
		return result;
	    }
	}
    }

  /* If the architecture provides a corefile target description hook, use
     it now.  Even if the core file contains a target description in a note
     section, it is not useful for targets that can potentially have distinct
     descriptions for each thread.  One example is AArch64's SVE/SME
     extensions that allow per-thread vector length changes, resulting in
     registers with different sizes.  */
  if (m_core_gdbarch && gdbarch_core_read_description_p (m_core_gdbarch))
    {
      const struct target_desc *result;

      result = gdbarch_core_read_description (m_core_gdbarch, this, core_bfd);
      if (result != nullptr)
	return result;
    }

  return this->beneath ()->read_description ();
}

std::string
core_target::pid_to_str (ptid_t ptid)
{
  struct inferior *inf;
  int pid;

  /* The preferred way is to have a gdbarch/OS specific
     implementation.  */
  if (m_core_gdbarch != nullptr
      && gdbarch_core_pid_to_str_p (m_core_gdbarch))
    return gdbarch_core_pid_to_str (m_core_gdbarch, ptid);

  /* Otherwise, if we don't have one, we'll just fallback to
     "process", with normal_pid_to_str.  */

  /* Try the LWPID field first.  */
  pid = ptid.lwp ();
  if (pid != 0)
    return normal_pid_to_str (ptid_t (pid));

  /* Otherwise, this isn't a "threaded" core -- use the PID field, but
     only if it isn't a fake PID.  */
  inf = find_inferior_ptid (this, ptid);
  if (inf != NULL && !inf->fake_pid_p)
    return normal_pid_to_str (ptid);

  /* No luck.  We simply don't have a valid PID to print.  */
  return "<main task>";
}

const char *
core_target::thread_name (struct thread_info *thr)
{
  if (m_core_gdbarch != nullptr
      && gdbarch_core_thread_name_p (m_core_gdbarch))
    return gdbarch_core_thread_name (m_core_gdbarch, thr);
  return NULL;
}

bool
core_target::has_memory ()
{
  return (core_bfd != NULL);
}

bool
core_target::has_stack ()
{
  return (core_bfd != NULL);
}

bool
core_target::has_registers ()
{
  return (core_bfd != NULL);
}

/* Implement the to_info_proc method.  */

bool
core_target::info_proc (const char *args, enum info_proc_what request)
{
  struct gdbarch *gdbarch = get_current_arch ();

  /* Since this is the core file target, call the 'core_info_proc'
     method on gdbarch, not 'info_proc'.  */
  if (gdbarch_core_info_proc_p (gdbarch))
    gdbarch_core_info_proc (gdbarch, args, request);

  return true;
}

/* Implementation of the "supports_memory_tagging" target_ops method.  */

bool
core_target::supports_memory_tagging ()
{
  /* Look for memory tag sections.  If they exist, that means this core file
     supports memory tagging.  */

  return (bfd_get_section_by_name (core_bfd, "memtag") != nullptr);
}

/* Implementation of the "fetch_memtags" target_ops method.  */

bool
core_target::fetch_memtags (CORE_ADDR address, size_t len,
			    gdb::byte_vector &tags, int type)
{
  gdbarch *gdbarch = current_inferior ()->arch ();

  /* Make sure we have a way to decode the memory tag notes.  */
  if (!gdbarch_decode_memtag_section_p (gdbarch))
    error (_("gdbarch_decode_memtag_section not implemented for this "
	     "architecture."));

  memtag_section_info info;
  info.memtag_section = nullptr;

  while (get_next_core_memtag_section (core_bfd, info.memtag_section,
				       address, info))
  {
    size_t adjusted_length
      = (address + len < info.end_address) ? len : (info.end_address - address);

    /* Decode the memory tag note and return the tags.  */
    gdb::byte_vector tags_read
      = gdbarch_decode_memtag_section (gdbarch, info.memtag_section, type,
				       address, adjusted_length);

    /* Transfer over the tags that have been read.  */
    tags.insert (tags.end (), tags_read.begin (), tags_read.end ());

    /* ADDRESS + LEN may cross the boundaries of a particular memory tag
       segment.  Check if we need to fetch tags from a different section.  */
    if (!tags_read.empty () && (address + len) < info.end_address)
      return true;

    /* There are more tags to fetch.  Update ADDRESS and LEN.  */
    len -= (info.end_address - address);
    address = info.end_address;
  }

  return false;
}

/* Implementation of the "fetch_x86_xsave_layout" target_ops method.  */

x86_xsave_layout
core_target::fetch_x86_xsave_layout ()
{
  if (m_core_gdbarch != nullptr &&
      gdbarch_core_read_x86_xsave_layout_p (m_core_gdbarch))
    {
      x86_xsave_layout layout;
      if (!gdbarch_core_read_x86_xsave_layout (m_core_gdbarch, layout))
	return {};

      return layout;
    }

  return {};
}

/* Get a pointer to the current core target.  If not connected to a
   core target, return NULL.  */

static core_target *
get_current_core_target ()
{
  target_ops *proc_target = current_inferior ()->process_target ();
  return dynamic_cast<core_target *> (proc_target);
}

/* Display file backed mappings from core file.  */

void
core_target::info_proc_mappings (struct gdbarch *gdbarch)
{
  if (!m_core_file_mappings.empty ())
    {
      gdb_printf (_("Mapped address spaces:\n\n"));
      if (gdbarch_addr_bit (gdbarch) == 32)
	{
	  gdb_printf ("\t%10s %10s %10s %10s %s\n",
		      "Start Addr",
		      "  End Addr",
		      "      Size", "    Offset", "objfile");
	}
      else
	{
	  gdb_printf ("  %18s %18s %10s %10s %s\n",
		      "Start Addr",
		      "  End Addr",
		      "      Size", "    Offset", "objfile");
	}
    }

  for (const target_section &tsp : m_core_file_mappings)
    {
      ULONGEST start = tsp.addr;
      ULONGEST end = tsp.endaddr;
      ULONGEST file_ofs = tsp.the_bfd_section->filepos;
      const char *filename = bfd_get_filename (tsp.the_bfd_section->owner);

      if (gdbarch_addr_bit (gdbarch) == 32)
	gdb_printf ("\t%10s %10s %10s %10s %s\n",
		    paddress (gdbarch, start),
		    paddress (gdbarch, end),
		    hex_string (end - start),
		    hex_string (file_ofs),
		    filename);
      else
	gdb_printf ("  %18s %18s %10s %10s %s\n",
		    paddress (gdbarch, start),
		    paddress (gdbarch, end),
		    hex_string (end - start),
		    hex_string (file_ofs),
		    filename);
    }
}

/* Implement "maintenance print core-file-backed-mappings" command.  

   If mappings are loaded, the results should be similar to the
   mappings shown by "info proc mappings".  This command is mainly a
   debugging tool for GDB developers to make sure that the expected
   mappings are present after loading a core file.  For Linux, the
   output provided by this command will be very similar (if not
   identical) to that provided by "info proc mappings".  This is not
   necessarily the case for other OSes which might provide
   more/different information in the "info proc mappings" output.  */

static void
maintenance_print_core_file_backed_mappings (const char *args, int from_tty)
{
  core_target *targ = get_current_core_target ();
  if (targ != nullptr)
    targ->info_proc_mappings (targ->core_gdbarch ());
}

void _initialize_corelow ();
void
_initialize_corelow ()
{
  add_target (core_target_info, core_target_open, filename_completer);
  add_cmd ("core-file-backed-mappings", class_maintenance,
	   maintenance_print_core_file_backed_mappings,
	   _("Print core file's file-backed mappings."),
	   &maintenanceprintlist);
}
