/* Iterator of varobj.
   Copyright (C) 2013-2024 Free Software Foundation, Inc.

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

#ifndef VAROBJ_ITER_H
#define VAROBJ_ITER_H

/* A node or item of varobj, composed of the name and the value.  */

struct varobj_item
{
  /* Name of this item.  */
  std::string name;

  /* Value of this item.  */
  value_ref_ptr value;
};

/* A dynamic varobj iterator "class".  */

struct varobj_iter
{
public:

  virtual ~varobj_iter () = default;

  virtual std::unique_ptr<varobj_item> next () = 0;
};

#endif /* VAROBJ_ITER_H */
