/* BFD support for KVX.
   Copyright (C) 2009-2024 Free Software Foundation, Inc.
   Contributed by Kalray SA.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3. If not,
   see <http://www.gnu.org/licenses/>.  */

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

/* This routine is provided two arch_infos and returns if machines
   are compatible.
*/

static const bfd_arch_info_type *
kvx_compatible (const bfd_arch_info_type *a, const bfd_arch_info_type *b)
{
  long amach =  a->mach, bmach =  b->mach;
  /* If a & b are for different architecture we can do nothing.  */
  if (a->arch != b->arch)
    return NULL;

  if ((amach == bfd_mach_kv3_1_64 && bmach == bfd_mach_kv3_1_usr)
      || (amach == bfd_mach_kv3_2_64 && bmach == bfd_mach_kv3_2_usr))
    return b;

  if ((bmach == bfd_mach_kv3_1_64 && amach == bfd_mach_kv3_1_usr)
      || (bmach == bfd_mach_kv3_2_64 && amach == bfd_mach_kv3_2_usr))
    return a;

  /* Otherwise if either a or b is the 'default' machine
   * then it can be polymorphed into the other.
   * This will enable to execute merge_private_bfd_data
   */
  if (a->the_default)
    return b;

  if (b->the_default)
    return a;

  /* We do not want to transmute some machine into another one */
  if (amach != bmach)
    return NULL;

  /* If a & b are for the same machine then all is well.  */
  if (amach == bmach)
    return a;

  return NULL;
}

static bool
scan (const struct bfd_arch_info *info, const char *string)
{
  /* First test for an exact match.  */
  if (strcasecmp (string, info->printable_name) == 0)
    return true;

  /* Finally check for the default architecture.  */
  if (strcasecmp (string, "kvx") == 0)
    return info->the_default;

  return false;
}

#define N(addr_bits, machine, print, default, next)            \
{                                                              \
  32,                          /* 32 bits in a word.  */       \
  addr_bits,                   /* bits in an address.  */      \
  8,                           /* 8 bits in a byte.  */        \
  bfd_arch_kvx,                                                 \
  machine,                     /* Machine number.  */          \
  "kvx",                        /* Architecture name.   */      \
  print,                       /* Printable name.  */          \
  4,                           /* Section align power.  */     \
  default,                     /* Is this the default ?  */    \
  kvx_compatible,					       \
  scan,                                                        \
  bfd_arch_default_fill,                                       \
  next,                                                \
          0                                            \
}


const bfd_arch_info_type bfd_kv4_1_usr_arch =
  N (64 , bfd_mach_kv4_1_usr , "kvx:kv4-1:usr" , false , NULL);

const bfd_arch_info_type bfd_kv3_2_usr_arch =
  N (64 , bfd_mach_kv3_2_usr , "kvx:kv3-2:usr" , false , &bfd_kv4_1_usr_arch);

const bfd_arch_info_type bfd_kv3_1_usr_arch =
  N (64 , bfd_mach_kv3_1_usr , "kvx:kv3-1:usr" , false , &bfd_kv3_2_usr_arch);

const bfd_arch_info_type bfd_kv4_1_64_arch =
  N (64 , bfd_mach_kv4_1_64  , "kvx:kv4-1:64"  , false , &bfd_kv3_1_usr_arch);

const bfd_arch_info_type bfd_kv3_2_64_arch =
  N (64 , bfd_mach_kv3_2_64  , "kvx:kv3-2:64"  , false , &bfd_kv4_1_64_arch);

const bfd_arch_info_type bfd_kv3_1_64_arch =
  N (64 , bfd_mach_kv3_1_64  , "kvx:kv3-1:64"  , false , &bfd_kv3_2_64_arch);

const bfd_arch_info_type bfd_kv4_1_arch =
  N (32 , bfd_mach_kv4_1     , "kvx:kv4-1"     , false , &bfd_kv3_1_64_arch);

const bfd_arch_info_type bfd_kv3_2_arch =
  N (32 , bfd_mach_kv3_2     , "kvx:kv3-2"     , false , &bfd_kv4_1_arch);

const bfd_arch_info_type bfd_kvx_arch =
  N (32 , bfd_mach_kv3_1     , "kvx:kv3-1"     , true  , &bfd_kv3_2_arch);
