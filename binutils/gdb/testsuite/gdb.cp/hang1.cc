#include "hang.H"

extern int dummy2 (void);
extern int dummy3 (void);

int main (int argc, char **argv) { return dummy2() + dummy3(); }
