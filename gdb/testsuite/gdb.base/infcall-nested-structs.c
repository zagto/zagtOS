/* This testcase is part of GDB, the GNU debugger.

   Copyright 2018-2019 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* This file is used for testing GDBs ability to pass structures to, and
   return structures from, functions.  All of the structures in this test
   are special in that they are small structures containing from 1 up to 5
   scalar fields, the fields can be inside nested structures, and there can
   be empty structures around too.

   This test is specifically written for RiscV and Aarch64, which both have
   special ABI rules for structures like these, however, there should be no harm
   in running these tests on other targets, though in many cases the
   structures will treated no differently to the structures already covered
   in the structs.exp test script.  */

#include <string.h>

/* Useful abreviations.  */
typedef char tc;
typedef short ts;
typedef int ti;
typedef long tl;
typedef long long tll;
typedef float tf;
typedef double td;
typedef long double tld;

#ifdef TEST_COMPLEX
typedef float _Complex tfc;
typedef double _Complex tdc;
typedef long double _Complex tldc;
#endif /* TEST_COMPLEX */

#define MAKE_CHECK_FUNCS(TYPE)					\
  int								\
  check_arg_ ## TYPE (struct TYPE arg)				\
  {								\
    return cmp_ ## TYPE (arg, ref_val_ ## TYPE);		\
  }								\
								\
  struct TYPE							\
  rtn_str_ ## TYPE (void)					\
  {								\
    return (ref_val_ ## TYPE);					\
  }

#define REF_VAL(NAME) struct NAME ref_val_ ## NAME
#define ES(NAME) struct { } NAME

/* Test is either for a single type or two differing types.  */
#if defined tA && ! defined tB
#define tB tA
#endif
#if ! defined tB
#error "Incorrect configuration of tA and tB defines"
#endif

/* Structures with a single field nested to various depths, along with
   some empty structures.  */
struct struct_01_01 { ES(es1); struct { struct { tA a; } s1; } s2; };
struct struct_01_02 { tA a; struct { struct { ES(es1); } s1; } s2; };
struct struct_01_03 { struct { struct { ES(es1); } s1; } s2; ES(es1); struct { struct { tA a; } s3; } s4;};
struct struct_01_04 { ES(es1); ES(es2); tA a; ES(es3); };

/* Structures with two fields nested to various depths, along with
   some empty structures.  */
struct struct_02_01 { ES(es1); struct { struct { tA a; tB b; } s1; } s2; };
struct struct_02_02 { tA a; struct { struct { ES(es1); } s1; } s2; tB b; };
struct struct_02_03 { struct { struct { ES(es1); } s1; } s2; ES(es1); struct { struct { tA a; } s3; } s4; struct { struct { tB b; } s5; } s6;};
struct struct_02_04 { ES(es1); ES(es2); tA a; ES(es3); tB b; };

/* Structures with four fields nested to various depths, along with
   some empty structures.  */
struct struct_04_01 { ES(es1); struct { struct { tA a; tB b; tA c; tB d; } s1; } s2; };
struct struct_04_02 { tA a; struct { struct { ES(es1); } s1; } s2; tB b; struct { struct { ES(es1); } s2; } s3; tA c; struct { struct { ES(es2); } s4; } s5; tB d;};
struct struct_04_03 { struct { struct { ES(es1); } s1; } s2; ES(es1); struct { struct { tA a; } s3; } s4; struct { struct { tB b; } s5; } s6; struct { struct { tA c; } s7; } s8; struct { struct { tB d; } s9; } s10;};
struct struct_04_04 { ES(es1); ES(es2); tA a; ES(es3); tB b; ES(es4); tA c; ES(es5); tB d; };

/* Structures with five fields nested to various depths, along with
   some empty structures.  */
struct struct_05_01 { ES(es1); struct { struct { tA a; tB b; tA c; tB d; tA e; } s1; } s2; };
struct struct_05_02 { tA a; struct { struct { ES(es1); } s1; } s2; tB b; struct { struct { ES(es1); } s2; } s3; tA c; struct { struct { ES(es2); } s4; } s5; tB d; struct { struct { ES(es2); } s6; } s7; tB e;};
struct struct_05_03 { struct { struct { ES(es1); } s1; } s2; ES(es1); struct { struct { tA a; } s3; } s4; struct { struct { tB b; } s5; } s6; struct { struct { tA c; } s7; } s8; struct { struct { tB d; } s9; } s10; struct { struct { tA e; } s11; } s12;};
struct struct_05_04 { ES(es1); ES(es2); tA a; ES(es3); tB b; ES(es4); tA c; ES(es5); tB d; ES(es6); tA e; };

int cmp_struct_01_01 (struct struct_01_01 a, struct struct_01_01 b)
{ return a.s2.s1.a == b.s2.s1.a; }

int cmp_struct_01_02 (struct struct_01_02 a, struct struct_01_02 b)
{ return a.a == b.a; }

int cmp_struct_01_03 (struct struct_01_03 a, struct struct_01_03 b)
{ return a.s4.s3.a == b.s4.s3.a; }

int cmp_struct_01_04 (struct struct_01_04 a, struct struct_01_04 b)
{ return a.a == b.a; }

int cmp_struct_02_01 (struct struct_02_01 a, struct struct_02_01 b)
{ return a.s2.s1.a == b.s2.s1.a && a.s2.s1.b == a.s2.s1.b; }

int cmp_struct_02_02 (struct struct_02_02 a, struct struct_02_02 b)
{ return a.a == b.a && a.b == b.b; }

int cmp_struct_02_03 (struct struct_02_03 a, struct struct_02_03 b)
{ return a.s4.s3.a == b.s4.s3.a && a.s6.s5.b == b.s6.s5.b; }

int cmp_struct_02_04 (struct struct_02_04 a, struct struct_02_04 b)
{ return a.a == b.a && a.b == b.b; }

int cmp_struct_04_01 (struct struct_04_01 a, struct struct_04_01 b)
{ return a.s2.s1.a == b.s2.s1.a && a.s2.s1.b == a.s2.s1.b
	 && a.s2.s1.c == b.s2.s1.c && a.s2.s1.d == a.s2.s1.d; }

int cmp_struct_04_02 (struct struct_04_02 a, struct struct_04_02 b)
{ return a.a == b.a && a.b == b.b && a.c == b.c && a.d == b.d; }

int cmp_struct_04_03 (struct struct_04_03 a, struct struct_04_03 b)
{ return a.s4.s3.a == b.s4.s3.a && a.s6.s5.b == b.s6.s5.b
	 && a.s8.s7.c == b.s8.s7.c && a.s10.s9.d == b.s10.s9.d; }

int cmp_struct_04_04 (struct struct_04_04 a, struct struct_04_04 b)
{ return a.a == b.a && a.b == b.b && a.c == b.c && a.d == b.d; }

int cmp_struct_05_01 (struct struct_05_01 a, struct struct_05_01 b)
{ return a.s2.s1.a == b.s2.s1.a && a.s2.s1.b == a.s2.s1.b
	 && a.s2.s1.c == b.s2.s1.c && a.s2.s1.d == a.s2.s1.d
	 && a.s2.s1.e == b.s2.s1.e; }

int cmp_struct_05_02 (struct struct_05_02 a, struct struct_05_02 b)
{ return a.a == b.a && a.b == b.b && a.c == b.c && a.d == b.d && a.e == b.e; }

int cmp_struct_05_03 (struct struct_05_03 a, struct struct_05_03 b)
{ return a.s4.s3.a == b.s4.s3.a && a.s6.s5.b == b.s6.s5.b
	 && a.s8.s7.c == b.s8.s7.c && a.s10.s9.d == b.s10.s9.d
	 && a.s12.s11.e == b.s12.s11.e; }

int cmp_struct_05_04 (struct struct_05_04 a, struct struct_05_04 b)
{ return a.a == b.a && a.b == b.b && a.c == b.c && a.d == b.d && a.e == b.e; }

REF_VAL(struct_01_01) = { {}, { { 'a' } } };
REF_VAL(struct_01_02) = { 'a', { { {} } } };
REF_VAL(struct_01_03) = { { { {} } }, {}, { { 'a' } } };
REF_VAL(struct_01_04) = { {}, {}, 'a', {} };

REF_VAL(struct_02_01) = { {}, { { 'a', 'b' } } };
REF_VAL(struct_02_02) = { 'a', { { {} } }, 'b' };
REF_VAL(struct_02_03) = { { { {} } }, {}, { { 'a' } }, { { 'b' } } };
REF_VAL(struct_02_04) = { {}, {}, 'a', {}, 'b' };

REF_VAL(struct_04_01) = { {}, { { 'a', 'b', 'c', 'd' } } };
REF_VAL(struct_04_02) = { 'a', { { {} } }, 'b', { { {} } }, 'c', { { {} } }, 'd' };
REF_VAL(struct_04_03) = { { { {} } }, {}, { { 'a' } }, { { 'b' } }, { { 'c' } }, { { 'd' } } };
REF_VAL(struct_04_04) = { {}, {}, 'a', {}, 'b', {}, 'c', {}, 'd' };

REF_VAL(struct_05_01) = { {}, { { 'a', 'b', 'c', 'd', 'e' } } };
REF_VAL(struct_05_02) = { 'a', { { {} } }, 'b', { { {} } }, 'c', { { {} } }, 'd', { { {} } }, 'e' };
REF_VAL(struct_05_03) = { { { {} } }, {}, { { 'a' } }, { { 'b' } }, { { 'c' } }, { { 'd' } }, { { 'e' } } };
REF_VAL(struct_05_04) = { {}, {}, 'a', {}, 'b', {}, 'c', {}, 'd', {}, 'e' };

/* Create all of the functions GDB will call to check functionality.  */
MAKE_CHECK_FUNCS(struct_01_01)
MAKE_CHECK_FUNCS(struct_01_02)
MAKE_CHECK_FUNCS(struct_01_03)
MAKE_CHECK_FUNCS(struct_01_04)
MAKE_CHECK_FUNCS(struct_02_01)
MAKE_CHECK_FUNCS(struct_02_02)
MAKE_CHECK_FUNCS(struct_02_03)
MAKE_CHECK_FUNCS(struct_02_04)
MAKE_CHECK_FUNCS(struct_04_01)
MAKE_CHECK_FUNCS(struct_04_02)
MAKE_CHECK_FUNCS(struct_04_03)
MAKE_CHECK_FUNCS(struct_04_04)
MAKE_CHECK_FUNCS(struct_05_01)
MAKE_CHECK_FUNCS(struct_05_02)
MAKE_CHECK_FUNCS(struct_05_03)
MAKE_CHECK_FUNCS(struct_05_04)

#define CALL_LINE(NAME) val += check_arg_ ## NAME (rtn_str_ ## NAME ())

int
call_all ()
{
  int val;

  CALL_LINE(struct_01_01);
  CALL_LINE(struct_01_02);
  CALL_LINE(struct_01_03);
  CALL_LINE(struct_01_04);
  CALL_LINE(struct_02_01);
  CALL_LINE(struct_02_02);
  CALL_LINE(struct_02_03);
  CALL_LINE(struct_02_04);
  CALL_LINE(struct_04_01);
  CALL_LINE(struct_04_02);
  CALL_LINE(struct_04_03);
  CALL_LINE(struct_04_04);
  CALL_LINE(struct_05_01);
  CALL_LINE(struct_05_02);
  CALL_LINE(struct_05_03);
  CALL_LINE(struct_05_04);

  return (val != 4);
}

void
breakpt (void)
{
  /* Nothing.  */
}

int
main ()
{
  int res;

  res = call_all ();
  breakpt (); /* Break Here.  */
  return res;
}
