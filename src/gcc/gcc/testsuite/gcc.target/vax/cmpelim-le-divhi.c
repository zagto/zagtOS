/* { dg-do compile } */
/* { dg-options "-fdump-rtl-cmpelim -dp" } */
/* { dg-skip-if "code quality test" { *-*-* } { "-O0" "-O1" } { "" } } */

typedef int __attribute__ ((mode (HI), vector_size (2))) int_t;

void
le_divhi (int_t *w, int_t *x, int_t *y)
{
  int_t v;

  v = *x / *y;
  if (v[0] <= 0)
    *w = v;
  else
    *w = v + 2;
}

/* Expect assembly like:

	divw3 *12(%ap),*8(%ap),%r0	# 34	[c=76]  *divhi3_ccnz/1
	jleq .L2			# 36	[c=26]  *branch_ccnz
	addw2 $2,%r0			# 33	[c=32]  *addhi3
.L2:

 */

/* { dg-final { scan-rtl-dump-times "deleting insn with uid" 1 "cmpelim" } } */
/* { dg-final { scan-assembler-not "\t(bit|cmpz?|tst). " } } */
/* { dg-final { scan-assembler "divhi\[^ \]*_ccnz(/\[0-9\]+)?\n" } } */
/* { dg-final { scan-assembler "branch_ccnz\n" } } */
