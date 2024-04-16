/* Definitions for BFD wrappers used by GDB.

   Copyright (C) 2011-2024 Free Software Foundation, Inc.

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
#include "gdb_bfd.h"
#include "ui-out.h"
#include "gdbcmd.h"
#include "hashtab.h"
#include "gdbsupport/filestuff.h"
#ifdef HAVE_MMAP
#include <sys/mman.h>
#ifndef MAP_FAILED
#define MAP_FAILED ((void *) -1)
#endif
#endif
#include "target.h"
#include "gdbsupport/fileio.h"
#include "inferior.h"
#include "cli/cli-style.h"
#include <unordered_map>

#if CXX_STD_THREAD

#include <mutex>

/* Lock held when doing BFD operations.  A recursive mutex is used
   because we use this mutex internally and also for BFD, just to make
   life a bit simpler, and we may sometimes hold it while calling into
   BFD.  */
static std::recursive_mutex gdb_bfd_mutex;

/* BFD locking function.  */

static bool
gdb_bfd_lock (void *ignore)
{
  gdb_bfd_mutex.lock ();
  return true;
}

/* BFD unlocking function.  */

static bool
gdb_bfd_unlock (void *ignore)
{
  gdb_bfd_mutex.unlock ();
  return true;
}

#endif /* CXX_STD_THREAD */

/* An object of this type is stored in the section's user data when
   mapping a section.  */

struct gdb_bfd_section_data
{
  /* Size of the data.  */
  bfd_size_type size;
  /* If the data was mmapped, this is the length of the map.  */
  bfd_size_type map_len;
  /* The data.  If NULL, the section data has not been read.  */
  void *data;
  /* If the data was mmapped, this is the map address.  */
  void *map_addr;
};

/* A hash table holding every BFD that gdb knows about.  This is not
   to be confused with 'gdb_bfd_cache', which is used for sharing
   BFDs; in contrast, this hash is used just to implement
   "maint info bfd".  */

static htab_t all_bfds;

/* An object of this type is stored in each BFD's user data.  */

struct gdb_bfd_data
{
  /* Note that if ST is nullptr, then we simply fill in zeroes.  */
  gdb_bfd_data (bfd *abfd, struct stat *st)
    : mtime (st == nullptr ? 0 : st->st_mtime),
      size (st == nullptr ? 0 : st->st_size),
      inode (st == nullptr ? 0 : st->st_ino),
      device_id (st == nullptr ? 0 : st->st_dev),
      relocation_computed (0),
      needs_relocations (0),
      crc_computed (0)
  {
  }

  ~gdb_bfd_data ()
  {
  }

  /* The reference count.  */
  int refc = 1;

  /* The mtime of the BFD at the point the cache entry was made.  */
  time_t mtime;

  /* The file size (in bytes) at the point the cache entry was made.  */
  off_t size;

  /* The inode of the file at the point the cache entry was made.  */
  ino_t inode;

  /* The device id of the file at the point the cache entry was made.  */
  dev_t device_id;

  /* This is true if we have determined whether this BFD has any
     sections requiring relocation.  */
  unsigned int relocation_computed : 1;

  /* This is true if any section needs relocation.  */
  unsigned int needs_relocations : 1;

  /* This is true if we have successfully computed the file's CRC.  */
  unsigned int crc_computed : 1;

  /* The file's CRC.  */
  unsigned long crc = 0;

  /* If the BFD comes from an archive, this points to the archive's
     BFD.  Otherwise, this is NULL.  */
  bfd *archive_bfd = nullptr;

  /* Table of all the bfds this bfd has included.  */
  std::vector<gdb_bfd_ref_ptr> included_bfds;

  /* The registry.  */
  registry<bfd> registry_fields;
};

registry<bfd> *
registry_accessor<bfd>::get (bfd *abfd)
{
  struct gdb_bfd_data *gdata = (struct gdb_bfd_data *) bfd_usrdata (abfd);
  return &gdata->registry_fields;
}

/* A hash table storing all the BFDs maintained in the cache.  */

static htab_t gdb_bfd_cache;

/* When true gdb will reuse an existing bfd object if the filename,
   modification time, and file size all match.  */

static bool bfd_sharing = true;
static void
show_bfd_sharing  (struct ui_file *file, int from_tty,
		   struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("BFD sharing is %s.\n"), value);
}

/* When true debugging of the bfd caches is enabled.  */

static bool debug_bfd_cache;

/* Print an "bfd-cache" debug statement.  */

#define bfd_cache_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (debug_bfd_cache, "bfd-cache", fmt, ##__VA_ARGS__)

static void
show_bfd_cache_debug (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  gdb_printf (file, _("BFD cache debugging is %s.\n"), value);
}

/* The type of an object being looked up in gdb_bfd_cache.  We use
   htab's capability of storing one kind of object (BFD in this case)
   and using a different sort of object for searching.  */

struct gdb_bfd_cache_search
{
  /* The filename.  */
  const char *filename;
  /* The mtime.  */
  time_t mtime;
  /* The file size (in bytes).  */
  off_t size;
  /* The inode of the file.  */
  ino_t inode;
  /* The device id of the file.  */
  dev_t device_id;
};

/* A hash function for BFDs.  */

static hashval_t
hash_bfd (const void *b)
{
  const bfd *abfd = (const struct bfd *) b;

  /* It is simplest to just hash the filename.  */
  return htab_hash_string (bfd_get_filename (abfd));
}

/* An equality function for BFDs.  Note that this expects the caller
   to search using struct gdb_bfd_cache_search only, not BFDs.  */

static int
eq_bfd (const void *a, const void *b)
{
  const bfd *abfd = (const struct bfd *) a;
  const struct gdb_bfd_cache_search *s
    = (const struct gdb_bfd_cache_search *) b;
  struct gdb_bfd_data *gdata = (struct gdb_bfd_data *) bfd_usrdata (abfd);

  return (gdata->mtime == s->mtime
	  && gdata->size == s->size
	  && gdata->inode == s->inode
	  && gdata->device_id == s->device_id
	  && strcmp (bfd_get_filename (abfd), s->filename) == 0);
}

/* See gdb_bfd.h.  */

int
is_target_filename (const char *name)
{
  return startswith (name, TARGET_SYSROOT_PREFIX);
}

/* See gdb_bfd.h.  */

int
gdb_bfd_has_target_filename (struct bfd *abfd)
{
  return is_target_filename (bfd_get_filename (abfd));
}

/* For `gdb_bfd_open_from_target_memory`.  An object that manages the
   details of a BFD in target memory.  */

struct target_buffer : public gdb_bfd_iovec_base
{
  /* Constructor.  BASE and SIZE define where the BFD can be found in
     target memory.  */
  target_buffer (CORE_ADDR base, ULONGEST size)
    : m_base (base),
      m_size (size),
      m_filename (xstrprintf ("<in-memory@%s-%s>",
			      core_addr_to_string_nz (m_base),
			      core_addr_to_string_nz (m_base + m_size)))
  {
  }

  /* Return the size of the in-memory BFD file.  */
  ULONGEST size () const
  { return m_size; }

  /* Return the base address of the in-memory BFD file.  */
  CORE_ADDR base () const
  { return m_base; }

  /* Return a generated filename for the in-memory BFD file.  The generated
     name will include the begin and end address of the in-memory file.  */
  const char *filename () const
  { return m_filename.get (); }

  file_ptr read (bfd *abfd, void *buffer, file_ptr nbytes,
		 file_ptr offset) override;

  int stat (struct bfd *abfd, struct stat *sb) override;

private:
  /* The base address of the in-memory BFD file.  */
  CORE_ADDR m_base;

  /* The size (in-bytes) of the in-memory BFD file.  */
  ULONGEST m_size;

  /* Holds the generated name of the in-memory BFD file.  */
  gdb::unique_xmalloc_ptr<char> m_filename;
};

/* For `gdb_bfd_open_from_target_memory`.  For reading the file, we just need to
   pass through to target_read_memory and fix up the arguments and return
   values.  */

file_ptr
target_buffer::read (struct bfd *abfd, void *buf,
		     file_ptr nbytes, file_ptr offset)
{
  /* If this read will read all of the file, limit it to just the rest.  */
  if (offset + nbytes > size ())
    nbytes = size () - offset;

  /* If there are no more bytes left, we've reached EOF.  */
  if (nbytes == 0)
    return 0;

  int err = target_read_memory (base () + offset, (gdb_byte *) buf, nbytes);
  if (err)
    return -1;

  return nbytes;
}

/* For `gdb_bfd_open_from_target_memory`.  For statting the file, we only
   support the st_size attribute.  */

int
target_buffer::stat (struct bfd *abfd, struct stat *sb)
{
  memset (sb, 0, sizeof (struct stat));
  sb->st_size = size ();
  return 0;
}

/* See gdb_bfd.h.  */

gdb_bfd_ref_ptr
gdb_bfd_open_from_target_memory (CORE_ADDR addr, ULONGEST size,
				 const char *target)
{
  std::unique_ptr<target_buffer> buffer
    = std::make_unique<target_buffer> (addr, size);

  return gdb_bfd_openr_iovec (buffer->filename (), target,
			      [&] (bfd *nbfd)
			      {
				return buffer.release ();
			      });
}

/* An object that manages the underlying stream for a BFD, using
   target file I/O.  */

struct target_fileio_stream : public gdb_bfd_iovec_base
{
  target_fileio_stream (bfd *nbfd, int fd)
    : m_bfd (nbfd),
      m_fd (fd)
  {
  }

  ~target_fileio_stream ();

  file_ptr read (bfd *abfd, void *buffer, file_ptr nbytes,
		 file_ptr offset) override;

  int stat (struct bfd *abfd, struct stat *sb) override;

private:

  /* The BFD.  Saved for the destructor.  */
  bfd *m_bfd;

  /* The file descriptor.  */
  int m_fd;
};

/* Wrapper for target_fileio_open suitable for use as a helper
   function for gdb_bfd_openr_iovec.  */

static target_fileio_stream *
gdb_bfd_iovec_fileio_open (struct bfd *abfd, inferior *inf, bool warn_if_slow)
{
  const char *filename = bfd_get_filename (abfd);
  int fd;
  fileio_error target_errno;

  gdb_assert (is_target_filename (filename));

  fd = target_fileio_open (inf,
			   filename + strlen (TARGET_SYSROOT_PREFIX),
			   FILEIO_O_RDONLY, 0, warn_if_slow,
			   &target_errno);
  if (fd == -1)
    {
      errno = fileio_error_to_host (target_errno);
      bfd_set_error (bfd_error_system_call);
      return NULL;
    }

  return new target_fileio_stream (abfd, fd);
}

/* Wrapper for target_fileio_pread.  */

file_ptr
target_fileio_stream::read (struct bfd *abfd, void *buf,
			    file_ptr nbytes, file_ptr offset)
{
  fileio_error target_errno;
  file_ptr pos, bytes;

  pos = 0;
  while (nbytes > pos)
    {
      QUIT;

      bytes = target_fileio_pread (m_fd, (gdb_byte *) buf + pos,
				   nbytes - pos, offset + pos,
				   &target_errno);
      if (bytes == 0)
	/* Success, but no bytes, means end-of-file.  */
	break;
      if (bytes == -1)
	{
	  errno = fileio_error_to_host (target_errno);
	  bfd_set_error (bfd_error_system_call);
	  return -1;
	}

      pos += bytes;
    }

  return pos;
}

/* Warn that it wasn't possible to close a bfd for file NAME, because
   of REASON.  */

static void
gdb_bfd_close_warning (const char *name, const char *reason)
{
  warning (_("cannot close \"%s\": %s"), name, reason);
}

/* Wrapper for target_fileio_close.  */

target_fileio_stream::~target_fileio_stream ()
{
  fileio_error target_errno;

  /* Ignore errors on close.  These may happen with remote
     targets if the connection has already been torn down.  */
  try
    {
      target_fileio_close (m_fd, &target_errno);
    }
  catch (const gdb_exception &ex)
    {
      /* Also avoid crossing exceptions over bfd.  */
      gdb_bfd_close_warning (bfd_get_filename (m_bfd),
			     ex.message->c_str ());
    }
}

/* Wrapper for target_fileio_fstat.  */

int
target_fileio_stream::stat (struct bfd *abfd, struct stat *sb)
{
  fileio_error target_errno;
  int result;

  result = target_fileio_fstat (m_fd, sb, &target_errno);
  if (result == -1)
    {
      errno = fileio_error_to_host (target_errno);
      bfd_set_error (bfd_error_system_call);
    }

  return result;
}

/* A helper function to initialize the data that gdb attaches to each
   BFD.  */

static void
gdb_bfd_init_data (struct bfd *abfd, struct stat *st)
{
  struct gdb_bfd_data *gdata;
  void **slot;

  gdb_assert (bfd_usrdata (abfd) == nullptr);

  /* Ask BFD to decompress sections in bfd_get_full_section_contents.  */
  abfd->flags |= BFD_DECOMPRESS;

  gdata = new gdb_bfd_data (abfd, st);
  bfd_set_usrdata (abfd, gdata);

  /* This is the first we've seen it, so add it to the hash table.  */
  slot = htab_find_slot (all_bfds, abfd, INSERT);
  gdb_assert (slot && !*slot);
  *slot = abfd;
}

/* See gdb_bfd.h.  */

gdb_bfd_ref_ptr
gdb_bfd_open (const char *name, const char *target, int fd,
	      bool warn_if_slow)
{
  hashval_t hash;
  void **slot;
  bfd *abfd;
  struct gdb_bfd_cache_search search;
  struct stat st;

  if (is_target_filename (name))
    {
      if (!target_filesystem_is_local ())
	{
	  gdb_assert (fd == -1);

	  auto open = [&] (bfd *nbfd) -> gdb_bfd_iovec_base *
	  {
	    return gdb_bfd_iovec_fileio_open (nbfd, current_inferior (),
					      warn_if_slow);
	  };

	  return gdb_bfd_openr_iovec (name, target, open);
	}

      name += strlen (TARGET_SYSROOT_PREFIX);
    }

#if CXX_STD_THREAD
  std::lock_guard<std::recursive_mutex> guard (gdb_bfd_mutex);
#endif

  if (gdb_bfd_cache == NULL)
    gdb_bfd_cache = htab_create_alloc (1, hash_bfd, eq_bfd, NULL,
				       xcalloc, xfree);

  if (fd == -1)
    {
      fd = gdb_open_cloexec (name, O_RDONLY | O_BINARY, 0).release ();
      if (fd == -1)
	{
	  bfd_set_error (bfd_error_system_call);
	  return NULL;
	}
    }

  if (fstat (fd, &st) < 0)
    {
      /* Weird situation here -- don't cache if we can't stat.  */
      bfd_cache_debug_printf ("Could not stat %s - not caching", name);
      abfd = bfd_fopen (name, target, FOPEN_RB, fd);
      if (abfd == nullptr)
	return nullptr;
      return gdb_bfd_ref_ptr::new_reference (abfd);
    }

  search.filename = name;
  search.mtime = st.st_mtime;
  search.size = st.st_size;
  search.inode = st.st_ino;
  search.device_id = st.st_dev;

  /* Note that this must compute the same result as hash_bfd.  */
  hash = htab_hash_string (name);
  /* Note that we cannot use htab_find_slot_with_hash here, because
     opening the BFD may fail; and this would violate hashtab
     invariants.  */
  abfd = (struct bfd *) htab_find_with_hash (gdb_bfd_cache, &search, hash);
  if (bfd_sharing && abfd != NULL)
    {
      bfd_cache_debug_printf ("Reusing cached bfd %s for %s",
			      host_address_to_string (abfd),
			      bfd_get_filename (abfd));
      close (fd);
      return gdb_bfd_ref_ptr::new_reference (abfd);
    }

  abfd = bfd_fopen (name, target, FOPEN_RB, fd);
  if (abfd == NULL)
    return NULL;

  bfd_set_cacheable (abfd, 1);

  bfd_cache_debug_printf ("Creating new bfd %s for %s",
			  host_address_to_string (abfd),
			  bfd_get_filename (abfd));

  if (bfd_sharing)
    {
      slot = htab_find_slot_with_hash (gdb_bfd_cache, &search, hash, INSERT);
      gdb_assert (!*slot);
      *slot = abfd;
    }

  /* It's important to pass the already-computed stat info here,
     rather than, say, calling gdb_bfd_ref_ptr::new_reference.  BFD by
     default will "stat" the file each time bfd_get_mtime is called --
     and since we already entered it into the hash table using this
     mtime, if the file changed at the wrong moment, the race would
     lead to a hash table corruption.  */
  gdb_bfd_init_data (abfd, &st);
  return gdb_bfd_ref_ptr (abfd);
}

/* A helper function that releases any section data attached to the
   BFD.  */

static void
free_one_bfd_section (asection *sectp)
{
  struct gdb_bfd_section_data *sect
    = (struct gdb_bfd_section_data *) bfd_section_userdata (sectp);

  if (sect != NULL && sect->data != NULL)
    {
#ifdef HAVE_MMAP
      if (sect->map_addr != NULL)
	{
	  int res;

	  res = munmap (sect->map_addr, sect->map_len);
	  gdb_assert (res == 0);
	}
      else
#endif
	xfree (sect->data);
    }
}

/* Close ABFD, and warn if that fails.  */

static int
gdb_bfd_close_or_warn (struct bfd *abfd)
{
  int ret;
  const char *name = bfd_get_filename (abfd);

  for (asection *sect : gdb_bfd_sections (abfd))
    free_one_bfd_section (sect);

  ret = bfd_close (abfd);

  if (!ret)
    gdb_bfd_close_warning (name,
			   bfd_errmsg (bfd_get_error ()));

  return ret;
}

/* See gdb_bfd.h.  */

void
gdb_bfd_ref (struct bfd *abfd)
{
  struct gdb_bfd_data *gdata;

  if (abfd == NULL)
    return;

#if CXX_STD_THREAD
  std::lock_guard<std::recursive_mutex> guard (gdb_bfd_mutex);
#endif

  gdata = (struct gdb_bfd_data *) bfd_usrdata (abfd);

  bfd_cache_debug_printf ("Increase reference count on bfd %s (%s)",
			  host_address_to_string (abfd),
			  bfd_get_filename (abfd));

  if (gdata != NULL)
    {
      gdata->refc += 1;
      return;
    }

  /* Caching only happens via gdb_bfd_open, so passing nullptr here is
     fine.  */
  gdb_bfd_init_data (abfd, nullptr);
}

/* See gdb_bfd.h.  */

void
gdb_bfd_unref (struct bfd *abfd)
{
  struct gdb_bfd_data *gdata;
  struct gdb_bfd_cache_search search;
  bfd *archive_bfd;

  if (abfd == NULL)
    return;

#if CXX_STD_THREAD
  std::lock_guard<std::recursive_mutex> guard (gdb_bfd_mutex);
#endif

  gdata = (struct gdb_bfd_data *) bfd_usrdata (abfd);
  gdb_assert (gdata->refc >= 1);

  gdata->refc -= 1;
  if (gdata->refc > 0)
    {
      bfd_cache_debug_printf ("Decrease reference count on bfd %s (%s)",
			      host_address_to_string (abfd),
			      bfd_get_filename (abfd));
      return;
    }

  bfd_cache_debug_printf ("Delete final reference count on bfd %s (%s)",
			  host_address_to_string (abfd),
			  bfd_get_filename (abfd));

  archive_bfd = gdata->archive_bfd;
  search.filename = bfd_get_filename (abfd);

  if (gdb_bfd_cache && search.filename)
    {
      hashval_t hash = htab_hash_string (search.filename);
      void **slot;

      search.mtime = gdata->mtime;
      search.size = gdata->size;
      search.inode = gdata->inode;
      search.device_id = gdata->device_id;
      slot = htab_find_slot_with_hash (gdb_bfd_cache, &search, hash,
				       NO_INSERT);

      if (slot && *slot)
	htab_clear_slot (gdb_bfd_cache, slot);
    }

  delete gdata;
  bfd_set_usrdata (abfd, NULL);  /* Paranoia.  */

  htab_remove_elt (all_bfds, abfd);

  gdb_bfd_close_or_warn (abfd);

  gdb_bfd_unref (archive_bfd);
}

/* A helper function that returns the section data descriptor
   associated with SECTION.  If no such descriptor exists, a new one
   is allocated and cleared.  */

static struct gdb_bfd_section_data *
get_section_descriptor (asection *section)
{
  struct gdb_bfd_section_data *result;

  result = (struct gdb_bfd_section_data *) bfd_section_userdata (section);

  if (result == NULL)
    {
      result = ((struct gdb_bfd_section_data *)
		bfd_zalloc (section->owner, sizeof (*result)));
      bfd_set_section_userdata (section, result);
    }

  return result;
}

/* See gdb_bfd.h.  */

const gdb_byte *
gdb_bfd_map_section (asection *sectp, bfd_size_type *size)
{
  bfd *abfd;
  struct gdb_bfd_section_data *descriptor;
  bfd_byte *data;

  gdb_assert ((sectp->flags & SEC_RELOC) == 0);
  gdb_assert (size != NULL);

  abfd = sectp->owner;

  descriptor = get_section_descriptor (sectp);

  /* If the data was already read for this BFD, just reuse it.  */
  if (descriptor->data != NULL)
    goto done;

#ifdef HAVE_MMAP
  if (!bfd_is_section_compressed (abfd, sectp))
    {
      /* The page size, used when mmapping.  */
      static int pagesize;

      if (pagesize == 0)
	pagesize = getpagesize ();

      /* Only try to mmap sections which are large enough: we don't want
	 to waste space due to fragmentation.  */

      if (bfd_section_size (sectp) > 4 * pagesize)
	{
	  descriptor->size = bfd_section_size (sectp);
	  descriptor->data = bfd_mmap (abfd, 0, descriptor->size, PROT_READ,
				       MAP_PRIVATE, sectp->filepos,
				       &descriptor->map_addr,
				       &descriptor->map_len);

	  if ((caddr_t)descriptor->data != MAP_FAILED)
	    {
#if HAVE_POSIX_MADVISE
	      posix_madvise (descriptor->map_addr, descriptor->map_len,
			     POSIX_MADV_WILLNEED);
#endif
	      goto done;
	    }

	  /* On failure, clear out the section data and try again.  */
	  memset (descriptor, 0, sizeof (*descriptor));
	}
    }
#endif /* HAVE_MMAP */

  /* Handle compressed sections, or ordinary uncompressed sections in
     the no-mmap case.  */

  descriptor->size = bfd_section_size (sectp);
  descriptor->data = NULL;

  data = NULL;
  if (!bfd_get_full_section_contents (abfd, sectp, &data))
    {
      warning (_("Can't read data for section '%s' in file '%s'"),
	       bfd_section_name (sectp),
	       bfd_get_filename (abfd));
      /* Set size to 0 to prevent further attempts to read the invalid
	 section.  */
      *size = 0;
      return NULL;
    }
  descriptor->data = data;

 done:
  gdb_assert (descriptor->data != NULL);
  *size = descriptor->size;
  return (const gdb_byte *) descriptor->data;
}

/* Return 32-bit CRC for ABFD.  If successful store it to *FILE_CRC_RETURN and
   return 1.  Otherwise print a warning and return 0.  ABFD seek position is
   not preserved.  */

static int
get_file_crc (bfd *abfd, unsigned long *file_crc_return)
{
  uint32_t file_crc = 0;

  if (bfd_seek (abfd, 0, SEEK_SET) != 0)
    {
      warning (_("Problem reading \"%s\" for CRC: %s"),
	       bfd_get_filename (abfd), bfd_errmsg (bfd_get_error ()));
      return 0;
    }

  for (;;)
    {
      gdb_byte buffer[8 * 1024];
      bfd_size_type count;

      count = bfd_read (buffer, sizeof (buffer), abfd);
      if (count == (bfd_size_type) -1)
	{
	  warning (_("Problem reading \"%s\" for CRC: %s"),
		   bfd_get_filename (abfd), bfd_errmsg (bfd_get_error ()));
	  return 0;
	}
      if (count == 0)
	break;
      file_crc = bfd_calc_gnu_debuglink_crc32 (file_crc, buffer, count);
    }

  *file_crc_return = file_crc;
  return 1;
}

/* See gdb_bfd.h.  */

int
gdb_bfd_crc (struct bfd *abfd, unsigned long *crc_out)
{
  struct gdb_bfd_data *gdata = (struct gdb_bfd_data *) bfd_usrdata (abfd);

  if (!gdata->crc_computed)
    gdata->crc_computed = get_file_crc (abfd, &gdata->crc);

  if (gdata->crc_computed)
    *crc_out = gdata->crc;
  return gdata->crc_computed;
}



/* See gdb_bfd.h.  */

gdb_bfd_ref_ptr
gdb_bfd_fopen (const char *filename, const char *target, const char *mode,
	       int fd)
{
  bfd *result = bfd_fopen (filename, target, mode, fd);

  if (result != nullptr)
    bfd_set_cacheable (result, 1);

  return gdb_bfd_ref_ptr::new_reference (result);
}

/* See gdb_bfd.h.  */

gdb_bfd_ref_ptr
gdb_bfd_openr (const char *filename, const char *target)
{
  bfd *result = bfd_openr (filename, target);

  return gdb_bfd_ref_ptr::new_reference (result);
}

/* See gdb_bfd.h.  */

gdb_bfd_ref_ptr
gdb_bfd_openw (const char *filename, const char *target)
{
  bfd *result = bfd_openw (filename, target);

  return gdb_bfd_ref_ptr::new_reference (result);
}

/* See gdb_bfd.h.  */

gdb_bfd_ref_ptr
gdb_bfd_openr_iovec (const char *filename, const char *target,
		     gdb_iovec_opener_ftype open_fn)
{
  auto do_open = [] (bfd *nbfd, void *closure) -> void *
  {
    auto real_opener = static_cast<gdb_iovec_opener_ftype *> (closure);
    return (*real_opener) (nbfd);
  };

  auto read_trampoline = [] (bfd *nbfd, void *stream, void *buf,
			     file_ptr nbytes, file_ptr offset) -> file_ptr
  {
    gdb_bfd_iovec_base *obj = static_cast<gdb_bfd_iovec_base *> (stream);
    return obj->read (nbfd, buf, nbytes, offset);
  };

  auto stat_trampoline = [] (struct bfd *abfd, void *stream,
			     struct stat *sb) -> int
  {
    gdb_bfd_iovec_base *obj = static_cast<gdb_bfd_iovec_base *> (stream);
    return obj->stat (abfd, sb);
  };

  auto close_trampoline = [] (struct bfd *nbfd, void *stream) -> int
  {
    gdb_bfd_iovec_base *obj = static_cast<gdb_bfd_iovec_base *> (stream);
    delete obj;
    /* Success.  */
    return 0;
  };

  bfd *result = bfd_openr_iovec (filename, target,
				 do_open, &open_fn,
				 read_trampoline, close_trampoline,
				 stat_trampoline);

  return gdb_bfd_ref_ptr::new_reference (result);
}

/* See gdb_bfd.h.  */

void
gdb_bfd_mark_parent (bfd *child, bfd *parent)
{
  struct gdb_bfd_data *gdata;

  gdb_bfd_ref (child);
  /* No need to stash the filename here, because we also keep a
     reference on the parent archive.  */

  gdata = (struct gdb_bfd_data *) bfd_usrdata (child);
  if (gdata->archive_bfd == NULL)
    {
      gdata->archive_bfd = parent;
      gdb_bfd_ref (parent);
    }
  else
    gdb_assert (gdata->archive_bfd == parent);
}

/* See gdb_bfd.h.  */

gdb_bfd_ref_ptr
gdb_bfd_openr_next_archived_file (bfd *archive, bfd *previous)
{
  bfd *result = bfd_openr_next_archived_file (archive, previous);

  if (result)
    gdb_bfd_mark_parent (result, archive);

  return gdb_bfd_ref_ptr (result);
}

/* See gdb_bfd.h.  */

void
gdb_bfd_record_inclusion (bfd *includer, bfd *includee)
{
  struct gdb_bfd_data *gdata;

  gdata = (struct gdb_bfd_data *) bfd_usrdata (includer);
  gdata->included_bfds.push_back (gdb_bfd_ref_ptr::new_reference (includee));
}



static_assert (ARRAY_SIZE (_bfd_std_section) == 4);

/* See gdb_bfd.h.  */

int
gdb_bfd_section_index (bfd *abfd, asection *section)
{
  if (section == NULL)
    return -1;
  else if (section == bfd_com_section_ptr)
    return bfd_count_sections (abfd);
  else if (section == bfd_und_section_ptr)
    return bfd_count_sections (abfd) + 1;
  else if (section == bfd_abs_section_ptr)
    return bfd_count_sections (abfd) + 2;
  else if (section == bfd_ind_section_ptr)
    return bfd_count_sections (abfd) + 3;
  return section->index;
}

/* See gdb_bfd.h.  */

int
gdb_bfd_count_sections (bfd *abfd)
{
  return bfd_count_sections (abfd) + 4;
}

/* See gdb_bfd.h.  */

int
gdb_bfd_requires_relocations (bfd *abfd)
{
  struct gdb_bfd_data *gdata = (struct gdb_bfd_data *) bfd_usrdata (abfd);

  if (gdata->relocation_computed == 0)
    {
      asection *sect;

      for (sect = abfd->sections; sect != NULL; sect = sect->next)
	if ((sect->flags & SEC_RELOC) != 0)
	  {
	    gdata->needs_relocations = 1;
	    break;
	  }

      gdata->relocation_computed = 1;
    }

  return gdata->needs_relocations;
}

/* See gdb_bfd.h.  */

bool
gdb_bfd_get_full_section_contents (bfd *abfd, asection *section,
				   gdb::byte_vector *contents)
{
  bfd_size_type section_size = bfd_section_size (section);

  contents->resize (section_size);

  return bfd_get_section_contents (abfd, section, contents->data (), 0,
				   section_size);
}

#define AMBIGUOUS_MESS1	".\nMatching formats:"
#define AMBIGUOUS_MESS2	\
  ".\nUse \"set gnutarget format-name\" to specify the format."

/* See gdb_bfd.h.  */

std::string
gdb_bfd_errmsg (bfd_error_type error_tag, char **matching)
{
  char **p;

  /* Check if errmsg just need simple return.  */
  if (error_tag != bfd_error_file_ambiguously_recognized || matching == NULL)
    return bfd_errmsg (error_tag);

  std::string ret (bfd_errmsg (error_tag));
  ret += AMBIGUOUS_MESS1;

  for (p = matching; *p; p++)
    {
      ret += " ";
      ret += *p;
    }
  ret += AMBIGUOUS_MESS2;

  xfree (matching);

  return ret;
}

/* A callback for htab_traverse that prints a single BFD.  */

static int
print_one_bfd (void **slot, void *data)
{
  bfd *abfd = (struct bfd *) *slot;
  struct gdb_bfd_data *gdata = (struct gdb_bfd_data *) bfd_usrdata (abfd);
  struct ui_out *uiout = (struct ui_out *) data;

  ui_out_emit_tuple tuple_emitter (uiout, NULL);
  uiout->field_signed ("refcount", gdata->refc);
  uiout->field_string ("addr", host_address_to_string (abfd));
  uiout->field_string ("filename", bfd_get_filename (abfd),
		       file_name_style.style ());
  uiout->text ("\n");

  return 1;
}

/* Implement the 'maint info bfd' command.  */

static void
maintenance_info_bfds (const char *arg, int from_tty)
{
  struct ui_out *uiout = current_uiout;

  ui_out_emit_table table_emitter (uiout, 3, -1, "bfds");
  uiout->table_header (10, ui_left, "refcount", "Refcount");
  uiout->table_header (18, ui_left, "addr", "Address");
  uiout->table_header (40, ui_left, "filename", "Filename");

  uiout->table_body ();
  htab_traverse (all_bfds, print_one_bfd, uiout);
}

/* BFD related per-inferior data.  */

struct bfd_inferior_data
{
  std::unordered_map<std::string, unsigned long> bfd_error_string_counts;
};

/* Per-inferior data key.  */

static const registry<inferior>::key<bfd_inferior_data> bfd_inferior_data_key;

/* Fetch per-inferior BFD data.  It always returns a valid pointer to
   a bfd_inferior_data struct.  */

static struct bfd_inferior_data *
get_bfd_inferior_data (struct inferior *inf)
{
  struct bfd_inferior_data *data;

  data = bfd_inferior_data_key.get (inf);
  if (data == nullptr)
    data = bfd_inferior_data_key.emplace (inf);

  return data;
}

/* Increment the BFD error count for STR and return the updated
   count.  */

static unsigned long
increment_bfd_error_count (std::string str)
{
  struct bfd_inferior_data *bid = get_bfd_inferior_data (current_inferior ());

  auto &map = bid->bfd_error_string_counts;
  return ++map[std::move (str)];
}

static bfd_error_handler_type default_bfd_error_handler;

/* Define a BFD error handler which will suppress the printing of
   messages which have been printed once already.  This is done on a
   per-inferior basis.  */

static void ATTRIBUTE_PRINTF (1, 0)
gdb_bfd_error_handler (const char *fmt, va_list ap)
{
  va_list ap_copy;

  va_copy(ap_copy, ap);
  const std::string str = string_vprintf (fmt, ap_copy);
  va_end (ap_copy);

  if (increment_bfd_error_count (std::move (str)) > 1)
    return;

  /* We must call the BFD mechanism for printing format strings since
     it supports additional format specifiers that GDB's vwarning() doesn't
     recognize.  It also outputs additional text, i.e. "BFD: ", which
     makes it clear that it's a BFD warning/error.  */
  (*default_bfd_error_handler) (fmt, ap);
}

/* See gdb_bfd.h.  */

void
gdb_bfd_init ()
{
  if (bfd_init () == BFD_INIT_MAGIC)
    {
#if CXX_STD_THREAD
      if (bfd_thread_init (gdb_bfd_lock, gdb_bfd_unlock, nullptr))
#endif
	return;
    }

  error (_("fatal error: libbfd ABI mismatch"));
}

void _initialize_gdb_bfd ();
void
_initialize_gdb_bfd ()
{
  all_bfds = htab_create_alloc (10, htab_hash_pointer, htab_eq_pointer,
				NULL, xcalloc, xfree);

  add_cmd ("bfds", class_maintenance, maintenance_info_bfds, _("\
List the BFDs that are currently open."),
	   &maintenanceinfolist);

  add_setshow_boolean_cmd ("bfd-sharing", no_class,
			   &bfd_sharing, _("\
Set whether gdb will share bfds that appear to be the same file."), _("\
Show whether gdb will share bfds that appear to be the same file."), _("\
When enabled gdb will reuse existing bfds rather than reopening the\n\
same file.  To decide if two files are the same then gdb compares the\n\
filename, file size, file modification time, and file inode."),
			   NULL,
			   &show_bfd_sharing,
			   &maintenance_set_cmdlist,
			   &maintenance_show_cmdlist);

  add_setshow_boolean_cmd ("bfd-cache", class_maintenance,
			   &debug_bfd_cache,
			   _("Set bfd cache debugging."),
			   _("Show bfd cache debugging."),
			   _("\
When non-zero, bfd cache specific debugging is enabled."),
			   NULL,
			   &show_bfd_cache_debug,
			   &setdebuglist, &showdebuglist);

  /* Hook the BFD error/warning handler to limit amount of output.  */
  default_bfd_error_handler = bfd_set_error_handler (gdb_bfd_error_handler);
}
