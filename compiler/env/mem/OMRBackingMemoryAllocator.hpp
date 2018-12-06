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

#ifndef OMR_BACKINGMEMORYALLOCATOR_INCL
#define OMR_BACKINGMEMORYALLOCATOR_INCL

#pragma once

#include <deque>
#include "infra/ReferenceWrapper.hpp"
#include "env/mem/RawAllocator.hpp"
#include "env/mem/MemorySegment.hpp"
#include "env/mem/BackingMemoryAllocator.hpp"

namespace OMR
{

// BackingMemoryAllocator manages "large" chunks of memory (minimum size configured
//  by _minimumAllocationSize but allocate() can request a larger size) from the RawAllocator
//  provided at construction. Allocated memory is returned as a TR::MemorySegment.
//  The minimum allocation size MUST be a power of 2 otherwise an assertion will be
//  thrown by the constructor.
//  An allocation limit can be set either at construction time or later, which will
//  prevent the BackingMemoryAllocator from allocating more than the limit
//  (by default, _allocationLimit == 0 which means no limit). Setting an allocation
//  limit to a value lower than the current amount of allocated memory will throw
//  an assertion.

class BackingMemoryAllocator
   {
public:
   BackingMemoryAllocator(TR::RawAllocator & rawAllocator, size_t minimumAllocationSize, size_t allocationLimit=0)
      : _rawAllocator(rawAllocator)
      , _minimumAllocationSize(minimumAllocationSize)
      , _allocationLimit(allocationLimit)
      , _bytesAllocated(0)
      , _allocatedSegments( SegmentDequeAllocator(rawAllocator) )
      {
      validateConstruction();
      }

   BackingMemoryAllocator(TR::BackingMemoryAllocator & other)
      : _rawAllocator(other._rawAllocator.clone())
      , _minimumAllocationSize(other._minimumAllocationSize)
      , _allocationLimit(other._allocationLimit)
      , _bytesAllocated(0)
      , _allocatedSegments( SegmentDequeAllocator(other._rawAllocator) )
      {
      validateConstruction();
      }

   void validateConstruction()
      {
      TR_ASSERT(_minimumAllocationSize > 0 && _minimumAllocationSize && ((_minimumAllocationSize & (_minimumAllocationSize - 1)) == 0), "minimumAllocationSize must be power of 2");
      TR_ASSERT(_allocationLimit == 0 || _allocationLimit >= _minimumAllocationSize, "non-zero allocationLimit must be at least as large as minimumAllocationSize");
      }

   virtual TR::BackingMemoryAllocator & clone();

   virtual TR::MemorySegment &allocate(size_t requestedSize, void * hint = 0);
   virtual void deallocate(TR::MemorySegment & p) throw();

   void deallocateSegments();

   void setAllocationLimit(size_t newLimit)
      {
      TR_ASSERT(_bytesAllocated < newLimit, "Have already allocated more memory than new allocation limit");
      _allocationLimit = newLimit;
      }

   size_t allocationLimit()                                     { return _allocationLimit; }
   virtual size_t bytesAllocated()                              { return _bytesAllocated; }
   size_t minimumAllocationSize()                               { return _minimumAllocationSize; }
   bool fitsInMinimumSizeSegment(size_t size) throw()           { return size <= _minimumAllocationSize; }
   TR::RawAllocator & rawAllocator()                            { return _rawAllocator; }

private:
   TR::RawAllocator & _rawAllocator;
   size_t _minimumAllocationSize;
   size_t _allocationLimit;
   size_t _bytesAllocated;

   typedef TR::typed_allocator<
      TR::reference_wrapper<TR::MemorySegment>,
      TR::RawAllocator
      > SegmentDequeAllocator;

   std::deque<
      TR::reference_wrapper<TR::MemorySegment>,
      SegmentDequeAllocator
      > _allocatedSegments;
   };
}

void *operator new(size_t size, TR::MemorySegment &segment) throw();
void *operator new[](size_t size, TR::MemorySegment &segment) throw();

void operator delete(void *, TR::MemorySegment &segment) throw();
void operator delete[](void *ptr, TR::MemorySegment  &segment) throw();

#endif // OMR_BACKINGMEMORYALLOCATOR_INCL
