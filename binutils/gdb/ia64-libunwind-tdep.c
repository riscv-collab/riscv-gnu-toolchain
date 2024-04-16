/* Frame unwinder for ia64 frames using the libunwind library.

   Copyright (C) 2003-2024 Free Software Foundation, Inc.

   Written by Jeff Johnston, contributed by Red Hat Inc.

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

#include "inferior.h"
#include "frame.h"
#include "frame-base.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "gdbtypes.h"
#include "symtab.h"
#include "objfiles.h"
#include "regcache.h"

#include <dlfcn.h>

#include "ia64-libunwind-tdep.h"

#include "gdbsupport/preprocessor.h"

/* IA-64 is the only target that currently uses ia64-libunwind-tdep.
   Note how UNW_TARGET, UNW_OBJ, etc. are compile time constants below.
   Those come from libunwind's headers, and are target dependent.
   Also, some of libunwind's typedefs are target dependent, as e.g.,
   unw_word_t.  If some other target wants to use this, we will need
   to do some abstracting in order to make it possible to select which
   libunwind we're talking to at runtime (and have one per arch).  */

/* The following two macros are normally defined in <endian.h>.
   But systems such as ia64-hpux do not provide such header, so
   we just define them here if not already defined.  */
#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN    4321
#endif

static int libunwind_initialized;
static const registry<gdbarch>::key<libunwind_descr> libunwind_descr_handle;

/* Required function pointers from libunwind.  */
typedef int (unw_get_reg_p_ftype) (unw_cursor_t *, unw_regnum_t, unw_word_t *);
static unw_get_reg_p_ftype *unw_get_reg_p;
typedef int (unw_get_fpreg_p_ftype) (unw_cursor_t *, unw_regnum_t,
				     unw_fpreg_t *);
static unw_get_fpreg_p_ftype *unw_get_fpreg_p;
typedef int (unw_get_saveloc_p_ftype) (unw_cursor_t *, unw_regnum_t,
				       unw_save_loc_t *);
static unw_get_saveloc_p_ftype *unw_get_saveloc_p;
typedef int (unw_is_signal_frame_p_ftype) (unw_cursor_t *);
static unw_is_signal_frame_p_ftype *unw_is_signal_frame_p;
typedef int (unw_step_p_ftype) (unw_cursor_t *);
static unw_step_p_ftype *unw_step_p;
typedef int (unw_init_remote_p_ftype) (unw_cursor_t *, unw_addr_space_t,
				       void *);
static unw_init_remote_p_ftype *unw_init_remote_p;
typedef unw_addr_space_t (unw_create_addr_space_p_ftype) (unw_accessors_t *,
							  int);
static unw_create_addr_space_p_ftype *unw_create_addr_space_p;
typedef void (unw_destroy_addr_space_p_ftype) (unw_addr_space_t);
static unw_destroy_addr_space_p_ftype *unw_destroy_addr_space_p;
typedef int (unw_search_unwind_table_p_ftype) (unw_addr_space_t, unw_word_t,
					       unw_dyn_info_t *,
					       unw_proc_info_t *, int, void *);
static unw_search_unwind_table_p_ftype *unw_search_unwind_table_p;
typedef unw_word_t (unw_find_dyn_list_p_ftype) (unw_addr_space_t,
						unw_dyn_info_t *, void *);
static unw_find_dyn_list_p_ftype *unw_find_dyn_list_p;


struct libunwind_frame_cache
{
  CORE_ADDR base;
  CORE_ADDR func_addr;
  unw_cursor_t cursor;
  unw_addr_space_t as;
};

/* We need to qualify the function names with a platform-specific prefix
   to match the names used by the libunwind library.  The UNW_OBJ macro is
   provided by the libunwind.h header file.  */

#ifndef LIBUNWIND_SO
/* Use the stable ABI major version number.  `libunwind-ia64.so' is a link time
   only library, not a runtime one.  */
#define LIBUNWIND_SO "libunwind-" STRINGIFY(UNW_TARGET) ".so.8"

/* Provide also compatibility with older .so.  The two APIs are compatible, .8
   is only extended a bit, GDB does not use the extended API at all.  */
#define LIBUNWIND_SO_7 "libunwind-" STRINGIFY(UNW_TARGET) ".so.7"
#endif

static const char *get_reg_name = STRINGIFY(UNW_OBJ(get_reg));
static const char *get_fpreg_name = STRINGIFY(UNW_OBJ(get_fpreg));
static const char *get_saveloc_name = STRINGIFY(UNW_OBJ(get_save_loc));
static const char *is_signal_frame_name = STRINGIFY(UNW_OBJ(is_signal_frame));
static const char *step_name = STRINGIFY(UNW_OBJ(step));
static const char *init_remote_name = STRINGIFY(UNW_OBJ(init_remote));
static const char *create_addr_space_name
  = STRINGIFY(UNW_OBJ(create_addr_space));
static const char *destroy_addr_space_name
  = STRINGIFY(UNW_OBJ(destroy_addr_space));
static const char *search_unwind_table_name
  = STRINGIFY(UNW_OBJ(search_unwind_table));
static const char *find_dyn_list_name = STRINGIFY(UNW_OBJ(find_dyn_list));

static struct libunwind_descr *
libunwind_descr (struct gdbarch *gdbarch)
{
  struct libunwind_descr *result = libunwind_descr_handle.get (gdbarch);
  if (result == nullptr)
    result = libunwind_descr_handle.emplace (gdbarch);
  return result;
}

void
libunwind_frame_set_descr (struct gdbarch *gdbarch,
			   struct libunwind_descr *descr)
{
  struct libunwind_descr *arch_descr;

  gdb_assert (gdbarch != NULL);

  arch_descr = libunwind_descr (gdbarch);
  gdb_assert (arch_descr != NULL);

  /* Copy new descriptor info into arch descriptor.  */
  arch_descr->gdb2uw = descr->gdb2uw;
  arch_descr->uw2gdb = descr->uw2gdb;
  arch_descr->is_fpreg = descr->is_fpreg;
  arch_descr->accessors = descr->accessors;
  arch_descr->special_accessors = descr->special_accessors;
}

static struct libunwind_frame_cache *
libunwind_frame_cache (frame_info_ptr this_frame, void **this_cache)
{
  unw_accessors_t *acc;
  unw_addr_space_t as;
  unw_word_t fp;
  unw_regnum_t uw_sp_regnum;
  struct libunwind_frame_cache *cache;
  struct libunwind_descr *descr;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int ret;

  if (*this_cache)
    return (struct libunwind_frame_cache *) *this_cache;

  /* Allocate a new cache.  */
  cache = FRAME_OBSTACK_ZALLOC (struct libunwind_frame_cache);

  cache->func_addr = get_frame_func (this_frame);
  if (cache->func_addr == 0)
    /* This can happen when the frame corresponds to a function for which
       there is no debugging information nor any entry in the symbol table.
       This is probably a static function for which an entry in the symbol
       table was not created when the objfile got linked (observed in
       libpthread.so on ia64-hpux).

       The best we can do, in that case, is use the frame PC as the function
       address.  We don't need to give up since we still have the unwind
       record to help us perform the unwinding.  There is also another
       compelling to continue, because abandoning now means stopping
       the backtrace, which can never be helpful for the user.  */
    cache->func_addr = get_frame_pc (this_frame);

  /* Get a libunwind cursor to the previous frame.
  
     We do this by initializing a cursor.  Libunwind treats a new cursor
     as the top of stack and will get the current register set via the
     libunwind register accessor.  Now, we provide the platform-specific
     accessors and we set up the register accessor to use the frame
     register unwinding interfaces so that we properly get the registers
     for the current frame rather than the top.  We then use the unw_step
     function to move the libunwind cursor back one frame.  We can later
     use this cursor to find previous registers via the unw_get_reg
     interface which will invoke libunwind's special logic.  */
  descr = libunwind_descr (gdbarch);
  acc = (unw_accessors_t *) descr->accessors;
  as =  unw_create_addr_space_p (acc,
				 gdbarch_byte_order (gdbarch)
				 == BFD_ENDIAN_BIG
				 ? __BIG_ENDIAN
				 : __LITTLE_ENDIAN);

  unw_init_remote_p (&cache->cursor, as, this_frame);
  if (unw_step_p (&cache->cursor) < 0)
    {
      unw_destroy_addr_space_p (as);
      return NULL;
    }

  /* To get base address, get sp from previous frame.  */
  uw_sp_regnum = descr->gdb2uw (gdbarch_sp_regnum (gdbarch));
  ret = unw_get_reg_p (&cache->cursor, uw_sp_regnum, &fp);
  if (ret < 0)
    {
      unw_destroy_addr_space_p (as);
      error (_("Can't get libunwind sp register."));
    }

  cache->base = (CORE_ADDR)fp;
  cache->as = as;

  *this_cache = cache;
  return cache;
}

void
libunwind_frame_dealloc_cache (frame_info_ptr self, void *this_cache)
{
  struct libunwind_frame_cache *cache
    = (struct libunwind_frame_cache *) this_cache;

  if (cache->as)
    unw_destroy_addr_space_p (cache->as);
}

unw_word_t
libunwind_find_dyn_list (unw_addr_space_t as, unw_dyn_info_t *di, void *arg)
{
  return unw_find_dyn_list_p (as, di, arg);
}

/* Verify if there is sufficient libunwind information for the frame to use
   libunwind frame unwinding.  */
int
libunwind_frame_sniffer (const struct frame_unwind *self,
			 frame_info_ptr this_frame, void **this_cache)
{
  unw_cursor_t cursor;
  unw_accessors_t *acc;
  unw_addr_space_t as;
  struct libunwind_descr *descr;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int ret;

  /* To test for libunwind unwind support, initialize a cursor to
     the current frame and try to back up.  We use this same method
     when setting up the frame cache (see libunwind_frame_cache()).
     If libunwind returns success for this operation, it means that
     it has found sufficient libunwind unwinding information to do so.  */

  descr = libunwind_descr (gdbarch);
  acc = (unw_accessors_t *) descr->accessors;
  as =  unw_create_addr_space_p (acc,
				 gdbarch_byte_order (gdbarch)
				 == BFD_ENDIAN_BIG
				 ? __BIG_ENDIAN
				 : __LITTLE_ENDIAN);

  ret = unw_init_remote_p (&cursor, as, this_frame);

  if (ret < 0)
    {
      unw_destroy_addr_space_p (as);
      return 0;
    }

 
  /* Check to see if we have libunwind info by checking if we are in a 
     signal frame.  If it doesn't return an error, we have libunwind info
     and can use libunwind.  */
  ret = unw_is_signal_frame_p (&cursor);
  unw_destroy_addr_space_p (as);

  if (ret < 0)
    return 0;

  return 1;
}

void
libunwind_frame_this_id (frame_info_ptr this_frame, void **this_cache,
			 struct frame_id *this_id)
{
  struct libunwind_frame_cache *cache =
    libunwind_frame_cache (this_frame, this_cache);

  if (cache != NULL)
    (*this_id) = frame_id_build (cache->base, cache->func_addr);
}

struct value *
libunwind_frame_prev_register (frame_info_ptr this_frame,
			       void **this_cache, int regnum)
{
  struct libunwind_frame_cache *cache =
    libunwind_frame_cache (this_frame, this_cache);

  unw_save_loc_t sl;
  int ret;
  unw_word_t intval;
  unw_fpreg_t fpval;
  unw_regnum_t uw_regnum;
  struct libunwind_descr *descr;
  struct value *val = NULL;

  if (cache == NULL)
    return frame_unwind_got_constant (this_frame, regnum, 0);
  
  /* Convert from gdb register number to libunwind register number.  */
  descr = libunwind_descr (get_frame_arch (this_frame));
  uw_regnum = descr->gdb2uw (regnum);

  gdb_assert (regnum >= 0);

  if (!target_has_registers ())
    error (_("No registers."));

  if (uw_regnum < 0)
    return frame_unwind_got_constant (this_frame, regnum, 0);

  if (unw_get_saveloc_p (&cache->cursor, uw_regnum, &sl) < 0)
    return frame_unwind_got_constant (this_frame, regnum, 0);

  switch (sl.type)
    {
    case UNW_SLT_MEMORY:
      val = frame_unwind_got_memory (this_frame, regnum, sl.u.addr);
      break;

    case UNW_SLT_REG:
      val = frame_unwind_got_register (this_frame, regnum,
				       descr->uw2gdb (sl.u.regnum));
      break;
    case UNW_SLT_NONE:
      {
	/* The register is not stored at a specific memory address nor
	   inside another register.  So use libunwind to fetch the register
	   value for us, and create a constant value with the result.  */
	if (descr->is_fpreg (uw_regnum))
	  {
	    ret = unw_get_fpreg_p (&cache->cursor, uw_regnum, &fpval);
	    if (ret < 0)
	      return frame_unwind_got_constant (this_frame, regnum, 0);
	    val = frame_unwind_got_bytes (this_frame, regnum,
					  (gdb_byte *) &fpval);
	  }
	else
	  {
	    ret = unw_get_reg_p (&cache->cursor, uw_regnum, &intval);
	    if (ret < 0)
	      return frame_unwind_got_constant (this_frame, regnum, 0);
	    val = frame_unwind_got_constant (this_frame, regnum, intval);
	  }
	break;
      }
    }

  return val;
} 

/* The following is a glue routine to call the libunwind unwind table
   search function to get unwind information for a specified ip address.  */ 
int
libunwind_search_unwind_table (void *as, long ip, void *di,
			       void *pi, int need_unwind_info, void *args)
{
  return unw_search_unwind_table_p (*(unw_addr_space_t *) as, (unw_word_t) ip,
				    (unw_dyn_info_t *) di,
				    (unw_proc_info_t *) pi, need_unwind_info,
				    args);
}

/* Verify if we are in a sigtramp frame and we can use libunwind to unwind.  */
int
libunwind_sigtramp_frame_sniffer (const struct frame_unwind *self,
				  frame_info_ptr this_frame,
				  void **this_cache)
{
  unw_cursor_t cursor;
  unw_accessors_t *acc;
  unw_addr_space_t as;
  struct libunwind_descr *descr;
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  int ret;

  /* To test for libunwind unwind support, initialize a cursor to the
     current frame and try to back up.  We use this same method when
     setting up the frame cache (see libunwind_frame_cache()).  If
     libunwind returns success for this operation, it means that it
     has found sufficient libunwind unwinding information to do
     so.  */

  descr = libunwind_descr (gdbarch);
  acc = (unw_accessors_t *) descr->accessors;
  as =  unw_create_addr_space_p (acc,
				 gdbarch_byte_order (gdbarch)
				 == BFD_ENDIAN_BIG
				 ? __BIG_ENDIAN
				 : __LITTLE_ENDIAN);

  ret = unw_init_remote_p (&cursor, as, this_frame);

  if (ret < 0)
    {
      unw_destroy_addr_space_p (as);
      return 0;
    }

  /* Check to see if we are in a signal frame.  */
  ret = unw_is_signal_frame_p (&cursor);
  unw_destroy_addr_space_p (as);
  if (ret > 0)
    return 1;

  return 0;
}

/* The following routine is for accessing special registers of the top frame.
   A special set of accessors must be given that work without frame info.
   This is used by ia64 to access the rse registers r32-r127.  While they
   are usually located at BOF, this is not always true and only the libunwind
   info can decipher where they actually are.  */
int
libunwind_get_reg_special (struct gdbarch *gdbarch, readable_regcache *regcache,
			   int regnum, void *buf)
{
  unw_cursor_t cursor;
  unw_accessors_t *acc;
  unw_addr_space_t as;
  struct libunwind_descr *descr;
  int ret;
  unw_regnum_t uw_regnum;
  unw_word_t intval;
  unw_fpreg_t fpval;
  void *ptr;


  descr = libunwind_descr (gdbarch);
  acc = (unw_accessors_t *) descr->special_accessors;
  as =  unw_create_addr_space_p (acc,
				 gdbarch_byte_order (gdbarch)
				 == BFD_ENDIAN_BIG
				 ? __BIG_ENDIAN
				 : __LITTLE_ENDIAN);

  ret = unw_init_remote_p (&cursor, as, regcache);
  if (ret < 0)
    {
      unw_destroy_addr_space_p (as);
      return -1;
    }

  uw_regnum = descr->gdb2uw (regnum);

  if (descr->is_fpreg (uw_regnum))
    {
      ret = unw_get_fpreg_p (&cursor, uw_regnum, &fpval);
      ptr = &fpval;
    }
  else
    {
      ret = unw_get_reg_p (&cursor, uw_regnum, &intval);
      ptr = &intval;
    }

  unw_destroy_addr_space_p (as);

  if (ret < 0)
    return -1;

  if (buf)
    memcpy (buf, ptr, register_size (gdbarch, regnum));

  return 0;
}
  
static int
libunwind_load (void)
{
  void *handle;
  char *so_error = NULL;

  handle = dlopen (LIBUNWIND_SO, RTLD_NOW);
  if (handle == NULL)
    {
      so_error = xstrdup (dlerror ());
#ifdef LIBUNWIND_SO_7
      handle = dlopen (LIBUNWIND_SO_7, RTLD_NOW);
#endif /* LIBUNWIND_SO_7 */
    }
  if (handle == NULL)
    {
      gdb_printf (gdb_stderr, _("[GDB failed to load %s: %s]\n"),
		  LIBUNWIND_SO, so_error);
#ifdef LIBUNWIND_SO_7
      gdb_printf (gdb_stderr, _("[GDB failed to load %s: %s]\n"),
		  LIBUNWIND_SO_7, dlerror ());
#endif /* LIBUNWIND_SO_7 */
    }
  xfree (so_error);
  if (handle == NULL)
    return 0;

  /* Initialize pointers to the dynamic library functions we will use.  */

  unw_get_reg_p = (unw_get_reg_p_ftype *) dlsym (handle, get_reg_name);
  if (unw_get_reg_p == NULL)
    return 0;

  unw_get_fpreg_p = (unw_get_fpreg_p_ftype *) dlsym (handle, get_fpreg_name);
  if (unw_get_fpreg_p == NULL)
    return 0;

  unw_get_saveloc_p
    = (unw_get_saveloc_p_ftype *) dlsym (handle, get_saveloc_name);
  if (unw_get_saveloc_p == NULL)
    return 0;

  unw_is_signal_frame_p
    = (unw_is_signal_frame_p_ftype *) dlsym (handle, is_signal_frame_name);
  if (unw_is_signal_frame_p == NULL)
    return 0;

  unw_step_p = (unw_step_p_ftype *) dlsym (handle, step_name);
  if (unw_step_p == NULL)
    return 0;

  unw_init_remote_p
    = (unw_init_remote_p_ftype *) dlsym (handle, init_remote_name);
  if (unw_init_remote_p == NULL)
    return 0;

  unw_create_addr_space_p
    = (unw_create_addr_space_p_ftype *) dlsym (handle, create_addr_space_name);
  if (unw_create_addr_space_p == NULL)
    return 0;

  unw_destroy_addr_space_p
    = (unw_destroy_addr_space_p_ftype *) dlsym (handle,
						destroy_addr_space_name);
  if (unw_destroy_addr_space_p == NULL)
    return 0;

  unw_search_unwind_table_p
    = (unw_search_unwind_table_p_ftype *) dlsym (handle,
						 search_unwind_table_name);
  if (unw_search_unwind_table_p == NULL)
    return 0;

  unw_find_dyn_list_p
    = (unw_find_dyn_list_p_ftype *) dlsym (handle, find_dyn_list_name);
  if (unw_find_dyn_list_p == NULL)
    return 0;
   
  return 1;
}

int
libunwind_is_initialized (void)
{
  return libunwind_initialized;
}

void _initialize_libunwind_frame ();
void
_initialize_libunwind_frame ()
{
  libunwind_initialized = libunwind_load ();
}
