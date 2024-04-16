/* This file is actually in C, it is not supposed to simulate something
   translated from another language or anything like that.  */
extern  int fsub_();

int csub (int x)
{
  return x + 1;
}

int
langs0__2do ()
{
  return fsub_ () + 2;
}

int
main ()
{
  if (langs0__2do () == 5003)
    /* Success.  */
    return 0;
  else
    return 1;
}
