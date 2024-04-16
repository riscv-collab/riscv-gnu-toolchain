/* Check that invoking ld.so as a program, invoking the main program,
   works.  Jump through a few hoops to avoid reading the host
   ld.so.cache (having no absolute path specified for the executable
   falls back on loading through the same mechanisms as a DSO).
#progos: linux
#sim: --sysroot=$pwd $pwd/lib/ld.so.1 --library-path /
 */
#include "hello.c"
