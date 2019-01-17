/*******************************************************************************
 * Copyright (c) 2019, 2019 IBM Corp. and others
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

#ifndef OMR_CUSTOMALLOCATOR_INCL
#define OMR_CUSTOMALLOCATOR_INCL

#pragma once

#include <stddef.h>  // for size_t
#include <cstdlib>   // for free, malloc
#include <new>       // for bad_alloc, nothrow, nothrow_t
#include "infra/Assert.hpp" //  for OMR_ASSERT_FATAL
#include "env/newmemory/RawAllocator.hpp"

// CustomAllocator is a more flexible implementation of TR::RawAllocator
// but at the performance cost of an extra pointer indirection for every
// API call. Upon construction, the functions to call to implement
// allocate() (AllocatFunction), deallocate (DeallocateFunction), and
// optionally noLongerUsed() (NoLongerUsedFunction) must be specified.
// Note that all CustomAllocator objects are assumed to be the same.
// If two different kinds are needed, then different classes must be
// created.

namespace OMR
{

class CustomAllocator :  public TR::RawAllocator
   {
public:
   typedef TR::RawMemory (*AllocateFunction)(size_t, TR::RawMemory);
   typedef void (*DeallocateFunction)(TR::RawMemory);
   typedef void (*NoLongerUsedFunction)(TR::RawMemory, size_t);

   CustomAllocator(AllocateFunction allocateFunction,
                   DeallocateFunction deallocateFunction,
                   NoLongerUseFunction noLongerUsedFunction=0)
      : _allocateFunction(allocateFunction)
      , _deallocateFunction(deallocateFunction)
      , _noLongerUsedFunction(noLongerUsedFunction)
      {
      OMR_ASSERT_FATAL(_allocateFunction && _deallocateFunction, "Custom allocator needs valid allocateFunction and deallocateFunction");
      }

   virtual TR::RawMemory allocate(size_t size, const std::nothrow_t tag, void * hint = 0) const throw()
      {
      return (*_allocateFunction)(size, hint);
      }

   virtual void deallocate(TR::RawMemory p) const throw()
      {
      (*_deallocateFunction)(p);
      }

   virtual void noLongerUsed(TR::RawMemory p, size_t size) const throw()
      {
      if (_noLongerUsedFunction)
         (*_noLongerUsedFunction)(p, size);
      }

   friend bool operator ==(const CustomAllocator &left, const CustomAllocator &right)
      {
      return true;
      }

   friend bool operator !=(const CustomAllocator &left, const CustomAllocator &right)
      {
      return !operator ==(left, right);
      }

   template <typename T>
   operator TR::typed_allocator<T, CustomAllocator>() throw()
      {
      return TR::typed_allocator<T, CustomAllocator>(*this);
      }

private:
   AllocateFunction     _allocateFunction;
   DeallocateFunction   _deallocateFunction;
   NoLongerUsedFunction _noLongerUsedFunction;
   };

} // namespace OMR

#endif // OMR_CUSTOMALLOCATOR_INCL
