#if defined(__cplusplus) || defined(__STDCPP__)
extern "C"
#endif
int
solib_main (int arg)
{
  int ans = arg*arg;		/* HERE */
  return ans;			/* STEP */
}
