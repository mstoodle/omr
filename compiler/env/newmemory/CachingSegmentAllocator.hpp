/*******************************************************************************
 * Copyright (c) 2019, 2019 IBM Corp. and others
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

#ifndef OMR_CACHINGSEGMENTALLOCATOR_INCL
#define OMR_CACHINGSEGMENTALLOCATOR_INCL

#include "env/newmemory/SegmentAllocator.hpp"

namespace TR { class MemorySegment; }

/* @class CachingSegmentAllocator
 * @brief A CachingSegmentAllocator is a TR::SegmentAllocator that attempts to avoid allocating segments
 * by first using any segments available from a SegmentCache. Only if the SegmentCache does not provide a
 * segment will the CachingSegmentAllocator attempt to allocate using a downstream TR::SegmentAllocator object.
 * The use of this class enables a SegmentCache to be used to allocate long-lived objects (via its own
 * SegmentAllocator) as well as a shorter-lived SegmentAllocator to be used when the SegmentCache is full.
 * The typical use case is to use a long-lived SegmentCache to hold a small number of large segments for
 * reuse across all compilations for a period of time. "Long-lived" here means lives across compilations.
 * A second SegmentCache can then be created per compilation. This shorter-lived SegmentCache needs to
 * allocate segments somehow: it can use a CachingSegmentAllocator to prefer using segments from the long-
 * lived cache until they are used up. Any subsequent segments will be allocated from the CachingSegmentAllocator's
 * downstream segment allocator (which can have compilation lifetime as well).
 */

namespace OMR {

class CachingSegmentAllocator : TR::SegmentAllocator
   {
public:
   CachingSegmentAllocator(TR::SegmentAllocator & chainedCache, TR::SegmentAllocator & downstreamAllocator)
      : TR::SegmentAllocator()
      , _cache(chainedCache)
      , _allocator(downstreamAllocator)
      { }
   ~CachingSegmentAllocator() throw()

   virtual TR::MemorySegment * allocate(size_t size, void * hint = 0) throw();
      {
      TR::MemorySegment *segment = _cache->allocate(requestedSize, tag, hint);
      if (segment == NULL)
         segment = _allocator->allocate(requestedSize, tag, hint);
      return segment;
      }

   virtual bool deallocate(TR::MemorySegment &segment) throw();
      {
      bool freed = _cache.deallocate(segment);
      if (!freed)
         freed = _allocator.deallocate(segment);
      return freed;
      }

protected:
   TR::SegmentAllocator & _cache;
   TR::SegmentAllocator & _allocator;
   };

} // namespace OMR

#endif // OMR_CACHINGSEGMENTALLOCATOR_INCL
