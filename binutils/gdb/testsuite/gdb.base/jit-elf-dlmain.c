#include <dlfcn.h>
#include <stdio.h>

int main (int argc, char *argv[])
{
  /* jit_libname is updated by jit-so.exp  */
  const char *jit_libname = "jit-dlmain-so.so";
  void *h;
  int (*p_main) (int, char **);

  h = NULL;  /* break here before-dlopen  */
  h = dlopen (jit_libname, RTLD_LAZY);
  if (h == NULL) return 1;

  p_main = dlsym (h, "jit_dl_main");
  if (p_main == NULL) return 2;

  h = h;  /* break here after-dlopen */
  return (*p_main) (argc, argv);
}
