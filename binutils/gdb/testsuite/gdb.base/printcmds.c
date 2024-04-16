/* This table is used as a source for every ascii character.
   It is explicitly unsigned to avoid differences due to native characters
   being either signed or unsigned. */
#include <stdlib.h>
unsigned char ctable1[256] = {
  0000, 0001, 0002, 0003, 0004, 0005, 0006, 0007,
  0010, 0011, 0012, 0013, 0014, 0015, 0016, 0017,
  0020, 0021, 0022, 0023, 0024, 0025, 0026, 0027,
  0030, 0031, 0032, 0033, 0034, 0035, 0036, 0037,
  0040, 0041, 0042, 0043, 0044, 0045, 0046, 0047,
  0050, 0051, 0052, 0053, 0054, 0055, 0056, 0057,
  0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067,
  0070, 0071, 0072, 0073, 0074, 0075, 0076, 0077,
  0100, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
  0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
  0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127,
  0130, 0131, 0132, 0133, 0134, 0135, 0136, 0137,
  0140, 0141, 0142, 0143, 0144, 0145, 0146, 0147,
  0150, 0151, 0152, 0153, 0154, 0155, 0156, 0157,
  0160, 0161, 0162, 0163, 0164, 0165, 0166, 0167,
  0170, 0171, 0172, 0173, 0174, 0175, 0176, 0177,
  0200, 0201, 0202, 0203, 0204, 0205, 0206, 0207,
  0210, 0211, 0212, 0213, 0214, 0215, 0216, 0217,
  0220, 0221, 0222, 0223, 0224, 0225, 0226, 0227,
  0230, 0231, 0232, 0233, 0234, 0235, 0236, 0237,
  0240, 0241, 0242, 0243, 0244, 0245, 0246, 0247,
  0250, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
  0260, 0261, 0262, 0263, 0264, 0265, 0266, 0267,
  0270, 0271, 0272, 0273, 0274, 0275, 0276, 0277,
  0300, 0301, 0302, 0303, 0304, 0305, 0306, 0307,
  0310, 0311, 0312, 0313, 0314, 0315, 0316, 0317,
  0320, 0321, 0322, 0323, 0324, 0325, 0326, 0327,
  0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
  0340, 0341, 0342, 0343, 0344, 0345, 0346, 0347,
  0350, 0351, 0352, 0353, 0354, 0355, 0356, 0357,
  0360, 0361, 0362, 0363, 0364, 0365, 0366, 0367,
  0370, 0371, 0372, 0373, 0374, 0375, 0376, 0377
};

unsigned char ctable2[] = {
  'a','X','X','X','X','X','X','X','X','X','X','X','X','X','X','X',
  'a','a','X','X','X','X','X','X','X','X','X','X','X','X','X','X',
  'a','a','a','X','X','X','X','X','X','X','X','X','X','X','X','X',
  'a','a','a','a','X','X','X','X','X','X','X','X','X','X','X','X',
  'a','a','a','a','a','X','X','X','X','X','X','X','X','X','X','X',
  'a','a','a','a','a','a','X','X','X','X','X','X','X','X','X','X',
  'a','a','a','a','a','a','a','X','X','X','X','X','X','X','X','X',
  'a','a','a','a','a','a','a','a','X','X','X','X','X','X','X','X',
  'a','a','a','a','a','a','a','a','a','X','X','X','X','X','X','X',
  'a','a','a','a','a','a','a','a','a','a','X','X','X','X','X','X',
  'a','a','a','a','a','a','a','a','a','a','a','X','X','X','X','X',
  'a','a','a','a','a','a','a','a','a','a','a','a','X','X','X','X',
  'a','a','a','a','a','a','a','a','a','a','a','a','a','X','X','X',
  'a','a','a','a','a','a','a','a','a','a','a','a','a','a','X','X',
  'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','X',
  'a','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a', 0
};

/* Single and multidimensional arrays to test access and printing of array
   members. */

typedef int ArrayInt [10];
ArrayInt a1 = {2,4,6,8,10,12,14,16,18,20};

typedef char ArrayChar [5];
ArrayChar a2 = {'a','b','c','d','\0'};

int int1dim[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
int int2dim[3][4] = {{0,1,2,3},{4,5,6,7},{8,9,10,11}};
int int3dim[2][3][2] = {{{0,1},{2,3},{4,5}},{{6,7},{8,9},{10,11}}};
int int4dim[1][2][3][2] = {{{{0,1},{2,3},{4,5}},{{6,7},{8,9},{10,11}}}};

char *teststring = (char*)"teststring contents";

typedef char *charptr;
charptr teststring2 = "more contents";

const char *teststring3 = "this is a longer test string that we can use";

/* Test printing of a struct containing character arrays. */

struct some_arrays {
    unsigned char array1[4];
    unsigned char array2[1];
    unsigned char array3[1];
    unsigned char array4[2];
    unsigned char array5[4];
} arrays = {
  {'a', 'b', 'c', '\0'},
  {'d'},
  {'e'},
  {'f', 'g' },
  {'h', 'i', 'j', '\0'}
};

struct some_arrays *parrays = &arrays;

enum some_volatile_enum { enumvolval1, enumvolval2 };

/* A volatile enum variable whose name is the same as the enumeration
   name.  See PR11827.  */
volatile enum some_volatile_enum some_volatile_enum = enumvolval1;

/* An enum considered as a "flag enum".  */
enum flag_enum
{
  FE_NONE       = 0x00,
  FE_ONE        = 0x01,
  FE_TWO        = 0x02,
  FE_TWO_LEGACY = 0x02,
};

enum flag_enum one = FE_ONE;
enum flag_enum three = (enum flag_enum) (FE_ONE | FE_TWO);

/* Another enum considered as a "flag enum", but with no enumerator with value
   0.  */
enum flag_enum_without_zero
{
  FEWZ_ONE = 0x01,
  FEWZ_TWO = 0x02,
};

enum flag_enum_without_zero flag_enum_without_zero = (enum flag_enum_without_zero) 0;

/* Not a flag enum, an enumerator value has multiple bits sets.  */
enum not_flag_enum
{
  NFE_ONE = 0x01,
  NFE_TWO = 0x02,
  NFE_F0  = 0xf0,
};

enum not_flag_enum three_not_flag = (enum not_flag_enum) (NFE_ONE | NFE_TWO);

/* A structure with an embedded array at an offset > 0.  The array has
   all elements with the same repeating value, which must not be the
   same as the value of the preceding fields in the structure for the
   test to be effective.  This tests whether GDB uses the correct
   element content offsets (relative to the complete `some_struct'
   value) when counting value repetitions.  */
struct some_struct
{
  int a;
  int b;
  unsigned char array[20];
} some_struct = {
  0x12345678,
  0x87654321,
  {
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa
  }
};

/* This is used in the printf test.  */
struct small_struct
{
  int a;
  int b;
  int c;
} a_small_struct = {
  1,
  2,
  3
};

/* The following variables are used for testing byte repeat sequences.
   The variable names are encoded: invalid_XYZ where:
   X = start
   Y = invalid
   Z = end

   Each of X and Z can be "E" (empty), "S" (single), "L" (long single),
   or "R" (repeat).

   Y can be either any of the above except "E" (otherwise there is nothing
   to test).  */
char invalid_ESE[] = "\240";
char invalid_SSE[] = "a\240";
char invalid_LSE[] = "abaabbaaabbb\240";
char invalid_RSE[] = "aaaaaaaaaaaaaaaaaaaa\240";
char invalid_ESS[] = "\240c";
char invalid_SSS[] = "a\240c";
char invalid_LSS[] = "abaabbaaabbb\240c";
char invalid_RSS[] = "aaaaaaaaaaaaaaaaaaaa\240c";
char invalid_ESL[] = "\240cdccddcccddd";
char invalid_SSL[] = "a\240cdccddcccddd";
char invalid_LSL[] = "abaabbaaabbb\240cdccddcccddd";
char invalid_RSL[] = "aaaaaaaaaaaaaaaaaaaa\240cdccddcccddd";
char invalid_ESR[] = "\240cccccccccccccccccccc";
char invalid_SSR[] = "a\240cccccccccccccccccccc";
char invalid_LSR[] = "abaabbaaabbb\240cccccccccccccccccccc";
char invalid_RSR[] = "aaaaaaaaaaaaaaaaaaaa\240cccccccccccccccccccc";
char invalid_ELE[] = "\240\240\240\240";
char invalid_SLE[] = "a\240\240\240\240";
char invalid_LLE[] = "abaabbaaabbb\240\240\240\240";
char invalid_RLE[] = "aaaaaaaaaaaaaaaaaaaa\240\240\240\240";
char invalid_ELS[] = "\240\240\240\240c";
char invalid_SLS[] = "a\240\240\240\240c";
char invalid_LLS[] = "abaabbaaabbb\240\240\240\240c";
char invalid_RLS[] = "aaaaaaaaaaaaaaaaaaaa\240\240\240\240c";
char invalid_ELL[] = "\240\240\240\240cdccddcccddd";
char invalid_SLL[] = "a\240\240\240\240cdccddcccddd";
char invalid_LLL[] = "abaabbaaabbb\240\240\240\240cdccddcccddd";
char invalid_RLL[] = "aaaaaaaaaaaaaaaaaaaa\240\240\240\240cdccddcccddd";
char invalid_ELR[] = "\240\240\240\240cccccccccccccccccccc";
char invalid_SLR[] = "a\240\240\240\240cccccccccccccccccccc";
char invalid_LLR[] = "abaabbaaabbb\240\240\240\240cccccccccccccccccccc";
char invalid_RLR[] = "aaaaaaaaaaaaaaaaaaaa\240\240\240\240cccccccccccccccccccc";
char invalid_ERE[] = ""
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240";
char invalid_LRE[] = "abaabbaaabbb"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240";
char invalid_RRE[] = "aaaaaaaaaaaaaaaaaaaa"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240";
char invalid_ERS[] = ""
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240c";
char invalid_ERL[] = ""
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240cdccddcccddd";
char invalid_ERR[] = ""
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240cccccccccccccccccccc";
char invalid_SRE[] = "a"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240";
char invalid_SRS[] = "a"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240c";
char invalid_SRL[] = "a"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240cdccddcccddd";
char invalid_SRR[] = "a"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240cccccccccccccccccccc";
char invalid_LRS[] = "abaabbaaabbb"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240c";
char invalid_LRL[] = "abaabbaaabbb"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240cdccddcccddd";
char invalid_LRR[] = "abaabbaaabbb"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240cccccccccccccccccccc";
char invalid_RRS[] = "aaaaaaaaaaaaaaaaaaaa"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240c";
char invalid_RRL[] = "aaaaaaaaaaaaaaaaaaaa"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240cdccddcccddd";
char invalid_RRR[] = "aaaaaaaaaaaaaaaaaaaa"
  "\240\240\240\240\240\240\240\240\240\240"
  "\240\240\240\240\240\240\240\240\240\240cccccccccccccccccccc";

/* -- */

float f_var = 65.0f;

int main ()
{
  void *p = malloc (1);

  /* Prevent AIX linker from removing variables.  */
  return ctable1[0] + ctable2[0] + int1dim[0] + int2dim[0][0]
    + int3dim[0][0][0] + int4dim[0][0][0][0] + teststring[0] +
      *parrays -> array1 + a1[0] + a2[0];
  free (p);
}
