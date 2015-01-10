/* This kludge works around a libpthread static linking problem:
   https://sourceware.org/bugzilla/show_bug.cgi?id=15648 */

#ifndef SHARED
# define __lll_lock_wait_private weak_function __lll_lock_wait_private
#endif

#include <lowlevellock.c>
