/* This testcase is part of GDB, the GNU debugger.

   Copyright 2019-2024 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see  <http://www.gnu.org/licenses/>.  */

#include <stdlib.h>
#include <string.h>

#define FIXED_MAP_SIZE 10

struct my_key_t
{
  int a;
  int b;
};

struct my_value_t
{
  int x;
  int y;
  int z;
};

struct map_t
{
  const char *name;
  int length;
  struct my_key_t *keys;
  struct my_value_t *values;

  /* This field is used only by the pretty printer.  */
  int show_header;
};

struct map_map_t
{
  int length;
  struct map_t **values;

  /* This field is used only by the pretty printer.  */
  int show_header;
};

struct map_t *
create_map (const char *name)
{
  struct map_t *m = (struct map_t *) malloc (sizeof (struct map_t));
  m->name = strdup (name);
  m->length = 0;
  m->keys = NULL;
  m->values = NULL;
  m->show_header = 0;
  return m;
}

void
add_map_element (struct map_t *m, struct my_key_t k, struct my_value_t v)
{
  if (m->length == 0)
    {
      m->keys = (struct my_key_t *) malloc (sizeof (struct my_key_t) * FIXED_MAP_SIZE);
      m->values = (struct my_value_t *) malloc (sizeof (struct my_value_t) * FIXED_MAP_SIZE);
    }

  m->keys[m->length] = k;
  m->values[m->length] = v;
  m->length++;
}

struct map_map_t *
create_map_map (void)
{
  struct map_map_t *mm = (struct map_map_t *) malloc (sizeof (struct map_map_t));
  mm->length = 0;
  mm->values = NULL;
  mm->show_header = 0;
  return mm;
}

void
add_map_map_element (struct map_map_t *mm, struct map_t *map)
{
  if (mm->length == 0)
    mm->values = (struct map_t **) malloc (sizeof (struct map_t *) * FIXED_MAP_SIZE);

  mm->values[mm->length] = map;
  mm->length++;
}

int
main (void)
{
  struct map_t *m1 = create_map ("m1");
  struct my_key_t k1 = {3, 4};
  struct my_key_t k2 = {4, 5};
  struct my_key_t k3 = {5, 6};
  struct my_key_t k4 = {6, 7};
  struct my_key_t k5 = {7, 8};
  struct my_key_t k6 = {8, 9};
  struct my_value_t v1 = {0, 1, 2};
  struct my_value_t v2 = {3, 4, 5};
  struct my_value_t v3 = {6, 7, 8};
  struct my_value_t v4 = {9, 0, 1};
  struct my_value_t v5 = {2, 3, 4};
  struct my_value_t v6 = {5, 6, 7};
  add_map_element (m1, k1, v1);
  add_map_element (m1, k2, v2);
  add_map_element (m1, k3, v3);

  struct map_t *m2 = create_map ("m2");
  add_map_element (m2, k4, v4);
  add_map_element (m2, k5, v5);
  add_map_element (m2, k6, v6);

  struct map_map_t *mm = create_map_map ();
  add_map_map_element (mm, m1);
  add_map_map_element (mm, m2);

  return 0; /* Break here.  */
}
