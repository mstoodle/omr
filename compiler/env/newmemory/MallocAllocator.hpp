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

#ifndef OMR_MALLOCALLOCATOR_INCL
#define OMR_MALLOCALLOCATOR_INCL

#pragma once

#include "env/newmemory/RawAllocator.hpp"

namespace OMR {

// MallocAllocator is a subclass of RawAllocator that uses malloc() and free() to manage
// RawMemory. It provides no implementation for noLongerUsed().

class MallocAllocator : public TR::RawAllocator
   {
public:
   MallocAllocator()
      : TR::RawAllocator()
      { }

   virtual TR::RawMemory allocate(size_t size, const std::nothrow_t tag, void * hint = 0) const throw()
      {
      return malloc(size);
      }

   TR::RawMemory allocate(size_t size, void * hint = 0) const
      {
      TR::RawMemory const alloc = allocate(size, std::nothrow, hint);
      if (!alloc) throw std::bad_alloc();
      return alloc;
      }

   virtual void deallocate(TR::RawMemory p) const throw()
      {
      free(p);
      }

   virtual void deallocate(TR::RawMemory p, const size_t size) const throw()
      {
      free(p);
      }

   friend bool operator ==(const MallocAllocator &left, const MallocAllocator &right)
      {
      return true;
      }

   friend bool operator !=(const MallocAllocator &left, const MallocAllocator &right)
      {
      return !operator ==(left, right);
      }

   template <typename T>
   operator TR::typed_allocator<T, MallocAllocator>() throw()
      {
      return TR::typed_allocator<T, MallocAllocator>(*this);
      }

   };

} // namespace OMR

// Allocate from a MallocAllocator using placement new and a MallocAllocator reference
inline void * operator new(size_t size, const OMR::MallocAllocator & allocator)
   {
   return allocator.allocate(size);
   }

inline void * operator new[](size_t size, const OMR::MallocAllocator & allocator)
   {
   return allocator.allocate(size);
   }

inline void * operator new(size_t size, const OMR::MallocAllocator & allocator, const std::nothrow_t& tag) throw()
   {
   return allocator.allocate(size, tag);
   }

inline void * operator new[](size_t size, const OMR::MallocAllocator & allocator, const std::nothrow_t& tag) throw()
   {
   return allocator.allocate(size, tag);
   }

inline void operator delete(void *ptr, const OMR::MallocAllocator & allocator) throw()
   {
   allocator.deallocate(ptr);
   }

inline void operator delete[](void *ptr, const OMR::MallocAllocator & allocator) throw()
   {
   allocator.deallocate(ptr);
   }

#endif // defined(OMR_MALLOCALLOCATOR_INCL)
