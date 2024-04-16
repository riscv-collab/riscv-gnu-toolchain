/* GNU/Linux/aarch64 specific target description, for the remote server
   for GDB.
   Copyright (C) 2017-2024 Free Software Foundation, Inc.

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

#include "server.h"

#include "linux-aarch64-tdesc.h"

#include "tdesc.h"
#include "arch/aarch64.h"
#include "linux-aarch32-low.h"
#include <inttypes.h>
#include <unordered_map>

/* All possible aarch64 target descriptors.  */
static std::unordered_map<aarch64_features, target_desc *> tdesc_aarch64_map;

static std::vector<const char *> expedited_registers;

/* Create the aarch64 target description.  */

const target_desc *
aarch64_linux_read_description (const aarch64_features &features)
{
  if (features.vq > AARCH64_MAX_SVE_VQ)
    error (_("VQ is %" PRIu64 ", maximum supported value is %d"), features.vq,
	   AARCH64_MAX_SVE_VQ);

  if (features.svq > AARCH64_MAX_SVE_VQ)
    error (_("Streaming svq is %" PRIu8 ", maximum supported value is %d"),
	   features.svq,
	   AARCH64_MAX_SVE_VQ);

  struct target_desc *tdesc = tdesc_aarch64_map[features];

  if (tdesc == NULL)
    {
      tdesc = aarch64_create_target_description (features);
      expedited_registers.clear ();

      /* Configure the expedited registers.  By default we include x29, sp and
	 pc.  */
      expedited_registers.push_back ("x29");
      expedited_registers.push_back ("sp");
      expedited_registers.push_back ("pc");

      if (features.vq > 0)
	expedited_registers.push_back ("vg");
      if (features.svq > 0)
	expedited_registers.push_back ("svg");

      expedited_registers.push_back (nullptr);

      init_target_desc (tdesc, (const char **) expedited_registers.data ());

      tdesc_aarch64_map[features] = tdesc;
    }

  return tdesc;
}
