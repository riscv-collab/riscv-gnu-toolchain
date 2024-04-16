typedef struct
{
  int count;
  int reload;
  int prescale;
  int tcspr;
  unsigned char bsr;
  unsigned char mode;
  unsigned char ic;
} Timer_A;

extern Timer_A timer_a;

extern void update_timer_a (void);
