// Support routines for the -*- C++ -*- dynamic memory management.

// Copyright (C) 1997-2021 Free Software Foundation, Inc.
//
// This file is part of GCC.
//
// GCC is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// GCC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

#include <bits/c++config.h>
#ifdef _ZAGTOS_KERNEL
 #include <common/inttypes.hpp>
 #include <memory/DLMallocGlue.hpp>
#else
 #include <cstdlib>
#endif
#include <bits/exception_defines.h>
#include "new"

using std::new_handler;
using std::bad_alloc;
#if _GLIBCXX_HOSTED
using std::malloc;
#else
// A freestanding C runtime may not provide "malloc" -- but there is no
// other reasonable way to implement "operator new".
extern "C" void *malloc (std::size_t);
#endif

_GLIBCXX_WEAK_DEFINITION void *
operator new (std::size_t sz) _GLIBCXX_THROW (std::bad_alloc)
{
  void *p;

  /* malloc (0) is unpredictable; avoid it.  */
  if (__builtin_expect (sz == 0, false))
    sz = 1;

#ifdef _ZAGTOS_KERNEL
  while ((p = DLMallocGlue.allocate(sz, 0).asPointer<void>()) == 0)
#else
  while ((p = malloc (sz)) == 0)
#endif
    {
      new_handler handler = std::get_new_handler ();
      if (! handler)
	_GLIBCXX_THROW_OR_ABORT(bad_alloc());
      handler ();
    }

  return p;
}
