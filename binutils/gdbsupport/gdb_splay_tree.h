/* GDB wrapper for splay trees.

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

#ifndef COMMON_GDB_SPLAY_TREE_H
#define COMMON_GDB_SPLAY_TREE_H

#include "splay-tree.h"

namespace gdb {

struct splay_tree_deleter
{
  void operator() (splay_tree tree) const
  {
    splay_tree_delete (tree);
  }
};

} /* namespace gdb */

/* A unique pointer to a splay tree.  */

typedef std::unique_ptr<splay_tree_s, gdb::splay_tree_deleter>
    gdb_splay_tree_up;

#endif /* COMMON_GDB_SPLAY_TREE_H */
