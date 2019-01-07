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

#include <stdlib.h>
#include "codegen/FrontEnd.hpp" // for feGetEnv

// Memory logging macros
// Memory allocation can be used very early by the compiler: can rely on printf working

static const char * LOGLEVEL = ::feGetEnv("OMRDebug_SegmentAllocator");
// LOGLEVEL="0"    // no debug output
// LOGLEVEL="1"    // summary output, about one line per function call
// LOGLEVEL="2"    // extra detail in functions
// LOGLEVEL="3"    // highest level of detail in functions

#define MEMLOG(n,s, ...) do { if ((LOGLEVEL != NULL) && ((int)(*LOGLEVEL) >= (int)(n+'0'))) { printf("SegAll %p : " s, this, __VA_ARGS__); } } while (0)
#define MEMLOG0(n,s)     do { if ((LOGLEVEL != NULL) && ((int)(*LOGLEVEL) >= (int)(n+'0'))) { printf("SegAll %p : " s, this); } } while (0)

#include "env/mem/SegmentAllocator.hpp"
#include "env/mem/MemorySegment.hpp"
#include <algorithm>


OMR::SegmentAllocator::SegmentAllocator(TR::BackingMemoryAllocator &backingMemoryAllocator,
                                        size_t allocationBlockSize)
   : _backingMemoryAllocator(backingMemoryAllocator)
   , _allocationBlockSize(allocationBlockSize)
   , _bytesAllocated(0)
   , _allocatedSegments( SegmentDequeAllocator(backingMemoryAllocator.rawAllocator()) )
   , _freeSegments( SegmentDequeAllocator(backingMemoryAllocator.rawAllocator()) )
   , _currentBackingMemoryChunk( TR::ref(_backingMemoryAllocator.allocate(allocationBlockSize)) )
   {
   validateConstruction();
   }

OMR::SegmentAllocator::SegmentAllocator(const TR::SegmentAllocator & other)
   : _backingMemoryAllocator(other._backingMemoryAllocator.clone())
   , _allocationBlockSize(other._allocationBlockSize)
   , _bytesAllocated(0)
   , _allocatedSegments( SegmentDequeAllocator(_backingMemoryAllocator.rawAllocator()) )
   , _freeSegments( SegmentDequeAllocator(_backingMemoryAllocator.rawAllocator()) )
   , _currentBackingMemoryChunk( TR::ref(_backingMemoryAllocator.allocate(_allocationBlockSize)) )
   {
   validateConstruction();
   }

void
OMR::SegmentAllocator::validateConstruction()
   {
   TR_ASSERT_FATAL(_allocationBlockSize > 0 && ((_allocationBlockSize & (_allocationBlockSize - 1)) == 0), "allocationBlockSize must be power of 2");
   TR_ASSERT_FATAL((_backingMemoryAllocator.minimumAllocationSize() / _allocationBlockSize) * _allocationBlockSize == _backingMemoryAllocator.minimumAllocationSize(), "allocationBlockSize must evenly divide into backingMemoryAllocator's minimum allocation size");
   MEMLOG(1, "creation: backing allocator %p block %lu\n", &_backingMemoryAllocator, _allocationBlockSize);
   MEMLOG(2, "chunk allocated %p %lu\n", &(_currentBackingMemoryChunk.get()), _currentBackingMemoryChunk.get().size());
   }

OMR::SegmentAllocator::~SegmentAllocator() throw()
   {
   // delete all the TR::MemorySegments for segments in the allocated list
   while (!_allocatedSegments.empty())
      {
      TR::MemorySegment &segment = _allocatedSegments.back().get();
      _allocatedSegments.pop_back();
      _bytesAllocated -= segment.size();
      _backingMemoryAllocator.rawAllocator().deallocate(&segment, sizeof(TR::MemorySegment));
      }

   // delete all the TR::MemorySegments for segments in the free list
   while (!_freeSegments.empty())
      {
      TR::MemorySegment &segment = _freeSegments.back().get();
      _freeSegments.pop_back();
      _backingMemoryAllocator.rawAllocator().deallocate(&segment, sizeof(TR::MemorySegment));
      }

   // let _backingMemoryAllocator know to deallocate all its segments
   _backingMemoryAllocator.deallocateSegments();

   TR_ASSERT(_bytesAllocated == 0, "Unexpectedly _bytesAllocated not equal to zero after freeing all segments");
   }

TR::MemorySegment &
OMR::SegmentAllocator::allocate(size_t requiredSize)
   {
   // round up to a multiple of the default allocation size
   size_t const roundedSize = ( ( ( requiredSize + (_allocationBlockSize - 1) ) / _allocationBlockSize ) * _allocationBlockSize );
   MEMLOG(1, "allocate request %lu will allocate at least %lu\n", requiredSize, roundedSize);

   // if we can recycle a free segment, then do that
   if (!_freeSegments.empty() && roundedSize == _allocationBlockSize)
      {
      TR::MemorySegment &recycledSegment = _freeSegments.back();
      TR_ASSERT_FATAL(recycledSegment.size() == roundedSize, "unexpected size on free segment");
      _freeSegments.pop_back();
      recycledSegment.reset();
      _bytesAllocated += roundedSize;
      MEMLOG(2, "recyling free segment %p size %lu\n", &recycledSegment, recycledSegment.size());
      _allocatedSegments.push_front( TR::ref(recycledSegment) );
      return recycledSegment;
      }

   // see if we could satisfy allocation request from _currentBackingMemoryChunk

   // first, see if we need to allocate a new chunk of backing memory
   size_t remainingSpace = _currentBackingMemoryChunk.get().remaining();
   if (remainingSpace < roundedSize && _backingMemoryAllocator.fitsInMinimumSizeSegment(roundedSize))
      {
      MEMLOG(2, "current chunk only %lu remaining, need to allocate a new one\n", remainingSpace);

      // doesn't fit in current chunk, but it would fit in a fresh chunk
      // first split remaining space in current chunk into small segments and put onto free list
      claimFreeSegments(_currentBackingMemoryChunk.get());
      _currentBackingMemoryChunk = TR::ref(_backingMemoryAllocator.allocate(_allocationBlockSize));

      // update remainingSpace so allocation will come from the (new) current chunk
      remainingSpace = _currentBackingMemoryChunk.get().remaining();
      MEMLOG(2, "new chunk %p allocated, remaining now %lu\n", &(_currentBackingMemoryChunk.get()), remainingSpace);
      }

   // allocate from current chunk if it will fit
   if (remainingSpace >= roundedSize)
      {
      MEMLOG0(2, "allocation will come from current chunk\n");
      return allocateFromSegment(roundedSize, _currentBackingMemoryChunk.get());
      }

   // alas, we need a dedicated backing memory allocation for this allocation request
   TR::MemorySegment &newSegment = _backingMemoryAllocator.allocate(roundedSize);
   MEMLOG(2, "need dedicated segment, allocated %p size %lu\n", &newSegment, newSegment.size());
   _allocatedSegments.push_front( TR::ref(newSegment) );
   _bytesAllocated += roundedSize;
   return newSegment;
   }

void
OMR::SegmentAllocator::deallocate(TR::MemorySegment & segment) throw()
   {
   MEMLOG(1, "deallocate %p with size %lu request\n", &segment, segment.size());
   size_t const segmentSize = segment.size();
   _bytesAllocated -= segmentSize;
   segment.reset(); // claim the entire segment

   // segment should no longer be consider "allocated"
   bool found=false;
   auto segmentToRemove = _allocatedSegments.begin();
   while (segmentToRemove != _allocatedSegments.end())
      {
      auto ref = *segmentToRemove;
      MEMLOG(3, "_allocatedSegments iterating segment %p\n", &(ref.get()));
      if (segment == ref.get())
         {
         found=true;
         break;
         }
      segmentToRemove++;
      }
   TR_ASSERT(found, "deallocate() request for segment not in list of allocated segments");
   // we cannot erase segmentToRemove from _allocatedSegments yet because _allocatedSegments owns the TR::MemorySegment object
   //  so if we were to erase it now, that memory would be reclaimed before we're done with the freeing operations below

   auto ref = *segmentToRemove;
   MEMLOG(2, "deallocate %p ( %p ) was found in allocatedSegments\n", &segment, &(ref.get()));

   // small segments that match the block size can just be put onto free list
   if (segmentSize == _allocationBlockSize)
      {
      try
         {
         _freeSegments.push_front(TR::ref(segment));
         MEMLOG(2, "segment %p reset and placed on free segment list\n", &segment);
         }
      catch (...)
         {
         // not much we can do here except leak
         }
      }
   else if (_backingMemoryAllocator.fitsInMinimumSizeSegment(segmentSize))
      {
      // segment should be a multiple of _allocationBlockSize, so cut up into smaller free segments
      TR_ASSERT((segmentSize % _allocationBlockSize) == 0, "unexpected segment size");
      MEMLOG(2, "segment %p will be broken down and placed onto free segment list\n", &segment);
      claimFreeSegments(segment);
      }
   else
      {
      // dedicated segment for the allocation, so just give it back to the backing memory allocator
      MEMLOG(2, "segment %p too large, deallocate via backing allocator\n", &segment);
      _backingMemoryAllocator.deallocate(segment);
      }

   // now we can remove it
   MEMLOG(2, "finally removing segment %p\n", &(ref.get()));
   _allocatedSegments.erase(segmentToRemove);
   }

// split up remaining segment space into block sized chunks and add to free list
void
OMR::SegmentAllocator::claimFreeSegments(TR::MemorySegment & segment)
   {
   while (segment.remaining() > 0)
      {
      try
         {
         void *newSegmentArea = segment.allocate(_allocationBlockSize);
         createFreeSegmentFromArea(_allocationBlockSize, newSegmentArea);
         }
      catch (...)
         {
         // not much we can do here except leak
         }
      }
   }

TR::MemorySegment &
OMR::SegmentAllocator::allocateFromSegment(size_t size, TR::MemorySegment &segment)
   {
   TR_ASSERT( (size % _allocationBlockSize) == 0, "Misaligned segment");
   void *newSegmentArea = segment.allocate(size);
   if (!newSegmentArea) throw std::bad_alloc();
   try
      {
      TR::MemorySegment &newSegment = allocateSegmentFromArea(size, newSegmentArea);
      return newSegment;
      }
   catch (...)
      {
      // one of the few scenarios where rewind() is safe and warranted
      segment.rewind(size);
      throw;
      }
   }

TR::MemorySegment &
OMR::SegmentAllocator::createFreeSegmentFromArea(size_t size, void *newSegmentArea)
   {
   TR::MemorySegment *newSegment = new (_backingMemoryAllocator.rawAllocator()) TR::MemorySegment(newSegmentArea, size);
   _freeSegments.push_front( TR::ref(*newSegment) );
   MEMLOG(1, "free segment %p claimed base %p size %lu\n", newSegment, newSegment->base(), newSegment->size());
   return *newSegment;
   }

TR::MemorySegment &
OMR::SegmentAllocator::allocateSegmentFromArea(size_t size, void *newSegmentArea)
   {
   TR::MemorySegment *newSegment = new (_backingMemoryAllocator.rawAllocator()) TR::MemorySegment(newSegmentArea, size);
   _allocatedSegments.push_front( TR::ref(*newSegment) );
   _bytesAllocated += size;
   MEMLOG(1, "allocated segment %p base %p size %lu\n", newSegment, newSegment->base(), newSegment->size());
   return *newSegment;
   }

size_t
OMR::SegmentAllocator::systemBytesAllocated()
   {
   return backingMemoryAllocator().bytesAllocated();
   }
