/* Branch trace support for GDB, the GNU debugger.

   Copyright (C) 2013-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>.

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

#ifndef COMMON_BTRACE_COMMON_H
#define COMMON_BTRACE_COMMON_H

/* Branch tracing (btrace) is a per-thread control-flow execution trace of the
   inferior.  For presentation purposes, the branch trace is represented as a
   list of sequential control-flow blocks, one such list per thread.  */

/* A branch trace block.

   This represents a block of sequential control-flow.  Adjacent blocks will be
   connected via calls, returns, or jumps.  The latter can be direct or
   indirect, conditional or unconditional.  Branches can further be
   asynchronous, e.g. interrupts.  */
struct btrace_block
{
  /* The address of the first byte of the first instruction in the block.
     The address may be zero if we do not know the beginning of this block,
     such as for the first block in a delta trace.  */
  CORE_ADDR begin;

  /* The address of the first byte of the last instruction in the block.  */
  CORE_ADDR end;

  /* Simple constructor.  */
  btrace_block (CORE_ADDR begin, CORE_ADDR end)
    : begin (begin),
      end (end)
  {
    /* Nothing.  */
  }
};

/* Enumeration of btrace formats.  */

enum btrace_format
{
  /* No branch trace format.  */
  BTRACE_FORMAT_NONE,

  /* Branch trace is in Branch Trace Store (BTS) format.
     Actually, the format is a sequence of blocks derived from BTS.  */
  BTRACE_FORMAT_BTS,

  /* Branch trace is in Intel Processor Trace format.  */
  BTRACE_FORMAT_PT
};

/* An enumeration of cpu vendors.  */

enum btrace_cpu_vendor
{
  /* We do not know this vendor.  */
  CV_UNKNOWN,

  /* Intel.  */
  CV_INTEL,

  /* AMD.  */
  CV_AMD
};

/* A cpu identifier.  */

struct btrace_cpu
{
  /* The processor vendor.  */
  enum btrace_cpu_vendor vendor;

  /* The cpu family.  */
  unsigned short family;

  /* The cpu model.  */
  unsigned char model;

  /* The cpu stepping.  */
  unsigned char stepping;
};

/* A BTS configuration.  */

struct btrace_config_bts
{
  /* The size of the branch trace buffer in bytes.

     This is unsigned int and not size_t since it is registered as
     control variable for "set record btrace bts buffer-size".  */
  unsigned int size;
};

/* An Intel Processor Trace configuration.  */

struct btrace_config_pt
{
  /* The size of the branch trace buffer in bytes.

     This is unsigned int and not size_t since it is registered as
     control variable for "set record btrace pt buffer-size".  */
  unsigned int size;
};

/* A branch tracing configuration.

   This describes the requested configuration as well as the actually
   obtained configuration.
   We describe the configuration for all different formats so we can
   easily switch between formats.  */

struct btrace_config
{
  /* The branch tracing format.  */
  enum btrace_format format;

  /* The BTS format configuration.  */
  struct btrace_config_bts bts;

  /* The Intel Processor Trace format configuration.  */
  struct btrace_config_pt pt;
};

/* Branch trace in BTS format.  */
struct btrace_data_bts
{
  /* Branch trace is represented as a vector of branch trace blocks starting
     with the most recent block.  This needs to be a pointer as we place
     btrace_data_bts into a union.  */
  std::vector<btrace_block> *blocks;
};

/* Configuration information to go with the trace data.  */
struct btrace_data_pt_config
{
  /* The processor on which the trace has been collected.  */
  struct btrace_cpu cpu;
};

/* Branch trace in Intel Processor Trace format.  */
struct btrace_data_pt
{
  /* Some configuration information to go with the data.  */
  struct btrace_data_pt_config config;

  /* The trace data.  */
  gdb_byte *data;

  /* The size of DATA in bytes.  */
  size_t size;
};

/* The branch trace data.  */
struct btrace_data
{
  btrace_data () = default;

  ~btrace_data ()
  {
    fini ();
  }

  btrace_data &operator= (btrace_data &&other)
  {
    if (this != &other)
      {
	fini ();
	format = other.format;
	variant = other.variant;
	other.format = BTRACE_FORMAT_NONE;
      }
    return *this;
  }

  /* Return true if this is empty; false otherwise.  */
  bool empty () const;

  /* Clear this object.  */
  void clear ();

  enum btrace_format format = BTRACE_FORMAT_NONE;

  union
  {
    /* Format == BTRACE_FORMAT_BTS.  */
    struct btrace_data_bts bts;

    /* Format == BTRACE_FORMAT_PT.  */
    struct btrace_data_pt pt;
  } variant;

private:

  DISABLE_COPY_AND_ASSIGN (btrace_data);

  void fini ();
};

/* Target specific branch trace information.  */
struct btrace_target_info
{
  btrace_target_info (ptid_t ptid) : ptid (ptid)
    {}

  btrace_target_info (ptid_t ptid, btrace_config conf)
    : ptid (ptid), conf (conf)
    {}

  virtual ~btrace_target_info () = default;

  /* The ptid of this thread.  */
  ptid_t ptid {};

  /* The obtained branch trace configuration.  */
  btrace_config conf {};
};

/* Enumeration of btrace read types.  */

enum btrace_read_type
{
  /* Send all available trace.  */
  BTRACE_READ_ALL,

  /* Send all available trace, if it changed.  */
  BTRACE_READ_NEW,

  /* Send the trace since the last request.  This will fail if the trace
     buffer overflowed.  */
  BTRACE_READ_DELTA
};

/* Enumeration of btrace errors.  */

enum btrace_error
{
  /* No error.  Everything is OK.  */
  BTRACE_ERR_NONE,

  /* An unknown error.  */
  BTRACE_ERR_UNKNOWN,

  /* Branch tracing is not supported on this system.  */
  BTRACE_ERR_NOT_SUPPORTED,

  /* The branch trace buffer overflowed; no delta read possible.  */
  BTRACE_ERR_OVERFLOW
};

/* Return a string representation of FORMAT.  */
extern const char *btrace_format_string (enum btrace_format format);

/* Return an abbreviation string representation of FORMAT.  */
extern const char *btrace_format_short_string (enum btrace_format format);

/* Append the branch trace data from SRC to the end of DST.
   Both SRC and DST must use the same format.
   Returns zero on success; a negative number otherwise.  */
extern int btrace_data_append (struct btrace_data *dst,
			       const struct btrace_data *src);

#endif /* COMMON_BTRACE_COMMON_H */
