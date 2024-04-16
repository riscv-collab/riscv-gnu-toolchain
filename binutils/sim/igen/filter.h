/* The IGEN simulator generator for GDB, the GNU Debugger.

   Copyright 2002-2024 Free Software Foundation, Inc.

   Contributed by Andrew Cagney.

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

#ifndef IGEN_FILTER_H
#define IGEN_FILTER_H

#include "lf.h"

/* NB, an empty filter is NULL */
typedef struct _filter filter;


/* parse the list merging any flags into the filter */

extern void filter_parse (filter **filters, const char *filt);


/* add the second filter to the first */

extern void filter_add (filter **filters, const filter *add);



/* returns true if SUB is a strict subset of SUPER.  For an empty set
   is a member of any set */

extern int filter_is_subset (const filter *superset, const filter *subset);


/* return true if there is at least one member common to the two
   filters */

extern int filter_is_common (const filter *l, const filter *r);


/* returns the index (pos + 1) if the name is in the filter.  */

extern int filter_is_member (const filter *set, const char *flag);


/* returns true if one of the flags is not present in the filter.
   === !filter_is_subset (filter_parse (NULL, flags), filters) */
int is_filtered_out (const filter *filters, const char *flags);


/* returns the next member of the filter set that follows MEMBER.
   Member does not need to be an elememt of the filter set.  Next of
   "" is the first non-empty member */
const char *filter_next (const filter *set, const char *member);



/* for debugging */

extern void dump_filter
  (lf *file, const char *prefix, const filter *filt, const char *suffix);

#endif /* IGEN_FILTER_H */
