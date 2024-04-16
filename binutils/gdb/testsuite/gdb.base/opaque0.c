/*  Note that struct foo is opaque (never defined) in this file.  This
    is allowed by C since this file does not reference any members of
    the structure.  The debugger needs to be able to associate this
    opaque structure definition with the full definition in another
    file.
*/

struct foo *foop;
extern struct foo *getfoo ();
extern void putfoo (struct foo *foop);

int main ()
{
    foop = getfoo ();
    putfoo (foop);
    return 0;
}
