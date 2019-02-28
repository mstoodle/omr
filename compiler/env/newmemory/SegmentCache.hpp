/*******************************************************************************
 * Copyright (c) 2000, 2019 IBM Corp. and others
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

#ifndef OMR_SEGMENTCACHE_INCL
#define OMR_SEGMENTCACHE_INCL

#include <stdlib.h>
#include <algorithm>
#include <deque>
#include <set>

// Memory logging macros
// Memory allocation can be used very early by the compiler: can rely on printf working

extern const char * SC_LOGLEVEL;
// LOGLEVEL="0"    // no debug output
// LOGLEVEL="1"    // summary output, about one line per function call
// LOGLEVEL="2"    // extra detail in functions
// LOGLEVEL="3"    // highest level of detail in functions

#define MEMLOG(n,s, ...) do { if ((SC_LOGLEVEL != NULL) && ((int)(*SC_LOGLEVEL) >= (int)(n+'0'))) { printf("Seg$$$ %p : " s, this, __VA_ARGS__); } } while (0)
#define MEMLOG0(n,s)     do { if ((SC_LOGLEVEL != NULL) && ((int)(*SC_LOGLEVEL) >= (int)(n+'0'))) { printf("Seg$$$ %p : " s, this); } } while (0)

#include "env/TypedAllocator.hpp"
#include "env/newmemory/MemorySegment.hpp"
#include "env/newmemory/RawAllocator.hpp"
#include "env/newmemory/SegmentAllocator.hpp"
#include "infra/ReferenceWrapper.hpp"

/* @class SegmentCache
 * @brief A SegmentCache is a TR::SegmentAllocator that attempts to reuse allocated segments as much as possible
 * The SegmentCache will only attempt to reuse segments below a certain size (cacheableSizeThreshold) and will
 * cut up deallocated segments into smaller segments of cacheBlockSize. Allocations smaller than cacheBlockSize
 * will be rounded up to the cacheBlockSize to prevent fragmentation. Any allocation request above the
 * cacheableThresholdSize will be directly allocated using the provided SegmentAllocator (which is typically a
 * NativeSegmentAllocator but does not have to be). Such large segments will also be directly deallocated
 * rather than being cut up to be placed on the free segment list.
 */

namespace OMR {

template <class RawAllocator>
class SegmentCache : TR::SegmentAllocator
   {
public:
   SegmentCache(TR::SegmentAllocator &segAllocator, size_t cacheBlockSize, size_t cacheableSizeThreshold)
      : rawAllocator()
      , _segAllocator(segAllocator)
      , _cacheBlockSize(cacheBlockSize)
      , _cacheableSizeThreshold(cacheableSizeThreshold)
      , _freeSegments( SegmentDequeAllocator(rawAllocator) )
      , _currentSegment( TR::ref(_segAllocator.allocate(cacheBlockSize)) )
      {
      TR_ASSERT_FATAL(cacheBlockSize > 0 && ((cacheBlockSize & (cacheBlockSize - 1)) == 0), "cacheBlockSize must be power of 2");
      TR_ASSERT_FATAL(cacheableSizeThreshold >= cacheBlockSize && ((cacheableSizeThreshold & (cacheableSizeThreshold - 1)) == 0), "cacheableSizeThreshold must be power of 2 and at least as large as cacheBlockSize");
      MEMLOG(1, "creation: rawAllocator %p segAllocator %p block size %lu size threshold %lu\n", &rawAllocator, &segAllocator, cacheBlockSize, cacheableSizeThreshold);
      MEMLOG(2, "allocated segment %p %lu\n", &(_currentSegment.get()), _currentSegment.get().size());
      }

   ~SegmentCache() throw()
      {
      // remove all segments from the free list
      //    delete each segment's TR::MemorySegment object because we allocated that directly
      //    NOTE: the memory pointed to by the segments is owned by _segAllocator so leave all that memory alone
      while (!_freeSegments.empty())
         {
         TR::MemorySegment &segment = _freeSegments.back().get();
         _freeSegments.pop_back();
         rawAllocator.deallocate(&segment);
         }

      _segAllocator.deallocate(_currentSegment.get());
      }

   // SegmentAllocator API:
   virtual size_t bytesAllocated() const throw()
      {
      return _bytesAllocated;
      }

   virtual size_t allocationLimit() const throw()
      {
      return _segAllocator.allocationLimit();
      }

   virtual TR::MemorySegment * allocate(size_t requiredSize, const std::nothrow_t tag, void * hint = 0) throw()
      {
      // round up to a multiple of the default allocation size
      size_t const roundedSize = ( ( ( requiredSize + (_cacheBlockSize - 1) ) / _cacheBlockSize ) * _cacheBlockSize );
      MEMLOG(1, "allocate request %lu rounded up to %lu\n", requiredSize, roundedSize);

      // is there a free segment in the cache that will fit the bill directly?
      if (!_freeSegments.empty() && roundedSize == _cacheBlockSize)
         {
         TR::MemorySegment &recycledSegment = _freeSegments.back();
         TR_ASSERT_FATAL(recycledSegment.size() == roundedSize, "unexpected size on free segment");
         _freeSegments.pop_back();
         recycledSegment.reset();
         _bytesAllocated += roundedSize;
         MEMLOG(2, "recyling free segment %p size %lu\n", &recycledSegment, recycledSegment.size());
         _allocatedSegments.push_front( TR::ref(recycledSegment) );
         return &recycledSegment;
         }

      // first, see if we need to allocate a new segment to satisfy the allocation
      size_t remainingSpace = _currentSegment.get().remaining();
      if (remainingSpace < roundedSize && roundedSize < _cacheableSizeThreshold)
         {
         MEMLOG(2, "current chunk only %lu remaining, need to allocate a new one\n", remainingSpace);

         // doesn't fit in current backing segment, but it would fit in a fresh segment
         // first split remaining space in current backing segment into small segments and put onto free list
         claimFreeSegments(_currentSegment.get());

         // allocate a fresh backing segment
         _currentSegment = TR::ref(_segAllocator.allocate(_cacheBlockSize));

         // update remainingSpace so allocation will come from the (new) current chunk
         remainingSpace = _currentSegment.get().remaining();
         MEMLOG(2, "new chunk %p allocated, remaining now %lu\n", &(_currentSegment.get()), remainingSpace);
         }

      // allocate from current chunk if it will fit
      if (remainingSpace >= roundedSize)
         {
         MEMLOG0(2, "allocation will come from current chunk\n");
         return allocateSmallSegmentFromBigSegment(roundedSize, _currentSegment.get());
         }

      // if we get here, then we need to allocate a segment specifically for this allocation request (because it's so big)
      TR::MemorySegment &newSegment = _segAllocator.allocate(roundedSize);
      MEMLOG(2, "need dedicated segment, allocated %p size %lu\n", &newSegment, newSegment.size());
      _allocatedSegments.push_front( TR::ref(newSegment) );
      _bytesAllocated += newSegment.size();
      return &newSegment;
      }

   virtual bool deallocate(TR::MemorySegment &segment) throw()
      {
      MEMLOG(1, "deallocate %p with size %lu request\n", &segment, segment.size());
      size_t const segmentSize = segment.size();
      _bytesAllocated -= segmentSize;
      segment.reset(); // claim the entire segment

      // from this point forward, segment should no longer be consider "allocated"
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

      if (!found)
         return false;

      auto ref = *segmentToRemove;
      MEMLOG(2, "deallocate %p ( %p ) was found in allocatedSegments (removing)\n", &segment, &(ref.get()));
      _allocatedSegments.erase(segmentToRemove);

      if (segmentSize < _segAllocator.allocationBlockSize())
         {
         // segment should be a multiple of _cacheBlockSize
         TR_ASSERT((segmentSize % _cacheBlockSize) == 0, "unexpected segment size");
         claimFreeSegments(segment);
         }
      else
         {
         // we allocated this segment specifically for a big allocation, so just free it
         MEMLOG(2, "large segment %p: deallocate via segment allocator\n", &segment);
         _segAllocator.deallocate(segment);
         }

      return true;
      }

   // additional SegmentCache API, most of this should become protected after refactoring across projects is complete:
   TR::SegmentAllocator & segAllocator() const
      {
      return _segAllocator;
      }

   size_t cacheBlockSize() const
      {
      return _cacheBlockSize;
      }

   // for compatibility, hopefully to be removed after refactoring across projects is complete
   size_t regionBytesAllocated() const
      {
      return bytesAllocated();
      }

   size_t systemBytesAllocated() const
      {
      return bytesAllocated();
      }

   void setAllocationLimit(size_t limit) const
      {
      /* made unnecessary */
      }

protected:

   // \brief divide the provided segments into segments of size _cacheBlockSize and place onto free list
   void claimFreeSegments(TR::MemorySegment & segment)
   {
      // small segments that match the block size can just be put directly onto free list
      if (segment.size() == _cacheBlockSize)
         {
         try
            {
            _freeSegments.push_front(TR::ref(segment));
            MEMLOG(2, "segment %p placed on free segment list\n", &segment);
            }
         catch (...)
            {
            // not much we can do here except leak
            }
         }
      else // need to split remaining space into cacheBlockSize segments and add to free list
         {
         MEMLOG(2, "segment %p will be broken down and placed onto free segment list\n", &segment);
         while (segment.remaining() > 0)
            {
            try
               {
               void *newSegmentArea = segment.allocate(_cacheBlockSize);
               createFreeSegmentFromArea(newSegmentArea, _cacheBlockSize);
               }
            catch (...)
               {
               // not much we can do here except leak
               }
            }
         }
      }

   // \brief helper to create a TR::MemorySegment object for the provided memory and size and puts it on the _freeSegments list
   TR::MemorySegment & createFreeSegmentFromArea(void *newSegmentArea, size_t size)
      {
      TR::MemorySegment *newSegment = new (rawAllocator) TR::MemorySegment(newSegmentArea, size);
      _freeSegments.push_front( TR::ref(*newSegment) );
      MEMLOG(1, "free segment %p claimed base %p size %lu\n", newSegment, newSegment->base(), newSegment->size());
      return *newSegment;
      }

   // \brief use part of segment to create a new TR::MemorySegment of the requested size
   TR::MemorySegment & allocateSmallSegmentFromBigSegment(size_t size, TR::MemorySegment &segment)
      {
      TR_ASSERT( (size % _cacheBlockSize) == 0, "Misaligned segment");
      void *newSegmentArea = segment.allocate(size);
      if (!newSegmentArea) throw std::bad_alloc();
      try
         {
         TR::MemorySegment &newSegment = allocateMemorySegment(newSegmentArea, size);
         return newSegment;
         }
      catch (...)
         {
         // one of the few scenarios where rewind() is safe and appropriate
         segment.rewind(size);
         throw;
         }
      }

   // \brief helper to create a TR::MemorySegment object for the provided memory and size and puts it on the _allocatedSegments list
   TR::MemorySegment & allocateMemorySegment(void *newSegmentArea, size_t size)
      {
      TR::MemorySegment *newSegment = new (rawAllocator) TR::MemorySegment(newSegmentArea, size);
      _allocatedSegments.push_front( TR::ref(*newSegment) );
      _bytesAllocated += size;
      MEMLOG(1, "allocated segment %p base %p size %lu\n", newSegment, newSegment->base(), newSegment->size());
      return *newSegment;
      }

   RawAllocator rawAllocator;            // used to allocate new TR::MemorySegment objects when needed
   TR::SegmentAllocator & _segAllocator; // used to allocate new segments
   size_t _cacheBlockSize;               // size of all segments on _freeSegments list
   size_t _cacheableSizeThreshold;       // only allocation requests below this size will be cached
   size_t _bytesAllocated;               // how many bytes are currently allocated by this cache

   typedef TR::typed_allocator<
      TR::reference_wrapper<TR::MemorySegment>,
      RawAllocator
      > SegmentDequeAllocator;

   // \brief list of allocated segments so that their TR::MemorySegment objects can be deallocated upon destruction
   // Used to prevent memory leaks
   std::deque<
      TR::reference_wrapper<TR::MemorySegment>,
      SegmentDequeAllocator
      > _allocatedSegments;

   // \brief list of free segments available to serve new allocation requests for <= _cacheBlockSize bytes
   // TR::MemorySegment objects used in this list must also be freed upon destruction to avoid leaking memory
   std::deque<
      TR::reference_wrapper<TR::MemorySegment>,
      SegmentDequeAllocator
      > _freeSegments;

   // \brief holds the current (large) segment that's been allocated by _segAllocator
   // The total size of the segment stored in _currentSegment is determined by the
   //  minimum allocation size of the SegmentAllocator provided on construction, but
   //  that size must be larger than _cacheBlockSize and _cacheableSizeThreshold.
   // Smaller allocation requests will be served from _currentSegment by carving it
   //  up into segments of no less than _cacheBlockSize and in multiples of _cacheBlockSize
   //  to simplify fragmentation effects..
   // Segments larger than _cacheableSizeThreshold are dedicated for a particular allocation
   //  and will be deallocated (via _segAllocator) when no longer needed.
   //  In particular, these larger segments are never placed on the free list.
   TR::reference_wrapper<TR::MemorySegment> _currentSegment;

   };

} // namespace OMR

#undef MEMLOG
#undef MEMLOG0

#endif // OMR_SEGMENTCACHE_INCL
