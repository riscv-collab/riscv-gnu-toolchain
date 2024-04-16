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

#ifndef ARCH_TIC6X_H
#define ARCH_TIC6X_H

enum c6x_feature
{
  C6X_CORE,
  C6X_GP,
  C6X_C6XP,
  C6X_LAST,
};

target_desc *tic6x_create_target_description (enum c6x_feature feature);

#endif /* ARCH_TIC6X_H */
