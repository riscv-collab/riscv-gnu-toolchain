/* This testcase is part of GDB, the GNU debugger.

   Copyright 2020-2024 Free Software Foundation, Inc.

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

#include <iostream>
#include <vector>

struct node {
  int id;
  node *left;
  node *right;
  bool visited;
};

node  node_array[50];
unsigned int CUR_IDX = 0;

node *
make_node (int val)
{
  node *new_node = &(node_array[CUR_IDX++]);
  new_node->left = NULL;
  new_node->right = NULL;
  new_node->id = val;
  new_node->visited = false;

  return new_node;
}

void
tree_insert (node *root, int val)
{
  if (val < root->id)
    {
      if (root->left)
        tree_insert (root->left, val);
      else
        root->left = make_node(val);
    }
  else if (val > root->id)
    {
      if (root->right)
        tree_insert (root->right, val);
      else
        root->right = make_node(val);
    }
}

void
inorder (node *root)
{
  std::vector<node *> todo;
  todo.push_back (root);
  while (!todo.empty())
    {
      node *curr = todo.back();
      todo.pop_back(); /* break-here */
      if (curr->visited)
        std::cout << curr->id << " ";
      else
        {
          curr->visited = true;
          if (curr->right)
            todo.push_back (curr->right);
          todo.push_back (curr);
          if (curr->left)
            todo.push_back (curr->left);
        }
    }
}

int
main (int argc, char **argv)
{
  node *root = make_node (35);

  tree_insert (root, 28);
  tree_insert (root, 20);
  tree_insert (root, 60);

  inorder (root);

  return 0;
}
