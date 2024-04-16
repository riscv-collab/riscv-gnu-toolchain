/* Copyright (C) 2017-2024 Free Software Foundation, Inc.

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


#include "gdbsupport/common-defs.h"
#include "arc.h"
#include <stdlib.h>
#include <unordered_map>
#include <string>

/* Target description features.  */
#include "features/arc/v1-core.c"
#include "features/arc/v1-aux.c"
#include "features/arc/v2-core.c"
#include "features/arc/v2-aux.c"

#ifndef GDBSERVER
#define STATIC_IN_GDB static
#else
#define STATIC_IN_GDB
#endif

STATIC_IN_GDB target_desc_up
arc_create_target_description (const struct arc_arch_features &features)
{
  /* Create a new target description.  */
  target_desc_up tdesc = allocate_target_description ();

#ifndef IN_PROCESS_AGENT
  std::string arch_name;

  /* Architecture names here must match the ones in
     ARCH_INFO_STRUCT in bfd/cpu-arc.c.  */
  if (features.isa == ARC_ISA_ARCV1 && features.reg_size == 4)
      arch_name = "arc:ARC700";
  else if (features.isa == ARC_ISA_ARCV2 && features.reg_size == 4)
      arch_name = "arc:ARCv2";
  else
    {
      std::string msg = string_printf
	("Cannot determine architecture: ISA=%d; bitness=%d",
	 features.isa, 8 * features.reg_size);
      gdb_assert_not_reached ("%s", msg.c_str ());
    }

  set_tdesc_architecture (tdesc.get (), arch_name.c_str ());
#endif

  long regnum = 0;

  switch (features.isa)
    {
    case ARC_ISA_ARCV1:
      regnum = create_feature_arc_v1_core (tdesc.get (), regnum);
      regnum = create_feature_arc_v1_aux (tdesc.get (), regnum);
      break;
    case ARC_ISA_ARCV2:
      regnum = create_feature_arc_v2_core (tdesc.get (), regnum);
      regnum = create_feature_arc_v2_aux (tdesc.get (), regnum);
      break;
    default:
      std::string msg = string_printf
	("Cannot choose target description XML: %d", features.isa);
      gdb_assert_not_reached ("%s", msg.c_str ());
    }

  return tdesc;
}

#ifndef GDBSERVER

/* Wrapper used by std::unordered_map to generate hash for features set.  */
struct arc_arch_features_hasher
{
  std::size_t
  operator() (const arc_arch_features &features) const noexcept
  {
    return features.hash ();
  }
};

/* Cache of previously created target descriptions, indexed by the hash
   of the features set used to create them.  */
static std::unordered_map<arc_arch_features,
			  const target_desc_up,
			  arc_arch_features_hasher> arc_tdesc_cache;

/* See arch/arc.h.  */

const target_desc *
arc_lookup_target_description (const struct arc_arch_features &features)
{
  /* Lookup in the cache first.  If found, return the pointer from the
     "target_desc_up" type which is a "unique_ptr".  This should be fine
     as the "arc_tdesc_cache" will persist until GDB terminates.  */
  const auto it = arc_tdesc_cache.find (features);
  if (it != arc_tdesc_cache.end ())
    return it->second.get ();

  target_desc_up tdesc = arc_create_target_description (features);


  /* Add to the cache, and return a pointer borrowed from the
     target_desc_up.  This is safe as the cache (and the pointers
     contained within it) are not deleted until GDB exits.  */
  target_desc *ptr = tdesc.get ();
  arc_tdesc_cache.emplace (features, std::move (tdesc));
  return ptr;
}

#endif /* !GDBSERVER */
