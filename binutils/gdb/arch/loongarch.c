/* Copyright (C) 2022-2024 Free Software Foundation, Inc.

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
#include "loongarch.h"
#include <stdlib.h>
#include <unordered_map>

/* Target description features.  */

#include "../features/loongarch/base32.c"
#include "../features/loongarch/base64.c"
#include "../features/loongarch/fpu.c"

#ifndef GDBSERVER
#define STATIC_IN_GDB static
#else
#define STATIC_IN_GDB
#endif

STATIC_IN_GDB target_desc_up
loongarch_create_target_description (const struct loongarch_gdbarch_features features)
{
  /* Now we should create a new target description.  */
  target_desc_up tdesc = allocate_target_description ();

  std::string arch_name = "loongarch";

  if (features.xlen == 4)
    arch_name.append ("32");
  else if (features.xlen == 8)
    arch_name.append ("64");

  if (features.fputype == SINGLE_FLOAT)
    arch_name.append ("f");
  else if (features.fputype == DOUBLE_FLOAT)
    arch_name.append ("d");

  set_tdesc_architecture (tdesc.get (), arch_name.c_str ());

  long regnum = 0;

  /* For now we only support creating 32-bit or 64-bit x-registers.  */
  if (features.xlen == 4)
    regnum = create_feature_loongarch_base32 (tdesc.get (), regnum);
  else if (features.xlen == 8)
    regnum = create_feature_loongarch_base64 (tdesc.get (), regnum);

  /* For now we only support creating single float and double float.  */
  regnum = create_feature_loongarch_fpu (tdesc.get (), regnum);

  return tdesc;
}

#ifndef GDBSERVER

/* Wrapper used by std::unordered_map to generate hash for feature set.  */
struct loongarch_gdbarch_features_hasher
{
  std::size_t
  operator() (const loongarch_gdbarch_features &features) const noexcept
  {
    return features.hash ();
  }
};

/* Cache of previously seen target descriptions, indexed by the feature set
   that created them.  */
static std::unordered_map<loongarch_gdbarch_features,
			  const target_desc_up,
			  loongarch_gdbarch_features_hasher> loongarch_tdesc_cache;

const target_desc *
loongarch_lookup_target_description (const struct loongarch_gdbarch_features features)
{
  /* Lookup in the cache.  If we find it then return the pointer out of
     the target_desc_up (which is a unique_ptr).  This is safe as the
     loongarch_tdesc_cache will exist until GDB exits.  */
  const auto it = loongarch_tdesc_cache.find (features);
  if (it != loongarch_tdesc_cache.end ())
    return it->second.get ();

  target_desc_up tdesc (loongarch_create_target_description (features));

  /* Add to the cache, and return a pointer borrowed from the
     target_desc_up.  This is safe as the cache (and the pointers
     contained within it) are not deleted until GDB exits.  */
  target_desc *ptr = tdesc.get ();
  loongarch_tdesc_cache.emplace (features, std::move (tdesc));
  return ptr;
}

#endif /* !GDBSERVER */
