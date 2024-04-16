extern int m(int x);
extern int n(int y);

namespace NameSpace {
  int overload ();
  int overload (int);
  int overload (double);
};

#ifdef WANT_F1
int f1(void)
{
  return 1;			/* f1 breakpoint */
}
#else
extern int f1(void);
#endif

#ifdef WANT_F2
int f2(void)
{
  return 1;			/* f2 breakpoint */
}
#else
extern int f2(void);
#endif
