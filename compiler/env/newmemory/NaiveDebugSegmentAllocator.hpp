/*******************************************************************************
 * Copyright (c) 2017, 2019 IBM Corp. and others
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

#ifndef OMR_NAIVEDEBUGSEGMENTALLOCATOR_INCL
#define OMR_NAIVEDEBUGSEGMENTALLOCATOR_INCL

#pragma once

#include "codegen/FrontEnd.hpp"
#include "env/newmemory/DebugAllocator.hpp"
#include "env/newmemory/NaiveSegmentAllocator.hpp"

// Memory logging macros
// Memory allocation can be used very early by the compiler: can rely on printf working

extern const char * NDSA_LOGLEVEL;
// LOGLEVEL="0"    // no debug output
// LOGLEVEL="1"    // summary output, about one line per function call
// LOGLEVEL="2"    // extra detail in functions
// LOGLEVEL="3"    // highest level of detail in functions

#define MEMLOG(n,s, ...) do { if ((NDSA_LOGLEVEL != NULL) && ((int)(*NDSA_LOGLEVEL) >= (int)(n+'0'))) { printf("dbgSegAll %p : " s, this, __VA_ARGS__); } } while (0)
#define MEMLOG0(n,s)     do { if ((NDSA_LOGLEVEL != NULL) && ((int)(*NDSA_LOGLEVEL) >= (int)(n+'0'))) { printf("dbgSegAll %p : " s, this); } } while (0)

/** @class NaiveDebugSegmentAllocator
 *  @brief The NaiveDebugSegmentAllocator class provides a facility for verifying the
 *  correctness of compiler scratch memory use.
 *
 *  Using the native facilities available on each platform, a NaiveDebugSegmentAllocator
 *  provides an alternative allocation mechanism for the compiler's scratch memory
 *  regions.  Instead of releasing virtual address segments back to the operating
 *  system, this implementation instead either remaps the segment, freeing the underlying
 *  physical pages, and changing the memory protection to trap on any access [preferred],
 *  or, if such facilities are not available, paints the memory segment with a value that
 *  should cause pointer dereferences to be unaligned, and resolve to the high-half of the
 *  virtual address space often reserved for kernel / supervisor use.
 *
 *  Once the compiler has been modified to allow stateful RawAllocator objects,
 *  the functionality in this class is expected to sink into DebugAllocator and this
 *  class will no longer be required over and above
 *  OMR::NaiveSegmentAllocator<DebugAllocator>. Until then, it's needed to track the
 *  "deallocated" segments until destruction time.
 **/

namespace OMR
{

class NaiveDebugSegmentAllocator : public OMR::NaiveSegmentAllocator<OMR::DebugAllocator>
   {
public:
   NaiveDebugSegmentAllocator(size_t minAllocationSize, size_t allocationLimit=SIZE_MAX)
      : OMR::NaiveSegmentAllocator<OMR::DebugAllocator>(minAllocationSize, allocationLimit)
      , _releasedSegments( SegmentDequeAllocator(rawAllocator) )
      { }
   virtual ~NaiveDebugSegmentAllocator()
      {
      MEMLOG0(1, "destructing and freeing all released segments\n");
      auto it = _releasedSegments.begin();
      while (it != _releasedSegments.end())
         {
         auto segmentToRemove = *it;
         MEMLOG(3, "\t_releasedSegments iterating segment %p\n", &(segmentToRemove.get()));
         directDeallocate(segmentToRemove);
         _releasedSegments.erase(it);
         it++;
         }
      MEMLOG(1, "after deallocating all released segments, _bytesAllocated is %lu\n", _bytesAllocated);
      }

protected:
   virtual void directDeallocate(TR::MemorySegment & segment) throw()
      {
      MEMLOG(1, "deallocate request for segment %p, marking as no longer in use\n", &segment);
      rawAllocator.noLongerUsed(segment.base(), segment.size());
      rawAllocator.noLongerUsed(&segment, sizeof(TR::MemorySegment));
      try
         {
         _releasedSegments.push_back(TR::ref(segment));
         }
      catch (...)
         {
         MEMLOG(1, "\tunexpected exception recording segment %p in released segment list, for safety deallocating segment\n", &segment);
         directDeallocate(segment);
         }
      }

   typedef TR::typed_allocator<
      TR::reference_wrapper<TR::MemorySegment>,
      OMR::DebugAllocator
      > SegmentDequeAllocator;

   std::deque<
      TR::reference_wrapper<TR::MemorySegment>,
      SegmentDequeAllocator
      > _releasedSegments;
   };

} // namespace OMR

#undef MEMLOG
#undef MEMLOG0

#endif // OMR_NAIVEDEBUGSEGMENTALLOCATOR_INCL
