typedef unsigned long long uint64_t;
void abort (void);

#define BASE 0x1234567812345678ull

#define DEF_BBIT_TAKEN(BRANCH_IF, BIT)					\
  int bbit_is_taken_##BRANCH_IF##_##BIT (volatile uint64_t *p)		\
  {									\
    int ret;								\
    asm (".set push				\n\t"			\
	 ".set noreorder			\n\t"			\
	 "bbit" #BRANCH_IF " %1, " #BIT ", 1f	\n\t"			\
	 "nop					\n\t"			\
	 "li %0, 0				\n\t"			\
	 "b 2f					\n\t"			\
	 "nop					\n\t"			\
	 "1:					\n\t"			\
	 "li %0, 1				\n\t"			\
	 "2:					\n\t"			\
	 ".set pop"							\
	 : "=r"(ret) : "r"(*p));					\
    return ret;								\
  }									\
  volatile uint64_t taken_##BRANCH_IF##_##BIT =				\
    BASE & (~(1ull << BIT)) | ((uint64_t) BRANCH_IF << BIT);		\
  volatile uint64_t not_taken_##BRANCH_IF##_##BIT =			\
    BASE & (~(1ull << BIT)) | (((uint64_t) !BRANCH_IF) << BIT);  

DEF_BBIT_TAKEN (0, 10);
DEF_BBIT_TAKEN (0, 36);
DEF_BBIT_TAKEN (1, 20);
DEF_BBIT_TAKEN (1, 49);

#define EXPECT(X) if (!(X)) abort ();

main ()
{
  EXPECT (bbit_is_taken_0_10 (&taken_0_10));
  EXPECT (!bbit_is_taken_0_10 (&not_taken_0_10));

  EXPECT (bbit_is_taken_0_36 (&taken_0_36));
  EXPECT (!bbit_is_taken_0_36 (&not_taken_0_36));

  EXPECT (bbit_is_taken_1_20 (&taken_1_20));
  EXPECT (!bbit_is_taken_1_20 (&not_taken_1_20));

  EXPECT (bbit_is_taken_1_49 (&taken_1_49));
  EXPECT (!bbit_is_taken_1_49 (&not_taken_1_49));
}
