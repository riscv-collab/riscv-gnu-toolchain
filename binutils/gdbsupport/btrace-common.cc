/* Copyright (C) 2014-2024 Free Software Foundation, Inc.

   Contributed by Intel Corp. <markus.t.metzger@intel.com>

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

#include "common-defs.h"
#include "btrace-common.h"


/* See btrace-common.h.  */

const char *
btrace_format_string (enum btrace_format format)
{
  switch (format)
    {
    case BTRACE_FORMAT_NONE:
      return _("No or unknown format");

    case BTRACE_FORMAT_BTS:
      return _("Branch Trace Store");

    case BTRACE_FORMAT_PT:
      return _("Intel Processor Trace");
    }

  internal_error (_("Unknown branch trace format"));
}

/* See btrace-common.h.  */

const char *
btrace_format_short_string (enum btrace_format format)
{
  switch (format)
    {
    case BTRACE_FORMAT_NONE:
      return "unknown";

    case BTRACE_FORMAT_BTS:
      return "bts";

    case BTRACE_FORMAT_PT:
      return "pt";
    }

  internal_error (_("Unknown branch trace format"));
}

/* See btrace-common.h.  */

void
btrace_data::fini ()
{
  switch (format)
    {
    case BTRACE_FORMAT_NONE:
      /* Nothing to do.  */
      return;

    case BTRACE_FORMAT_BTS:
      delete variant.bts.blocks;
      variant.bts.blocks = nullptr;
      return;

    case BTRACE_FORMAT_PT:
      xfree (variant.pt.data);
      return;
    }

  internal_error (_("Unkown branch trace format."));
}

/* See btrace-common.h.  */

bool
btrace_data::empty () const
{
  switch (format)
    {
    case BTRACE_FORMAT_NONE:
      return true;

    case BTRACE_FORMAT_BTS:
      return variant.bts.blocks->empty ();

    case BTRACE_FORMAT_PT:
      return (variant.pt.size == 0);
    }

  internal_error (_("Unkown branch trace format."));
}

/* See btrace-common.h.  */

void
btrace_data::clear ()
{
  fini ();
  format = BTRACE_FORMAT_NONE;
}

/* See btrace-common.h.  */

int
btrace_data_append (struct btrace_data *dst,
		    const struct btrace_data *src)
{
  switch (src->format)
    {
    case BTRACE_FORMAT_NONE:
      return 0;

    case BTRACE_FORMAT_BTS:
      switch (dst->format)
	{
	default:
	  return -1;

	case BTRACE_FORMAT_NONE:
	  dst->format = BTRACE_FORMAT_BTS;
	  dst->variant.bts.blocks = new std::vector<btrace_block>;
	  [[fallthrough]];
	case BTRACE_FORMAT_BTS:
	  {
	    unsigned int blk;

	    /* We copy blocks in reverse order to have the oldest block at
	       index zero.  */
	    blk = src->variant.bts.blocks->size ();
	    while (blk != 0)
	      {
		const btrace_block &block
		  = src->variant.bts.blocks->at (--blk);
		dst->variant.bts.blocks->push_back (block);
	      }
	  }
	}
      return 0;

    case BTRACE_FORMAT_PT:
      switch (dst->format)
	{
	default:
	  return -1;

	case BTRACE_FORMAT_NONE:
	  dst->format = BTRACE_FORMAT_PT;
	  dst->variant.pt.data = NULL;
	  dst->variant.pt.size = 0;
	  [[fallthrough]];
	case BTRACE_FORMAT_PT:
	  {
	    gdb_byte *data;
	    size_t size;

	    size = src->variant.pt.size + dst->variant.pt.size;
	    data = (gdb_byte *) xmalloc (size);

	    if (dst->variant.pt.size > 0)
	      memcpy (data, dst->variant.pt.data, dst->variant.pt.size);
	    memcpy (data + dst->variant.pt.size, src->variant.pt.data,
		    src->variant.pt.size);

	    xfree (dst->variant.pt.data);

	    dst->variant.pt.data = data;
	    dst->variant.pt.size = size;
	  }
	}
      return 0;
    }

  internal_error (_("Unkown branch trace format."));
}
