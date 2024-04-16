/* Used by the test harness to see if toolchain targets Linux.  */
#ifdef __linux__
int main()
{
  return 0;
}
#else
# error "not linux"
#endif
