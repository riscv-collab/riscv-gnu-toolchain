/* Make sure that error returns are correct.  Usually this is trivially
   true, but on platforms with unusual type sizes all the casting might
   cause problems with unexpected sign-extension and truncation.  */

#include <ctf-api.h>
#include <stdio.h>
#include <stdlib.h>

int
main (int argc, char *argv[])
{
  ctf_dict_t *fp;
  ctf_next_t *i = NULL;
  size_t boom = 0;
  ctf_id_t itype, stype;
  ctf_encoding_t encoding = {0};
  ctf_membinfo_t mi;
  ssize_t ret;
  int err;

  if ((fp = ctf_create (&err)) == NULL)
    {
      fprintf (stderr, "%s: cannot create: %s\n", argv[0], ctf_errmsg (err));
      return 1;
    }

  /* First error class: int return.  */

  if (ctf_member_count (fp, 1024) >= 0)
    fprintf (stderr, "int return: non-error return: %i\n",
             ctf_member_count(fp, 1024));

  /* Second error class: type ID return.  */

  if (ctf_type_reference (fp, 1024) != CTF_ERR)
    fprintf (stderr, "ctf_id_t return: non-error return: %li\n",
             ctf_type_reference (fp, 1024));

  /* Third error class: ssize_t return.  Create a type to iterate over first.  */

  if ((itype = ctf_add_integer (fp, CTF_ADD_ROOT, "int", &encoding)) == CTF_ERR)
    fprintf (stderr, "cannot add int: %s\n", ctf_errmsg (ctf_errno (fp)));
  else if ((stype = ctf_add_struct (fp, CTF_ADD_ROOT, "foo")) == CTF_ERR)
    fprintf (stderr, "cannot add struct: %s\n", ctf_errmsg (ctf_errno (fp)));
  else if (ctf_add_member (fp, stype, "bar", itype) < 0)
    fprintf (stderr, "cannot add member: %s\n", ctf_errmsg (ctf_errno (fp)));

  if (ctf_member_info (fp, stype, "bar", &mi) < 0)
    fprintf (stderr, "cannot get member info: %s\n", ctf_errmsg (ctf_errno (fp)));

  /* Iteration should never produce an offset bigger than the offset just returned,
     and should quickly terminate.  */

  while ((ret = ctf_member_next (fp, stype, &i, NULL, NULL, 0)) >= 0) {
    if (ret > mi.ctm_offset)
      fprintf (stderr, "ssize_t return: unexpected offset: %zi\n", ret);
    if (boom++ > 1000)
      {
        fprintf (stderr, "member iteration went on way too long\n");
        exit (1);
      }
  }

  /* Fourth error class (trivial): pointer return.  */
  if (ctf_type_aname (fp, 1024) != NULL)
    fprintf (stderr, "pointer return: non-error return: %p\n",
             ctf_type_aname (fp, 1024));

  ctf_file_close (fp);

  printf("All done.\n");

  return 0;
}
