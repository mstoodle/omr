/*******************************************************************************
 * Copyright (c) 2000, 2018 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef OMR_SEGMENTALLOCATOR_INCL
#define OMR_SEGMENTALLOCATOR_INCL

#include <set>
#include <deque>
#include "env/mem/MemorySegment.hpp"
#include "env/TypedAllocator.hpp"
#include "infra/ReferenceWrapper.hpp"
#include "env/mem/RawAllocator.hpp"
#include "env/mem/BackingMemoryAllocator.hpp"
#include "env/mem/SegmentAllocator.hpp"

namespace OMR {

// SegmentAllocator uses a BackingMemoryAllocator to allocate large chunk of memory,
//  which are then split into smaller pieces and returned as TR::MemorySegments. There is a
//  block size that is used for allocation requests that are smaller than the chunks
//  allocated by the BackingMemoryAllocator: the requested allocation size will be rounded
//  up to a multiple of the allocationBlockSize (which must evenly fit into the
//  BackingMemoryAllocator's minimumAllocationSize -- the default "chunk" size). 
//
// SegmentAllocator maintains a free list of segments that have been previously
//  deallocated by the JIT (but are obviously not returned to the BackingMemoryAllocator).
//  These free segments are designed to be exactly the allocationBlockSize. Whenever
//  a 'regular' segment (a segment whose size is an even multiple of the allocationBlockSize)
//  is deallocated by the JIT, that segment will be split into multiple segments that
//  each hold exactly allocationBlockSize bytes. These segments are then added to the free
//  list which are preferentially used to satisfy new allocation requests. Deallocated
//  segments that are not a multiple of the allocationBlockSize are returned to the
//  BackingMemoryAllocator for deallocation. Not reusing these segments is an attempt
//  to balance fragmentation against the (expected) higher cost to (de)allocate via the
//  BackingMemoryAllocator.
//
// The SegmentAllocator also maintains one "current" chunk of memory from the
//  BackingMemoryAllocator. Allocations that cannot be satisfied by a free segment
//  (either because the requested size is too large or because there aren't any)
//  can be allocated directly from this current chunk if it has sufficient free space.
//  Like all allocations, the allocated space will be rounded up to a multiple of
//  the allocationBlockSize. If requested size does not fit into the current chunk
//  but is less than the default chunk size (as determined by the BackingMemoryAllocator)
//  then any remaining space in the current chunk will be split into segments holding
//  exactly allocationBlockSize bytes and those segments will be placed on the free
//  list. Then a new chunk will be allocated from the BackingMemoryAllocator and it
//  will be used to satisfy the current allocation request in the usual way. If the
//  BackingMemoryAllocator considers the allocation request to be larger than its
//  default chunk size, then a dedicated memory segment will be allocated by the
//  BackingMemorySegment to satisfy this (large) memory allocation request. When
//  this large memory segment is deallocated by the JIT, it will be returned
//  directly to the BackingMemoryAllocator EVEN IF its size happens to be a
//  multiple of the allocationBlockSize. The segment is proactively deallocated
//  to avoid holding on to a large amount of unused memory longer than necessary.

class SegmentAllocator
   {
public:
   SegmentAllocator(TR::BackingMemoryAllocator & backingMemoryAllocator, size_t allocationBlockSize);
   SegmentAllocator(const TR::SegmentAllocator & s);
   ~SegmentAllocator() throw();

   void validateConstruction();

   TR::BackingMemoryAllocator & backingMemoryAllocator() { return _backingMemoryAllocator; }
   size_t allocationBlockSize()                          { return _allocationBlockSize; }
   size_t bytesAllocated()                               { return _bytesAllocated; }

   // for compatibility, should be removed after refactoring is complete
   size_t regionBytesAllocated()                         { return bytesAllocated(); }
   size_t systemBytesAllocated();

   virtual TR::MemorySegment &allocate(size_t requiredSize);
   virtual void deallocate(TR::MemorySegment &segment) throw();

protected:
   void claimFreeSegments(TR::MemorySegment & segment);
   TR::MemorySegment & allocateFromSegment(size_t size, TR::MemorySegment &segment);
   TR::MemorySegment & createFreeSegmentFromArea(size_t size, void *newSegmentArea);
   TR::MemorySegment & allocateSegmentFromArea(size_t size, void *newSegmentArea);

   TR::BackingMemoryAllocator & _backingMemoryAllocator;
   size_t _allocationBlockSize;
   size_t _bytesAllocated;

   typedef TR::typed_allocator<
      TR::reference_wrapper<TR::MemorySegment>,
      TR::RawAllocator
      > SegmentDequeAllocator;

   std::deque<
      TR::reference_wrapper<TR::MemorySegment>,
      SegmentDequeAllocator
      > _allocatedSegments;

   std::deque<
      TR::reference_wrapper<TR::MemorySegment>,
      SegmentDequeAllocator
      > _freeSegments;

   // Current "regular" backing memory chunk from which memory might be allocated.
   // "Regular" here means a minimum (default) sized segment (as defined by the
   //   BackingMemoryAllocator). Regular segments can be reused and will be placed
   //   on the free list when deallocated.
   // Segments larger than the default are dedicated for a particular allocation and
   //  will be deallocated (via the BackingMemoryAllocator) when no longer needed.
   //  In particular, these larger segments are never placed on the free list.
   // _currentBackingMemoryChunk is only allowed to hold a "regular" segment.
   TR::reference_wrapper<TR::MemorySegment> _currentBackingMemoryChunk;

   };

} // namespace OMR

#endif // OMR_SEGMENTALLOCATOR_INCL
