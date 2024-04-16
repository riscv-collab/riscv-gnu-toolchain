/* Make sure we do various things right that involve DTD lookups of parents
   from the perspective of children.  */

#include <ctf-api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum crash_method { ADD_STRUCT, ADD_UNION, ADD_MEMBER_OFFSET, ADD_MEMBER_ENCODED, ADD_ENUM, ADD_ENUMERATOR, SET_ARRAY };

void
dtd_crash (enum crash_method method, int parent_bigger)
{
  ctf_dict_t *pfp, *cfp;
  ctf_encoding_t e = { CTF_INT_SIGNED, 0, sizeof (long) };
  ctf_id_t ptype, ftype, stype, foo;
  int forward_kind = CTF_K_STRUCT;
  size_t i;
  int err;

  /* Maybe make the relevant type IDs in the parent much bigger than those
     in the child, or maybe vice versa.  */

  if ((pfp = ctf_create (&err)) == NULL)
    goto create_err;

  if (parent_bigger)
    {
      if ((foo = ctf_add_integer (pfp, CTF_ADD_NONROOT, "blah", &e)) == CTF_ERR)
	goto create_parent;

      for (i = 0; i < 4096; i++)
	if (ctf_add_pointer (pfp, CTF_ADD_NONROOT, foo) == CTF_ERR)
	  goto create_parent;
    }

  if ((ptype = ctf_add_integer (pfp, CTF_ADD_NONROOT, "int", &e)) == CTF_ERR)
    goto create_parent;

  /* Add a forward to a struct, union, or enum (depending on the method) in
     the parent, so we can try to replace it in the child and see what
     happens.  (Most of them are structs, or it doesn't matter, as for
     SET_ARRAY; so we do that by default.)  */

  switch (method)
    {
    case ADD_UNION:
      forward_kind = CTF_K_UNION;
      break;
    case ADD_ENUM:
    case ADD_ENUMERATOR:
      forward_kind = CTF_K_ENUM;
      break;
      /* Placate clang.  */
    default:
      break;
    }

  if ((ftype = ctf_add_forward (pfp, CTF_ADD_ROOT, "foo", forward_kind)) == CTF_ERR)
    goto create_parent;

  if ((cfp = ctf_create (&err)) == NULL)
    goto create_err;

  if (ctf_import (cfp, pfp) < 0)
    goto create_child;

  if (!parent_bigger)
    {
      if ((foo = ctf_add_integer (pfp, CTF_ADD_NONROOT, "blah", &e)) == CTF_ERR)
	goto create_parent;

      for (i = 0; i < 4096; i++)
	if (ctf_add_pointer (pfp, CTF_ADD_NONROOT, foo) == CTF_ERR)
	  goto create_parent;
    }

  switch (method)
    {
      /* These try to replace a forward, and should not do so if we're
	 adding in the child and it's in the parent.  */
    case ADD_STRUCT:
      if ((stype = ctf_add_struct_sized (cfp, CTF_ADD_ROOT, "foo", 1024)) == CTF_ERR)
	goto create_child;
      if (stype == ftype)
	fprintf (stderr, "Forward-promotion spotted!\n");
      break;

    case ADD_UNION:
      if ((stype = ctf_add_union_sized (cfp, CTF_ADD_ROOT, "foo", 1024)) == CTF_ERR)
	goto create_child;
      if (stype == ftype)
	fprintf (stderr, "Forward-promotion spotted!\n");
      break;

    case ADD_ENUM:
      if ((stype = ctf_add_enum (cfp, CTF_ADD_ROOT, "foo")) == CTF_ERR)
	goto create_child;
      if (stype == ftype)
	fprintf (stderr, "Forward-promotion spotted!\n");
      break;

      /* These try to look up the struct/union/enum we're adding to: make
	 sure this works from the perspective of the child if the type is in
	 the parent.  Also make sure that addition of child types to parent
	 types this way is prohibited, and that addition of parent types to
	 parent types is allowed.  */
    case ADD_MEMBER_OFFSET:
      {
	ctf_id_t ctype;

	if ((stype = ctf_add_struct (pfp, CTF_ADD_ROOT, "bar")) == CTF_ERR)
	  goto create_child;

	if ((ctype = ctf_add_integer (cfp, CTF_ADD_NONROOT, "xyzzy", &e)) == CTF_ERR)
	  goto create_child;

	if (ctf_add_member_offset (cfp, stype, "member", ptype, 5) == CTF_ERR)
	  goto create_child;

	if (ctf_add_member_offset (cfp, stype, "xyzzy", ctype, 4) != CTF_ERR)
	  fprintf (stderr, "Addition of child type to parent via child unexpectedly succeeded\n");
	else if (ctf_errno (cfp) == 0)
	  fprintf (stderr, "got error from ctype addition to parent struct, but no error found on child\n");

	break;
      }

    case ADD_ENUMERATOR:
      if ((stype = ctf_add_enum (pfp, CTF_ADD_ROOT, "bar")) == CTF_ERR)
	goto create_parent;

      if (ctf_add_enumerator (cfp, stype, "FOO", 0) == CTF_ERR)
	goto create_child;
      break;

      /* This tries to look up the member type we're adding, and goes wrong
	 if the struct is in the child and the member type is in the parent.  */
    case ADD_MEMBER_ENCODED:
      if ((stype = ctf_add_struct (cfp, CTF_ADD_ROOT, "foo")) == CTF_ERR)
	goto create_child;

      if (ctf_add_member_encoded (cfp, stype, "cmember", ptype, 5, e) == CTF_ERR)
	goto create_child;
      break;

      /* This tries to look up the array we're resetting the state of.  */
    case SET_ARRAY:
      {
	ctf_arinfo_t ar;

	ar.ctr_contents = ptype;
	ar.ctr_index = ptype;
	ar.ctr_nelems = 5;

	if ((stype = ctf_add_array (pfp, CTF_ADD_ROOT, &ar)) == CTF_ERR)
	  goto create_child;

	if (ctf_set_array (cfp, stype, &ar) == CTF_ERR)
	  goto create_child;
	break;
      }
    }

  ctf_dict_close (cfp);
  ctf_dict_close (pfp);

  return;

 create_err:
  fprintf (stderr, "Creation failed: %s\n", ctf_errmsg (err));
  exit (1);
 create_parent:
  fprintf (stderr, "Cannot create parent type: %s\n", ctf_errmsg (ctf_errno (pfp)));
  exit (1);
 create_child:
  fprintf (stderr, "Cannot create child type: %s\n", ctf_errmsg (ctf_errno (cfp)));
  exit (1);
}
