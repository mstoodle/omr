/*******************************************************************************
 * Copyright (c) 2000, 2019 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef TR_RAWALLOCATOR_INCL
#define TR_RAWALLOCATOR_INCL

#pragma once

#include <stddef.h>  // for size_t
#include <cstdlib>   // for free, malloc
#include <new>       // for bad_alloc, nothrow, nothrow_t
#include "env/TypedAllocator.hpp"


namespace TR {

typedef void *RawMemory;

// RawAllocator is an abstract class used by the rest of the compiler to interact
// with TR::RawMemory allocators (e.g. concrete subclasses like MallocAllocator,
// DebugRawAllocator, etc.). RawAllocator objects are expected to be thread-safe if
// they are used by multiple threads and to be *interchangeable*. Interchangeable
// means that objects *of a particular subclass* are supposed to be like one another
// in that it should be perfectly valid for one object to allocate a piece of
// TR::RawMemory and for a different object to be used to deallocate that TR::RawMemory.
//
// It is not expected that objects from different subclasses of RawAllocator will be
// interchangeable, since different subclasses may allocate/deallocate memory using
// different facilities.
//

class RawAllocator
   {
public:
   RawAllocator()
      { }

   // allocate() tries to allocate a TR::RawMemory block that can hold at least size bytes.
   // On failure, this version of allocate() returns NULL.
   virtual TR::RawMemory allocate(size_t size, const std::nothrow_t tag, void * hint = 0) const throw() =  0;

   // allocate() tries to allocate a TR::RawMemory block that can hold at least size bytes.
   // On failure, this version of allocate() will throw std::bad_alloc()
   virtual TR::RawMemory allocate(size_t size, void * hint = 0) const
      {
      TR::RawMemory const alloc = allocate(size, std::nothrow, hint);
      if (!alloc) throw std::bad_alloc();
      return alloc;
      }

   // deallocate the TR::RawMemory block pointed to by p
   // Less preferable alternative to deallocate(p, size) in that some RawAllocator subclasses may not
   // be able to support deallocating without a size. If you have a size, please convey it.
   virtual void deallocate(TR::RawMemory p) const throw() = 0;

   // default simplification: deallocate() does not need a size.
   // Despite defaulting to deallocate(p), this implementation is the preferred one to use if a size
   // is available. Some RawAllocator subclasses may be more effective when a size is known.
   virtual void deallocate(TR::RawMemory p, const size_t size) const throw()
      {
      deallocate(p);
      }

   // Notify the RawAllocator that this memory should no longer be accessed until it is deallocated
   // Typically used in debugging scenarios where, at the usual deallocate() point, noLongerUsed()
   // is called to remove access to the memory region so that any unexpected subsequent access to
   // that memory will fault (alternatively for platforms that don't support access protection, the
   // memory can be painted with a value to make accesses more obvious). See DebugAllocator for how
   // noLongerUsed() can be implemented. By default, noLongerUsed() will do nothing. Also note that
   // future work should probably remove this service as described in the DebugAllocator header file.
   virtual void noLongerUsed(TR::RawMemory p, size_t size) const throw()
      { }

   friend bool operator ==(const RawAllocator &left, const RawAllocator &right)
      {
      return true;
      }

   friend bool operator !=(const RawAllocator &left, const RawAllocator &right)
      {
      return !operator ==(left, right);
      }

   template <typename T>
   operator TR::typed_allocator<T, RawAllocator>() throw()
      {
      return TR::typed_allocator<T, RawAllocator>(*this);
      }

   };

}

#endif

