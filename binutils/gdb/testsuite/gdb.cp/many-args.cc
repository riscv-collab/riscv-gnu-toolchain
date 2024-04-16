/* Copyright 2019-2024 Free Software Foundation, Inc.

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

#include <string.h>

/* Vector type will align on a 16-byte boundary.  */
typedef int v4si __attribute__ ((vector_size (16)));

struct ss
{
  static v4si static_field;

  unsigned char aa;

  bool operator== (const ss &rhs)
  {
    return (memcmp (&this->static_field, &rhs.static_field,
		    sizeof (this->static_field)) == 0
            && this->aa == rhs.aa);
  }
};

v4si ss::static_field = { 1, 2, 3, 4 };

ss ref_val = { 'a' };

bool
check_val (ss v1, ss v2, ss v3, ss v4, ss v5, ss v6, ss v7, ss v8,
           ss v9, ss v10, ss v11, ss v12, ss v13, ss v14, ss v15,
           ss v16, ss v17, ss v18, ss v19, ss v20, ss v21, ss v22,
           ss v23, ss v24, ss v25, ss v26, ss v27, ss v28, ss v29,
           ss v30, ss v31, ss v32, ss v33, ss v34, ss v35, ss v36,
           ss v37, ss v38, ss v39, ss v40)
{
  return (v1 == ref_val && v2 == ref_val && v3 == ref_val && v4 == ref_val
          && v5 == ref_val && v6 == ref_val && v7 == ref_val
          && v8 == ref_val && v9 == ref_val && v10 == ref_val
          && v11 == ref_val && v12 == ref_val && v13 == ref_val
          && v14 == ref_val && v15 == ref_val && v16 == ref_val
          && v17 == ref_val && v18 == ref_val && v19 == ref_val
          && v20 == ref_val && v21 == ref_val && v22 == ref_val
          && v23 == ref_val && v24 == ref_val && v25 == ref_val
          && v26 == ref_val && v27 == ref_val && v28 == ref_val
          && v29 == ref_val && v30 == ref_val && v31 == ref_val
          && v32 == ref_val && v33 == ref_val && v34 == ref_val
          && v35 == ref_val && v36 == ref_val && v37 == ref_val
          && v38 == ref_val && v39 == ref_val && v40 == ref_val);
}

int
main ()
{
  bool flag = check_val (ref_val, ref_val, ref_val, ref_val, ref_val,
			 ref_val, ref_val, ref_val, ref_val, ref_val,
			 ref_val, ref_val, ref_val, ref_val, ref_val,
			 ref_val, ref_val, ref_val, ref_val, ref_val,
			 ref_val, ref_val, ref_val, ref_val, ref_val,
			 ref_val, ref_val, ref_val, ref_val, ref_val,
			 ref_val, ref_val, ref_val, ref_val, ref_val,
			 ref_val, ref_val, ref_val, ref_val, ref_val);
  return (flag ? 0 : 1);	/* break-here */
}
