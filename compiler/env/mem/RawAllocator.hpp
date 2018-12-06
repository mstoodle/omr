/*******************************************************************************
 * Copyright (c) 2000, 2018 IBM Corp. and others
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

namespace OMR { class RawAllocator; }
namespace TR { using OMR::RawAllocator; }

#include <stddef.h>  // for size_t
#include <cstdlib>   // for free, malloc
#include <new>       // for bad_alloc, nothrow, nothrow_t
#include "env/TypedAllocator.hpp"

namespace OMR {

// RawAllocator should carry no state so that all RawAllocator objects are equivalent
// and the lifetime of memory allocated by a RawAllocator object does not depend on
// the lifetime of that particular RawAllocator object. It should be perfectly valid
// for one RawAllocator object to be used to allocate a RawSegment and a completely
// different RawAllocator object used to deallocate that RawSegment.
// 

class RawAllocator
   {
public:
   typedef void *RawSegment;

   RawAllocator()
      { }

   RawAllocator(const TR::RawAllocator &other)
      { }

   virtual TR::RawAllocator & clone();

   virtual RawSegment allocate(size_t size, const std::nothrow_t tag, void * hint = 0) throw()
      {
      return malloc(size);
      }

   virtual RawSegment allocate(size_t size, void * hint = 0)
      {
      void * const alloc = allocate(size, std::nothrow, hint);
      if (!alloc) throw std::bad_alloc();
      return alloc;
      }

   virtual void deallocate(RawSegment p) throw()
      {
      free(p);
      }

   virtual void deallocate(RawSegment p, const size_t size) throw()
      {
      free(p);
      }

   virtual void protect(RawSegment p, size_t size) throw()
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
   operator TR::typed_allocator< T, RawAllocator >() throw()
      {
      return TR::typed_allocator< T, RawAllocator >(*this);
      }

   };

}

inline void * operator new(size_t size, TR::RawAllocator allocator)
   {
   return allocator.allocate(size);
   }

inline void operator delete(void *ptr, TR::RawAllocator allocator) throw()
   {
   allocator.deallocate(ptr);
   }

inline void * operator new[](size_t size, TR::RawAllocator allocator)
   {
   return allocator.allocate(size);
   }

inline void operator delete[](void *ptr, TR::RawAllocator allocator) throw()
   {
   allocator.deallocate(ptr);
   }

inline void * operator new(size_t size, TR::RawAllocator allocator, const std::nothrow_t& tag) throw()
   {
   return allocator.allocate(size, tag);
   }

inline void * operator new[](size_t size, TR::RawAllocator allocator, const std::nothrow_t& tag) throw()
   {
   return allocator.allocate(size, tag);
   }

#endif // TR_RAWALLOCATOR_INCL
