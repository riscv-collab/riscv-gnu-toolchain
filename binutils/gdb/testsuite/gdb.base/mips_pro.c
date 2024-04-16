/* Tests regarding examination of prologues.  */

int
inner (int z)
{
  return 2 * z;
}

int
middle (int x)
{
  if (x == 0)
    return inner (5);
  else
    return inner (6);
}

int
top (int y)
{
  return middle (y + 1);
}

int
main (int argc, char **argv)
{
  return top (-1) + top (1);
}
