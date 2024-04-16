char array[4];

int
call_me (int *arg)
{
  return (*arg) - 1;
}

int val = 1;

int
main ()
{
  return call_me (&val);
}
