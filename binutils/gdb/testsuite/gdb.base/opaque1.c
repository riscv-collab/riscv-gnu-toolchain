struct foo {
    int a;
    int b;
} afoo = { 1, 2};

struct foo *getfoo ()
{
    return (&afoo);
}

void putfoo (struct foo *foop)
{
}
