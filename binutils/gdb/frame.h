/* Definitions for dealing with stack frames, for GDB, the GNU debugger.

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

#if !defined (FRAME_H)
#define FRAME_H 1

/* The following is the intended naming schema for frame functions.
   It isn't 100% consistent, but it is approaching that.  Frame naming
   schema:

   Prefixes:

   get_frame_WHAT...(): Get WHAT from the THIS frame (functionally
   equivalent to THIS->next->unwind->what)

   frame_unwind_WHAT...(): Unwind THIS frame's WHAT from the NEXT
   frame.

   frame_unwind_caller_WHAT...(): Unwind WHAT for NEXT stack frame's
   real caller.  Any inlined functions in NEXT's stack frame are
   skipped.  Use these to ignore any potentially inlined functions,
   e.g. inlined into the first instruction of a library trampoline.

   get_stack_frame_WHAT...(): Get WHAT for THIS frame, but if THIS is
   inlined, skip to the containing stack frame.

   put_frame_WHAT...(): Put a value into this frame (unsafe, need to
   invalidate the frame / regcache afterwards) (better name more
   strongly hinting at its unsafeness)

   safe_....(): Safer version of various functions, doesn't throw an
   error (leave this for later?).  Returns true / non-NULL if the request
   succeeds, false / NULL otherwise.

   Suffixes:

   void /frame/_WHAT(): Read WHAT's value into the buffer parameter.

   ULONGEST /frame/_WHAT_unsigned(): Return an unsigned value (the
   alternative is *frame_unsigned_WHAT).

   LONGEST /frame/_WHAT_signed(): Return WHAT signed value.

   What:

   /frame/_memory* (frame, coreaddr, len [, buf]): Extract/return
   *memory.

   /frame/_register* (frame, regnum [, buf]): extract/return register.

   CORE_ADDR /frame/_{pc,sp,...} (frame): Resume address, innner most
   stack *address, ...

   */

#include "cli/cli-option.h"
#include "frame-id.h"
#include "gdbsupport/common-debug.h"
#include "gdbsupport/intrusive_list.h"

struct symtab_and_line;
struct frame_unwind;
struct frame_base;
struct block;
struct gdbarch;
struct ui_file;
struct ui_out;
struct frame_print_options;

/* The frame object.  */


/* Save and restore the currently selected frame.  */

class scoped_restore_selected_frame
{
public:
  /* Save the currently selected frame.  */
  scoped_restore_selected_frame ();

  /* Restore the currently selected frame.  */
  ~scoped_restore_selected_frame ();

  DISABLE_COPY_AND_ASSIGN (scoped_restore_selected_frame);

private:

  /* The ID and level of the previously selected frame.  */
  struct frame_id m_fid;
  int m_level;

  /* Save/restore the language as well, because selecting a frame
     changes the current language to the frame's language if "set
     language auto".  */
  enum language m_lang;
};

/* Flag to control debugging.  */

extern bool frame_debug;

/* Print a "frame" debug statement.  */

#define frame_debug_printf(fmt, ...) \
  debug_prefixed_printf_cond (frame_debug, "frame", fmt, ##__VA_ARGS__)

/* Print "frame" enter/exit debug statements.  */

#define FRAME_SCOPED_DEBUG_ENTER_EXIT \
  scoped_debug_enter_exit (frame_debug, "frame")

/* Construct a frame ID.  The first parameter is the frame's constant
   stack address (typically the outer-bound), and the second the
   frame's constant code address (typically the entry point).
   The special identifier address is set to indicate a wild card.  */
extern struct frame_id frame_id_build (CORE_ADDR stack_addr,
				       CORE_ADDR code_addr);

/* Construct a special frame ID.  The first parameter is the frame's constant
   stack address (typically the outer-bound), the second is the
   frame's constant code address (typically the entry point),
   and the third parameter is the frame's special identifier address.  */
extern struct frame_id frame_id_build_special (CORE_ADDR stack_addr,
					       CORE_ADDR code_addr,
					       CORE_ADDR special_addr);

/* Construct a frame ID representing a frame where the stack address
   exists, but is unavailable.  CODE_ADDR is the frame's constant code
   address (typically the entry point).  The special identifier
   address is set to indicate a wild card.  */
extern struct frame_id frame_id_build_unavailable_stack (CORE_ADDR code_addr);

/* Construct a frame ID representing a frame where the stack address
   exists, but is unavailable.  CODE_ADDR is the frame's constant code
   address (typically the entry point).  SPECIAL_ADDR is the special
   identifier address.  */
extern struct frame_id
  frame_id_build_unavailable_stack_special (CORE_ADDR code_addr,
					    CORE_ADDR special_addr);

/* Construct a wild card frame ID.  The parameter is the frame's constant
   stack address (typically the outer-bound).  The code address as well
   as the special identifier address are set to indicate wild cards.  */
extern struct frame_id frame_id_build_wild (CORE_ADDR stack_addr);

/* Construct a frame ID for a sentinel frame.

   If either STACK_ADDR or CODE_ADDR is not 0, the ID represents a sentinel
   frame for a user-created frame.  STACK_ADDR and CODE_ADDR are the addresses
   used to create the frame.

   If STACK_ADDR and CODE_ADDR are both 0, the ID represents a regular sentinel
   frame (i.e. the "next" frame of the target's current frame).  */
extern frame_id frame_id_build_sentinel (CORE_ADDR stack_addr, CORE_ADDR code_addr);

/* Returns true when L is a valid frame.  */
extern bool frame_id_p (frame_id l);

/* Returns true when L is a valid frame representing a frame made up by GDB
   without stack data representation in inferior, such as INLINE_FRAME or
   TAILCALL_FRAME.  */
extern bool frame_id_artificial_p (frame_id l);

/* Frame types.  Some are real, some are signal trampolines, and some
   are completely artificial (dummy).  */

enum frame_type
{
  /* A true stack frame, created by the target program during normal
     execution.  */
  NORMAL_FRAME,
  /* A fake frame, created by GDB when performing an inferior function
     call.  */
  DUMMY_FRAME,
  /* A frame representing an inlined function, associated with an
     upcoming (prev, outer, older) NORMAL_FRAME.  */
  INLINE_FRAME,
  /* A virtual frame of a tail call - see dwarf2_tailcall_frame_unwind.  */
  TAILCALL_FRAME,
  /* In a signal handler, various OSs handle this in various ways.
     The main thing is that the frame may be far from normal.  */
  SIGTRAMP_FRAME,
  /* Fake frame representing a cross-architecture call.  */
  ARCH_FRAME,
  /* Sentinel or registers frame.  This frame obtains register values
     direct from the inferior's registers.  */
  SENTINEL_FRAME
};

/* Return a string representation of TYPE.  */

extern const char *frame_type_str (frame_type type);

/* A wrapper for "frame_info *".  frame_info objects are invalidated
   whenever reinit_frame_cache is called.  This class arranges to
   invalidate the pointer when appropriate.  This is done to help
   detect a GDB bug that was relatively common.

   A small amount of code must still operate on raw pointers, so a
   "get" method is provided.  However, you should normally not use
   this in new code.  */

class frame_info_ptr : public intrusive_list_node<frame_info_ptr>
{
public:
  /* Create a frame_info_ptr from a raw pointer.  */
  explicit frame_info_ptr (struct frame_info *ptr);

  /* Create a null frame_info_ptr.  */
  frame_info_ptr ()
  {
    frame_list.push_back (*this);
  }

  frame_info_ptr (std::nullptr_t)
  {
    frame_list.push_back (*this);
  }

  frame_info_ptr (const frame_info_ptr &other)
    : m_ptr (other.m_ptr),
      m_cached_id (other.m_cached_id),
      m_cached_level (other.m_cached_level)
  {
    frame_list.push_back (*this);
  }

  frame_info_ptr (frame_info_ptr &&other)
    : m_ptr (other.m_ptr),
      m_cached_id (other.m_cached_id),
      m_cached_level (other.m_cached_level)
  {
    other.m_ptr = nullptr;
    other.m_cached_id = null_frame_id;
    other.m_cached_level = invalid_level;
    frame_list.push_back (*this);
  }

  ~frame_info_ptr ()
  {
    /* If this node has static storage, it should be be deleted before
       frame_list.  */
    frame_list.erase (frame_list.iterator_to (*this));
  }

  frame_info_ptr &operator= (const frame_info_ptr &other)
  {
    m_ptr = other.m_ptr;
    m_cached_id = other.m_cached_id;
    m_cached_level = other.m_cached_level;
    return *this;
  }

  frame_info_ptr &operator= (std::nullptr_t)
  {
    m_ptr = nullptr;
    m_cached_id = null_frame_id;
    m_cached_level = invalid_level;
    return *this;
  }

  frame_info_ptr &operator= (frame_info_ptr &&other)
  {
    m_ptr = other.m_ptr;
    m_cached_id = other.m_cached_id;
    m_cached_level = other.m_cached_level;
    other.m_ptr = nullptr;
    other.m_cached_id = null_frame_id;
    other.m_cached_level = invalid_level;
    return *this;
  }

  frame_info *operator-> () const
  { return this->reinflate (); }

  /* Fetch the underlying pointer.  Note that new code should
     generally not use this -- avoid it if at all possible.  */
  frame_info *get () const
  {
    if (this->is_null ())
      return nullptr;

    return this->reinflate ();
  }

  /* Return true if this object is empty (does not wrap a frame_info
     object).  */

  bool is_null () const
  {
    return m_cached_level == this->invalid_level;
  };

  /* This exists for compatibility with pre-existing code that checked
     a "frame_info *" using "!".  */
  bool operator! () const
  {
    return this->is_null ();
  }

  /* This exists for compatibility with pre-existing code that checked
     a "frame_info *" like "if (ptr)".  */
  explicit operator bool () const
  {
    return !this->is_null ();
  }

  /* Invalidate this pointer.  */
  void invalidate ()
  {
    m_ptr = nullptr;
  }

private:
  /* We sometimes need to construct frame_info_ptr objects around the
     sentinel_frame, which has level -1.  Therefore, make the invalid frame
     level value -2.  */
  static constexpr int invalid_level = -2;

  /* Use the cached frame level and id to reinflate the pointer, and return
     it.  */
  frame_info *reinflate () const;

  /* The underlying pointer.  */
  mutable frame_info *m_ptr = nullptr;

  /* The frame_id of the underlying pointer.

     For the current target frames (frames with level 0, obtained through
     get_current_frame), we don't save the frame id, we leave it at
     null_frame_id.  For user-created frames (also with level 0, but created
     with create_new_frame), we do save the id.  */
  frame_id m_cached_id = null_frame_id;

  /* The frame level of the underlying pointer.  */
  int m_cached_level = invalid_level;

  /* All frame_info_ptr objects are kept on an intrusive list.
     This keeps their construction and destruction costs
     reasonably small.  */
  static intrusive_list<frame_info_ptr> frame_list;

  /* A friend so it can invalidate the pointers.  */
  friend void reinit_frame_cache ();
};

static inline bool
operator== (const frame_info *self, const frame_info_ptr &other)
{
  if (self == nullptr || other.is_null ())
    return self == nullptr && other.is_null ();

  return self == other.get ();
}

static inline bool
operator== (const frame_info_ptr &self, const frame_info_ptr &other)
{
  if (self.is_null () || other.is_null ())
    return self.is_null () && other.is_null ();

  return self.get () == other.get ();
}

static inline bool
operator== (const frame_info_ptr &self, const frame_info *other)
{
  if (self.is_null () || other == nullptr)
    return self.is_null () && other == nullptr;

  return self.get () == other;
}

static inline bool
operator!= (const frame_info *self, const frame_info_ptr &other)
{
  return !(self == other);
}

static inline bool
operator!= (const frame_info_ptr &self, const frame_info_ptr &other)
{
  return !(self == other);
}

static inline bool
operator!= (const frame_info_ptr &self, const frame_info *other)
{
  return !(self == other);
}

/* For every stopped thread, GDB tracks two frames: current and
   selected.  Current frame is the inner most frame of the selected
   thread.  Selected frame is the one being examined by the GDB
   CLI (selected using `up', `down', ...).  The frames are created
   on-demand (via get_prev_frame()) and then held in a frame cache.  */
/* FIXME: cagney/2002-11-28: Er, there is a lie here.  If you do the
   sequence: `thread 1; up; thread 2; thread 1' you lose thread 1's
   selected frame.  At present GDB only tracks the selected frame of
   the current thread.  But be warned, that might change.  */
/* FIXME: cagney/2002-11-14: At any time, only one thread's selected
   and current frame can be active.  Switching threads causes gdb to
   discard all that cached frame information.  Ulgh!  Instead, current
   and selected frame should be bound to a thread.  */

/* On demand, create the inner most frame using information found in
   the inferior.  If the inner most frame can't be created, throw an
   error.  */
extern frame_info_ptr get_current_frame (void);

/* Does the current target interface have enough state to be able to
   query the current inferior for frame info, and is the inferior in a
   state where that is possible?  */
extern bool has_stack_frames ();

/* Invalidates the frame cache (this function should have been called
   invalidate_cached_frames).

   FIXME: cagney/2002-11-28: There should be two methods: one that
   reverts the thread's selected frame back to current frame (for when
   the inferior resumes) and one that does not (for when the user
   modifies the target invalidating the frame cache).  */
extern void reinit_frame_cache (void);

/* Return the selected frame.  Always returns non-NULL.  If there
   isn't an inferior sufficient for creating a frame, an error is
   thrown.  When MESSAGE is non-NULL, use it for the error message,
   otherwise use a generic error message.  */
/* FIXME: cagney/2002-11-28: At present, when there is no selected
   frame, this function always returns the current (inner most) frame.
   It should instead, when a thread has previously had its frame
   selected (but not resumed) and the frame cache invalidated, find
   and then return that thread's previously selected frame.  */
extern frame_info_ptr get_selected_frame (const char *message = nullptr);

/* Select a specific frame.  */
extern void select_frame (frame_info_ptr);

/* Save the frame ID and frame level of the selected frame in FRAME_ID
   and FRAME_LEVEL, to be restored later with restore_selected_frame.

   This is preferred over getting the same info out of
   get_selected_frame directly because this function does not create
   the selected-frame's frame_info object if it hasn't been created
   yet, and thus is more efficient and doesn't throw.  */
extern void save_selected_frame (frame_id *frame_id, int *frame_level)
  noexcept;

/* Restore selected frame as saved with save_selected_frame.

   Does not try to find the corresponding frame_info object.  Instead
   the next call to get_selected_frame will look it up and cache the
   result.

   This function does not throw.  It is designed to be safe to called
   from the destructors of RAII types.  */
extern void restore_selected_frame (frame_id frame_id, int frame_level)
  noexcept;

/* Given a FRAME, return the next (more inner, younger) or previous
   (more outer, older) frame.  */
extern frame_info_ptr get_prev_frame (frame_info_ptr);
extern frame_info_ptr get_next_frame (frame_info_ptr);

/* Like get_next_frame(), but allows return of the sentinel frame.  NULL
   is never returned.  */
extern frame_info_ptr get_next_frame_sentinel_okay (frame_info_ptr);

/* Return a "struct frame_info" corresponding to the frame that called
   THIS_FRAME.  Returns NULL if there is no such frame.

   Unlike get_prev_frame, this function always tries to unwind the
   frame.  */
extern frame_info_ptr get_prev_frame_always (frame_info_ptr);

/* Given a frame's ID, relocate the frame.  Returns NULL if the frame
   is not found.  */
extern frame_info_ptr frame_find_by_id (frame_id id);

/* Base attributes of a frame: */

/* The frame's `resume' address.  Where the program will resume in
   this frame.

   This replaced: frame->pc; */
extern CORE_ADDR get_frame_pc (frame_info_ptr);

/* Same as get_frame_pc, but return a boolean indication of whether
   the PC is actually available, instead of throwing an error.  */

extern bool get_frame_pc_if_available (frame_info_ptr frame, CORE_ADDR *pc);

/* An address (not necessarily aligned to an instruction boundary)
   that falls within THIS frame's code block.

   When a function call is the last statement in a block, the return
   address for the call may land at the start of the next block.
   Similarly, if a no-return function call is the last statement in
   the function, the return address may end up pointing beyond the
   function, and possibly at the start of the next function.

   These methods make an allowance for this.  For call frames, this
   function returns the frame's PC-1 which "should" be an address in
   the frame's block.  */

extern CORE_ADDR get_frame_address_in_block (frame_info_ptr this_frame);

/* Same as get_frame_address_in_block, but returns a boolean
   indication of whether the frame address is determinable (when the
   PC is unavailable, it will not be), instead of possibly throwing an
   error trying to read an unavailable PC.  */

extern bool get_frame_address_in_block_if_available (frame_info_ptr this_frame,
						     CORE_ADDR *pc);

/* The frame's inner-most bound.  AKA the stack-pointer.  Confusingly
   known as top-of-stack.  */

extern CORE_ADDR get_frame_sp (frame_info_ptr);

/* Following on from the `resume' address.  Return the entry point
   address of the function containing that resume address, or zero if
   that function isn't known.  */
extern CORE_ADDR get_frame_func (frame_info_ptr fi);

/* Same as get_frame_func, but returns a boolean indication of whether
   the frame function is determinable (when the PC is unavailable, it
   will not be), instead of possibly throwing an error trying to read
   an unavailable PC.  */

extern bool get_frame_func_if_available (frame_info_ptr fi, CORE_ADDR *);

/* Closely related to the resume address, various symbol table
   attributes that are determined by the PC.  Note that for a normal
   frame, the PC refers to the resume address after the return, and
   not the call instruction.  In such a case, the address is adjusted
   so that it (approximately) identifies the call site (and not the
   return site).

   NOTE: cagney/2002-11-28: The frame cache could be used to cache the
   computed value.  Working on the assumption that the bottle-neck is
   in the single step code, and that code causes the frame cache to be
   constantly flushed, caching things in a frame is probably of little
   benefit.  As they say `show us the numbers'.

   NOTE: cagney/2002-11-28: Plenty more where this one came from:
   find_frame_block(), find_frame_partial_function(),
   find_frame_symtab(), find_frame_function().  Each will need to be
   carefully considered to determine if the real intent was for it to
   apply to the PC or the adjusted PC.  */
extern symtab_and_line find_frame_sal (frame_info_ptr frame);

/* Set the current source and line to the location given by frame
   FRAME, if possible.  */

void set_current_sal_from_frame (frame_info_ptr);

/* Return the frame base (what ever that is) (DEPRECATED).

   Old code was trying to use this single method for two conflicting
   purposes.  Such code needs to be updated to use either of:

   get_frame_id: A low level frame unique identifier, that consists of
   both a stack and a function address, that can be used to uniquely
   identify a frame.  This value is determined by the frame's
   low-level unwinder, the stack part [typically] being the
   top-of-stack of the previous frame, and the function part being the
   function's start address.  Since the correct identification of a
   frameless function requires both a stack and function address,
   the old get_frame_base method was not sufficient.

   get_frame_base_address: get_frame_locals_address:
   get_frame_args_address: A set of high-level debug-info dependant
   addresses that fall within the frame.  These addresses almost
   certainly will not match the stack address part of a frame ID (as
   returned by get_frame_base).

   This replaced: frame->frame; */

extern CORE_ADDR get_frame_base (frame_info_ptr);

/* Return the per-frame unique identifier.  Can be used to relocate a
   frame after a frame cache flush (and other similar operations).  If
   FI is NULL, return the null_frame_id.  */
extern struct frame_id get_frame_id (frame_info_ptr fi);
extern struct frame_id get_stack_frame_id (frame_info_ptr fi);
extern struct frame_id frame_unwind_caller_id (frame_info_ptr next_frame);

/* Assuming that a frame is `normal', return its base-address, or 0 if
   the information isn't available.  NOTE: This address is really only
   meaningful to the frame's high-level debug info.  */
extern CORE_ADDR get_frame_base_address (frame_info_ptr);

/* Assuming that a frame is `normal', return the base-address of the
   local variables, or 0 if the information isn't available.  NOTE:
   This address is really only meaningful to the frame's high-level
   debug info.  Typically, the argument and locals share a single
   base-address.  */
extern CORE_ADDR get_frame_locals_address (frame_info_ptr);

/* Assuming that a frame is `normal', return the base-address of the
   parameter list, or 0 if that information isn't available.  NOTE:
   This address is really only meaningful to the frame's high-level
   debug info.  Typically, the argument and locals share a single
   base-address.  */
extern CORE_ADDR get_frame_args_address (frame_info_ptr);

/* The frame's level: 0 for innermost, 1 for its caller, ...; or -1
   for an invalid frame).  */
extern int frame_relative_level (frame_info_ptr fi);

/* Return the frame's type.  */

extern enum frame_type get_frame_type (frame_info_ptr);

/* Return the frame's program space.  */
extern struct program_space *get_frame_program_space (frame_info_ptr);

/* Unwind THIS frame's program space from the NEXT frame.  */
extern struct program_space *frame_unwind_program_space (frame_info_ptr);

class address_space;

/* Return the frame's address space.  */
extern const address_space *get_frame_address_space (frame_info_ptr);

/* A frame may have a "static link".  That is, in some languages, a
   nested function may have access to variables from the enclosing
   block and frame.  This function looks for a frame's static link.
   If found, returns the corresponding frame; otherwise, returns a
   null frame_info_ptr.  */
extern frame_info_ptr frame_follow_static_link (frame_info_ptr frame);

/* For frames where we can not unwind further, describe why.  */

enum unwind_stop_reason
  {
#define SET(name, description) name,
#define FIRST_ENTRY(name) UNWIND_FIRST = name,
#define LAST_ENTRY(name) UNWIND_LAST = name,
#define FIRST_ERROR(name) UNWIND_FIRST_ERROR = name,

#include "unwind_stop_reasons.def"
#undef SET
#undef FIRST_ENTRY
#undef LAST_ENTRY
#undef FIRST_ERROR
  };

/* Return the reason why we can't unwind past this frame.  */

enum unwind_stop_reason get_frame_unwind_stop_reason (frame_info_ptr);

/* Translate a reason code to an informative string.  This converts the
   generic stop reason codes into a generic string describing the code.
   For a possibly frame specific string explaining the stop reason, use
   FRAME_STOP_REASON_STRING instead.  */

const char *unwind_stop_reason_to_string (enum unwind_stop_reason);

/* Return a possibly frame specific string explaining why the unwind
   stopped here.  E.g., if unwinding tripped on a memory error, this
   will return the error description string, which includes the address
   that we failed to access.  If there's no specific reason stored for
   a frame then a generic reason string will be returned.

   Should only be called for frames that don't have a previous frame.  */

const char *frame_stop_reason_string (frame_info_ptr);

/* Unwind the stack frame so that the value of REGNUM, in the previous
   (up, older) frame is returned.  If VALUEP is NULL, don't
   fetch/compute the value.  Instead just return the location of the
   value.  */
extern void frame_register_unwind (frame_info_ptr frame, int regnum,
				   int *optimizedp, int *unavailablep,
				   enum lval_type *lvalp,
				   CORE_ADDR *addrp, int *realnump,
				   gdb_byte *valuep);

/* Fetch a register from this, or unwind a register from the next
   frame.  Note that the get_frame methods are wrappers to
   frame->next->unwind.  They all [potentially] throw an error if the
   fetch fails.  The value methods never return NULL, but usually
   do return a lazy value.  */

extern void frame_unwind_register (frame_info_ptr next_frame,
				   int regnum, gdb_byte *buf);
extern void get_frame_register (frame_info_ptr frame,
				int regnum, gdb_byte *buf);

struct value *frame_unwind_register_value (frame_info_ptr next_frame,
					   int regnum);
struct value *get_frame_register_value (frame_info_ptr frame,
					int regnum);

extern LONGEST frame_unwind_register_signed (frame_info_ptr next_frame,
					     int regnum);
extern LONGEST get_frame_register_signed (frame_info_ptr frame,
					  int regnum);
extern ULONGEST frame_unwind_register_unsigned (frame_info_ptr next_frame,
						int regnum);
extern ULONGEST get_frame_register_unsigned (frame_info_ptr frame,
					     int regnum);

/* Read a register from this, or unwind a register from the next
   frame.  Note that the read_frame methods are wrappers to
   get_frame_register_value, that do not throw if the result is
   optimized out or unavailable.  */

extern bool read_frame_register_unsigned (frame_info_ptr frame,
					  int regnum, ULONGEST *val);

/* The reverse.  Store a register value relative to NEXT_FRAME's previous frame.
   Note: this call makes the frame's state undefined.  The register and frame
   caches must be flushed.  */
extern void put_frame_register (frame_info_ptr next_frame, int regnum,
				gdb::array_view<const gdb_byte> buf);

/* Read LEN bytes from one or multiple registers starting with REGNUM in
   NEXT_FRAME's previous frame, starting at OFFSET, into BUF.  If the register
   contents are optimized out or unavailable, set *OPTIMIZEDP, *UNAVAILABLEP
   accordingly.  */
extern bool get_frame_register_bytes (frame_info_ptr next_frame, int regnum,
				      CORE_ADDR offset,
				      gdb::array_view<gdb_byte> buffer,
				      int *optimizedp, int *unavailablep);

/* Write bytes from BUFFER to one or multiple registers starting with REGNUM
   in NEXT_FRAME's previous frame, starting at OFFSET.  */
extern void put_frame_register_bytes (frame_info_ptr next_frame, int regnum,
				      CORE_ADDR offset,
				      gdb::array_view<const gdb_byte> buffer);

/* Unwind the PC.  Strictly speaking return the resume address of the
   calling frame.  For GDB, `pc' is the resume address and not a
   specific register.  */

extern CORE_ADDR frame_unwind_caller_pc (frame_info_ptr frame);

/* Discard the specified frame.  Restoring the registers to the state
   of the caller.  */
extern void frame_pop (frame_info_ptr frame);

/* Return memory from the specified frame.  A frame knows its thread /
   LWP and hence can find its way down to a target.  The assumption
   here is that the current and previous frame share a common address
   space.

   If the memory read fails, these methods throw an error.

   NOTE: cagney/2003-06-03: Should there be unwind versions of these
   methods?  That isn't clear.  Can code, for instance, assume that
   this and the previous frame's memory or architecture are identical?
   If architecture / memory changes are always separated by special
   adaptor frames this should be ok.  */

extern void get_frame_memory (frame_info_ptr this_frame, CORE_ADDR addr,
			      gdb::array_view<gdb_byte> buffer);
extern LONGEST get_frame_memory_signed (frame_info_ptr this_frame,
					CORE_ADDR memaddr, int len);
extern ULONGEST get_frame_memory_unsigned (frame_info_ptr this_frame,
					   CORE_ADDR memaddr, int len);

/* Same as above, but return true zero when the entire memory read
   succeeds, false otherwise.  */
extern bool safe_frame_unwind_memory (frame_info_ptr this_frame, CORE_ADDR addr,
				      gdb::array_view<gdb_byte> buffer);

/* Return this frame's architecture.  */
extern struct gdbarch *get_frame_arch (frame_info_ptr this_frame);

/* Return the previous frame's architecture.  */
extern struct gdbarch *frame_unwind_arch (frame_info_ptr next_frame);

/* Return the previous frame's architecture, skipping inline functions.  */
extern struct gdbarch *frame_unwind_caller_arch (frame_info_ptr frame);


/* Values for the source flag to be used in print_frame_info ().
   For all the cases below, the address is never printed if
   'set print address' is off.  When 'set print address' is on,
   the address is printed if the program counter is not at the
   beginning of the source line of the frame
   and PRINT_WHAT is != LOC_AND_ADDRESS.  */
enum print_what
  {
    /* Print only the address, source line, like in stepi.  */
    SRC_LINE = -1,
    /* Print only the location, i.e. level, address,
       function, args (as controlled by 'set print frame-arguments'),
       file, line, line num.  */
    LOCATION,
    /* Print both of the above.  */
    SRC_AND_LOC,
    /* Print location only, print the address even if the program counter
       is at the beginning of the source line.  */
    LOC_AND_ADDRESS,
    /* Print only level and function,
       i.e. location only, without address, file, line, line num.  */
    SHORT_LOCATION
  };

/* Allocate zero initialized memory from the frame cache obstack.
   Appendices to the frame info (such as the unwind cache) should
   allocate memory using this method.  */

extern void *frame_obstack_zalloc (unsigned long size);
#define FRAME_OBSTACK_ZALLOC(TYPE) \
  ((TYPE *) frame_obstack_zalloc (sizeof (TYPE)))
#define FRAME_OBSTACK_CALLOC(NUMBER,TYPE) \
  ((TYPE *) frame_obstack_zalloc ((NUMBER) * sizeof (TYPE)))

class readonly_detached_regcache;
/* Create a regcache, and copy the frame's registers into it.  */
std::unique_ptr<readonly_detached_regcache> frame_save_as_regcache
    (frame_info_ptr this_frame);

extern const struct block *get_frame_block (frame_info_ptr,
					    CORE_ADDR *addr_in_block);

/* Return the `struct block' that belongs to the selected thread's
   selected frame.  If the inferior has no state, return NULL.

   NOTE: cagney/2002-11-29:

   No state?  Does the inferior have any execution state (a core file
   does, an executable does not).  At present the code tests
   `target_has_stack' but I'm left wondering if it should test
   `target_has_registers' or, even, a merged target_has_state.

   Should it look at the most recently specified SAL?  If the target
   has no state, should this function try to extract a block from the
   most recently selected SAL?  That way `list foo' would give it some
   sort of reference point.  Then again, perhaps that would confuse
   things.

   Calls to this function can be broken down into two categories: Code
   that uses the selected block as an additional, but optional, data
   point; Code that uses the selected block as a prop, when it should
   have the relevant frame/block/pc explicitly passed in.

   The latter can be eliminated by correctly parameterizing the code,
   the former though is more interesting.  Per the "address" command,
   it occurs in the CLI code and makes it possible for commands to
   work, even when the inferior has no state.  */

extern const struct block *get_selected_block (CORE_ADDR *addr_in_block);

extern struct symbol *get_frame_function (frame_info_ptr);

extern CORE_ADDR get_pc_function_start (CORE_ADDR);

extern frame_info_ptr find_relative_frame (frame_info_ptr, int *);

/* Wrapper over print_stack_frame modifying current_uiout with UIOUT for
   the function call.  */

extern void print_stack_frame_to_uiout (struct ui_out *uiout,
					frame_info_ptr, int print_level,
					enum print_what print_what,
					int set_current_sal);

extern void print_stack_frame (frame_info_ptr, int print_level,
			       enum print_what print_what,
			       int set_current_sal);

extern void print_frame_info (const frame_print_options &fp_opts,
			      frame_info_ptr, int print_level,
			      enum print_what print_what, int args,
			      int set_current_sal);

extern frame_info_ptr block_innermost_frame (const struct block *);

extern bool deprecated_frame_register_read (frame_info_ptr frame, int regnum,
					    gdb_byte *buf);

/* From stack.c.  */

/* The possible choices of "set print frame-arguments".  */
extern const char print_frame_arguments_all[];
extern const char print_frame_arguments_scalars[];
extern const char print_frame_arguments_none[];

/* The possible choices of "set print frame-info".  */
extern const char print_frame_info_auto[];
extern const char print_frame_info_source_line[];
extern const char print_frame_info_location[];
extern const char print_frame_info_source_and_location[];
extern const char print_frame_info_location_and_address[];
extern const char print_frame_info_short_location[];

/* The possible choices of "set print entry-values".  */
extern const char print_entry_values_no[];
extern const char print_entry_values_only[];
extern const char print_entry_values_preferred[];
extern const char print_entry_values_if_needed[];
extern const char print_entry_values_both[];
extern const char print_entry_values_compact[];
extern const char print_entry_values_default[];

/* Data for the frame-printing "set print" settings exposed as command
   options.  */

struct frame_print_options
{
  const char *print_frame_arguments = print_frame_arguments_scalars;
  const char *print_frame_info = print_frame_info_auto;
  const char *print_entry_values = print_entry_values_default;

  /* If true, don't invoke pretty-printers for frame
     arguments.  */
  bool print_raw_frame_arguments;
};

/* The values behind the global "set print ..." settings.  */
extern frame_print_options user_frame_print_options;

/* Inferior function parameter value read in from a frame.  */

struct frame_arg
{
  /* Symbol for this parameter used for example for its name.  */
  struct symbol *sym = nullptr;

  /* Value of the parameter.  It is NULL if ERROR is not NULL; if both VAL and
     ERROR are NULL this parameter's value should not be printed.  */
  struct value *val = nullptr;

  /* String containing the error message, it is more usually NULL indicating no
     error occurred reading this parameter.  */
  gdb::unique_xmalloc_ptr<char> error;

  /* One of the print_entry_values_* entries as appropriate specifically for
     this frame_arg.  It will be different from print_entry_values.  With
     print_entry_values_no this frame_arg should be printed as a normal
     parameter.  print_entry_values_only says it should be printed as entry
     value parameter.  print_entry_values_compact says it should be printed as
     both as a normal parameter and entry values parameter having the same
     value - print_entry_values_compact is not permitted fi ui_out_is_mi_like_p
     (in such case print_entry_values_no and print_entry_values_only is used
     for each parameter kind specifically.  */
  const char *entry_kind = nullptr;
};

extern void read_frame_arg (const frame_print_options &fp_opts,
			    symbol *sym, frame_info_ptr frame,
			    struct frame_arg *argp,
			    struct frame_arg *entryargp);
extern void read_frame_local (struct symbol *sym, frame_info_ptr frame,
			      struct frame_arg *argp);

extern void info_args_command (const char *, int);

extern void info_locals_command (const char *, int);

extern void return_command (const char *, int);

/* Set FRAME's unwinder temporarily, so that we can call a sniffer.
   If sniffing fails, the caller should be sure to call
   frame_cleanup_after_sniffer.  */

extern void frame_prepare_for_sniffer (frame_info_ptr frame,
				       const struct frame_unwind *unwind);

/* Clean up after a failed (wrong unwinder) attempt to unwind past
   FRAME.  */

extern void frame_cleanup_after_sniffer (frame_info_ptr frame);

/* Notes (cagney/2002-11-27, drow/2003-09-06):

   You might think that calls to this function can simply be replaced by a
   call to get_selected_frame().

   Unfortunately, it isn't that easy.

   The relevant code needs to be audited to determine if it is
   possible (or practical) to instead pass the applicable frame in as a
   parameter.  For instance, DEPRECATED_DO_REGISTERS_INFO() relied on
   the deprecated_selected_frame global, while its replacement,
   PRINT_REGISTERS_INFO(), is parameterized with the selected frame.
   The only real exceptions occur at the edge (in the CLI code) where
   user commands need to pick up the selected frame before proceeding.

   There are also some functions called with a NULL frame meaning either "the
   program is not running" or "use the selected frame".

   This is important.  GDB is trying to stamp out the hack:

   saved_frame = deprecated_safe_get_selected_frame ();
   select_frame (...);
   hack_using_global_selected_frame ();
   select_frame (saved_frame);

   Take care!

   This function calls get_selected_frame if the inferior should have a
   frame, or returns NULL otherwise.  */

extern frame_info_ptr deprecated_safe_get_selected_frame (void);

/* Create a frame using the specified BASE and PC.  */

extern frame_info_ptr create_new_frame (CORE_ADDR base, CORE_ADDR pc);

/* Return true if the frame unwinder for frame FI is UNWINDER; false
   otherwise.  */

extern bool frame_unwinder_is (frame_info_ptr fi, const frame_unwind *unwinder);

/* Return the language of FRAME.  */

extern enum language get_frame_language (frame_info_ptr frame);

/* Return the first non-tailcall frame above FRAME or FRAME if it is not a
   tailcall frame.  Return NULL if FRAME is the start of a tailcall-only
   chain.  */

extern frame_info_ptr skip_tailcall_frames (frame_info_ptr frame);

/* Return the first frame above FRAME or FRAME of which the code is
   writable.  */

extern frame_info_ptr skip_unwritable_frames (frame_info_ptr frame);

/* Data for the "set backtrace" settings.  */

struct set_backtrace_options
{
  /* Flag to indicate whether backtraces should continue past
     main.  */
  bool backtrace_past_main = false;

  /* Flag to indicate whether backtraces should continue past
     entry.  */
  bool backtrace_past_entry = false;

  /* Upper bound on the number of backtrace levels.  Note this is not
     exposed as a command option, because "backtrace" and "frame
     apply" already have other means to set a frame count limit.  */
  unsigned int backtrace_limit = UINT_MAX;
};

/* The corresponding option definitions.  */
extern const gdb::option::option_def set_backtrace_option_defs[2];

/* The values behind the global "set backtrace ..." settings.  */
extern set_backtrace_options user_set_backtrace_options;

/* Get the number of calls to reinit_frame_cache.  */

unsigned int get_frame_cache_generation ();

/* Mark that the PC value is masked for the previous frame.  */

extern void set_frame_previous_pc_masked (frame_info_ptr frame);

/* Get whether the PC value is masked for the given frame.  */

extern bool get_frame_pc_masked (frame_info_ptr frame);


#endif /* !defined (FRAME_H)  */
