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

#ifndef OMR_NAIVESEGMENTALLOCATOR_INCL
#define OMR_NAIVESEGMENTALLOCATOR_INCL


#include <deque>
#include <stdint.h>
#include "infra/ReferenceWrapper.hpp"
#include "env/newmemory/RawAllocator.hpp"
#include "env/newmemory/MemorySegment.hpp"
#include "env/newmemory/SegmentAllocator.hpp"

// Memory logging macros
// Memory allocation can be used very early by the compiler: can rely on printf working

extern const char * NSA_LOGLEVEL;
// LOGLEVEL="0"    // no debug output
// LOGLEVEL="1"    // summary output, about one line per function call
// LOGLEVEL="2"    // extra detail in functions
// LOGLEVEL="3"    // highest level of detail in functions

#define MEMLOG(n,s, ...) do { if ((NSA_LOGLEVEL != NULL) && ((int)(*NSA_LOGLEVEL) >= (int)(n+'0'))) { printf("SegAll %p : " s, this, __VA_ARGS__); } } while (0)
#define MEMLOG0(n,s)     do { if ((NSA_LOGLEVEL != NULL) && ((int)(*NSA_LOGLEVEL) >= (int)(n+'0'))) { printf("SegAll %p : " s, this); } } while (0)


// NaiveSegmentAllocator<RawAllocator> is a TR::SegmentAllocator that straight-forwardly
// allocates MemorySegments when allocate() is called and frees them when deallocate()
// is called. It uses an embedded RawAllocator object.
//
// OMR::NaiveSegmentAllocator uses RawAllocator object to allocate both the underlying
// memory as well as the TR::MemorySegment object. This class is designed, however, to
// be subclassable to override the protected member functions directAllocate() and
// directDeallocate() to provide code to allocate/deallocate memory without necessarily
// using the RawAllocator object directly.
//
// This allocator rounds all allocations up to a "minimum" allocation size to avoid
// fragmentation from the backing RawAllocator. In typical configurations, this minimum
// allocation size will be set to something like 64KB or 1MB. An allocation limit can be
// optionally set to prevent excessive memory allocation.
//
// Every call to allocate() will definitely allocate a MemorySegment and calls to
// deallocate will definitely deallocate both the memory and the TR::MemorySegment
// object. Projects may subclass OMR::NaiveSegmentAllocator to change this behaviour,
// and even other subclasses in OMR (e.g. OMR::NaiveDebugSegmentAllocator) may not
// always deallocate segments when deallocate() is called.

namespace OMR
{

// The provided RawAllocator class is expected to implement TR::RawAllocator interface

template<class RawAllocator>
class NaiveSegmentAllocator : public TR::SegmentAllocator
   {
public:
   // \brief NaiveSegmentAllocator allocates memory segments and objects using the provided TR::RawAllocator
   NaiveSegmentAllocator(size_t minAllocationSize, size_t allocationLimit=SIZE_MAX)
      : rawAllocator()
      , _minAllocationSize(minAllocationSize)
      , _allocationLimit(allocationLimit)
      , _bytesAllocated(0)
      , _allocatedSegments( SegmentDequeAllocator(rawAllocator) )
      {
      TR_ASSERT(_minAllocationSize > 0 && _minAllocationSize && ((_minAllocationSize & (_minAllocationSize - 1)) == 0), "minAllocationSize must be power of 2");
      TR_ASSERT(_allocationLimit >= _minAllocationSize, "allocationLimit must be at least as large as minAllocationSize");
      MEMLOG(1, "creation  %p block size %lu limit %lu\n", this, _minAllocationSize, _allocationLimit);
      }

   virtual ~NaiveSegmentAllocator() throw()
      {
      MEMLOG0(1, "destructing\n");
      auto it = _allocatedSegments.begin();
      while (it != _allocatedSegments.end())
         {
         auto segmentToRemove = *it;
         MEMLOG(3, "\t_allocatedSegments iterating segment %p\n", &(segmentToRemove.get()));
         directDeallocate(segmentToRemove);
         _allocatedSegments.erase(it);
         it++;
         }

      MEMLOG(1, "after deallocating all segments, _bytesAllocated is %lu\n", _bytesAllocated);
      }

   // \brief returns the total number of bytes allocated in the currently allocated segments
   virtual size_t bytesAllocated() const throw()      { return _bytesAllocated; }

   // \brief returns the allocation limit assigned when this object was created
   virtual size_t allocationLimit() const throw()     { return _allocationLimit; }

   // \brief returns the minimum allocation block size used by the SegmentAllocator object
   virtual size_t allocationBlockSize() const throw() { return _minAllocationSize; }

   // \brief allocate a segment of at least the requested size
   virtual TR::MemorySegment * allocate(size_t requestedSize, const std::nothrow_t tag, void * hint = 0) throw()
      {
      MEMLOG(1, "allocate request %lu\n", requestedSize);

      // round up to at least the minimum allocation size
      size_t const roundedSize = (requestedSize > _minAllocationSize) ? requestedSize : _minAllocationSize;

      // check that we're allowed to allocate a new segment without blowing our allocation limit
      size_t alreadyAllocated = bytesAllocated();
      if (alreadyAllocated + roundedSize > _allocationLimit)
         {
         MEMLOG(1, "\trounded size %lu on top of already allocated %lu would exceed allocation limit %lu, returning NULL\n", roundedSize, alreadyAllocated, _allocationLimit);
         return NULL;
         }

      TR::MemorySegment *segment = directAllocate(roundedSize, hint);
      if (segment)
         {
         // remember the segment we allocated so we can make sure it's deallocated on destruction
         try
            {
            _allocatedSegments.push_back(TR::ref(*segment));
            }
         catch (...)
            {
            MEMLOG(1, "\tunexpected exception recording segment %p in _allocatedSegment list, for safety will deallocate and return NULL\n", segment);
            directDeallocate(*segment);
            return NULL;
            }
         }

      MEMLOG(1, "\treturning %p, size %lu\n", segment, (segment ? segment->size() : 0));
      return segment;
      }

   // \brief deallocate a segment previously allocated by this allocator
   virtual bool deallocate(TR::MemorySegment & segment) throw()
      {
      MEMLOG(1, "deallocating segment %p\n", &segment);

      auto it = _allocatedSegments.begin();
      while (it != _allocatedSegments.end())
         {
         auto seg = *it;
         if (&(seg.get()) == &segment)
            {
            _allocatedSegments.erase(it);
            directDeallocate(segment);
            return true;
            }

         it++;
         }
      return false;
      }

protected:

   // Below "direct" functions handle only the mechanics of segment allocation/deallocation so
   //  it can be easily overridden in subclasses
   // These intentionally cannot be called directly by users of NaiveSegmentAllocator because
   //  they do not record allocated segments into the _allocatedSegments list

   // \brief allocate backing memory to hold roundedSize bytes inside a new TR::MemorySegment object
   virtual TR::MemorySegment * directAllocate(size_t roundedSize, void * hint = 0) throw()
      {
      TR::MemorySegment *segment = NULL;
      TR::RawMemory segmentMemory = rawAllocator.allocate(roundedSize);
      if (segmentMemory)
         {
         segment = new (rawAllocator, std::nothrow) TR::MemorySegment(segmentMemory, roundedSize);
         if (segment)
            _bytesAllocated += roundedSize + sizeof(TR::MemorySegment);
         else
            rawAllocator.deallocate(segmentMemory);
         }

      return segment;
      }

   // \brief deallocate the provided TR::MemorySegment backing memory and the TR::MemorySegment object
   virtual void directDeallocate(TR::MemorySegment & segment)
      {
      size_t totalSize = segment.size() + sizeof(TR::MemorySegment);

      // deallocate the TR::RawMemory pointed at by the TR::MemorySegment
      rawAllocator.deallocate(segment.base());

      // delete the TR::MemorySegment object itself
      rawAllocator.deallocate(static_cast<void *>(&segment));

      _bytesAllocated -= totalSize;
      }

   RawAllocator rawAllocator;
   size_t _minAllocationSize;
   size_t _allocationLimit;
   size_t _bytesAllocated;

   typedef TR::typed_allocator<
      TR::reference_wrapper<TR::MemorySegment>,
      RawAllocator
      > SegmentDequeAllocator;

   std::deque<
      TR::reference_wrapper<TR::MemorySegment>,
      SegmentDequeAllocator
      > _allocatedSegments;
   };

} // namespace TR

#undef MEMLOG
#undef MEMLOG0

#endif // defined(OMR_NAIVESEGMENTALLOCATOR_INCL)
