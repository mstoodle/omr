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

#ifndef TR_SEGMENTALLOCATOR_INCL
#define TR_SEGMENTALLOCATOR_INCL

#pragma once

#include <stddef.h>  // for size_t
#include <new>       // for bad_alloc, nothrow, nothrow_t
#include "env/TypedAllocator.hpp"


namespace TR {

class MemorySegment;

// SegmentAllocator is an abstract class used by the rest of the compiler to interact
// with MemorySegment allocators (e.g. concrete subclasses like NaiveSegmentAllocator,
// SegmentCache, DebugSegmentAllocator, etc.). SegmentAllocator objects must track all
// memory they allocate and ensure that any memory that has not been explicitly deallocated
// is deallocated when the SegmentAllocator object is destructed. SegmentAllocator objects
// are not required to be thread-safe so explicit locking is needed if multiple threads
// are able to access a single SegmentAllocator object simultaneously.
//
// The main distinguishing feature of SegmentAllocator versus RawAllocator is that
// SegmentAllocator deals in TR::MemorySegment objects rather than RawMemory pointers.

class SegmentAllocator
   {
public:
   SegmentAllocator()
      { }

   virtual ~SegmentAllocator() throw()
      { }

   // \brief returns how many *total* memory bytes are currently allocated through this SegmentAllocator object
   virtual size_t bytesAllocated() const throw() = 0;

   // \brief returns the limit placed on the SegmentAllocator object, which could be SIZE_MAX (analogous to "no limit")
   virtual size_t allocationLimit() const throw() = 0;

   // \brief returns the minimum allocation block size used by the SegmentAllocator object
   virtual size_t allocationBlockSize() const throw() = 0;

   // \brief tries to allocate a TR::MemorySegment pointing at memory of at least size bytes
   // On failure returns NULL
   virtual TR::MemorySegment * allocate(size_t size, const std::nothrow_t tag, void * hint = 0) throw() = 0;

   // \brief tries to allocate a TR::MemorySegment pointing at memory of at least size bytes
   // On failure throws std::bad_alloc()
   TR::MemorySegment & allocate(size_t size, void * hint = 0)
      {
      TR::MemorySegment * const alloc = allocate(size, std::nothrow, hint);
      if (!alloc) throw std::bad_alloc();
      return *alloc;
      }

   // \brief deallocate the given MemorySegment including the MemorySegment object
   // \returns true if segment was deallocated, false otherwise
   virtual bool deallocate(TR::MemorySegment & p) throw() = 0;
   };

}

#endif // defined(TR_SEGMENTALLOCATOR_INCL)
