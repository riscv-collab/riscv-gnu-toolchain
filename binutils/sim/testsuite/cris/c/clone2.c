/* Make sure the thread system trivially works with trace output.
#progos: linux
#sim: --cris-trace=basic --trace-file=$pwd/clone2.tmp
#output: got: a\nthen: bc\nexit: 0\n
*/
#include "clone1.c"
