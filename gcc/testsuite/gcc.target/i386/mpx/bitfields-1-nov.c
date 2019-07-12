/* { dg-do run } */
/* { dg-options "-fcheck-pointer-bounds -mmpx" } */


#include "mpx-check.h"

struct s {
  int a;
  int b : 10;
  int c : 1;
  int e : 10;
} s;

#define HH (unsigned char)1

int foo (struct s *p)
{
  int val = p->b;
  printf ("%d\n", val);
  return val == HH;
}

int mpx_test (int argc, const char **argv)
{
  struct s buf[100];

  foo (buf);
  foo (buf + 99);

  return 0;
}
