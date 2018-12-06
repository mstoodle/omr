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


#include <stdlib.h>
#include "codegen/FrontEnd.hpp"                  // for feGetEnv

// Memory logging support
// Memory allocation can be used very early by the compiler: can rely on printf working

static char *LOGLEVEL = ::feGetEnv("OMRDebug_BackingMemory");

// LOGLEVEL="0"    // no debug output
// LOGLEVEL="1"    // summary output, about one line per function call
// LOGLEVEL="2"    // extra detail in functions
// LOGLEVEL="3"    // highest level of detail in functions

#define MEMLOG(n,s, ...) do { if ((LOGLEVEL != NULL) && ((int)(*LOGLEVEL) >= (int)(n+'0'))) { printf("BckMem %p : " s, this, __VA_ARGS__); } } while (0)
#define MEMLOG0(n,s)     do { if ((LOGLEVEL != NULL) && ((int)(*LOGLEVEL) >= (int)(n+'0'))) { printf("BckMem %p : " s, this); } } while (0)


#include "env/mem/BackingMemoryAllocator.hpp"


TR::BackingMemoryAllocator &
OMR::BackingMemoryAllocator::clone()
   {
   return *(new (_rawAllocator) TR::BackingMemoryAllocator(*(static_cast<TR::BackingMemoryAllocator *>(this))));
   }

TR::MemorySegment &
OMR::BackingMemoryAllocator::allocate(size_t requiredSize, void * hint)
   {
   size_t sizeToAllocate = std::max(requiredSize, _minimumAllocationSize);
   MEMLOG(1, "allocate request %lu will allocate at least %lu\n", requiredSize, sizeToAllocate);

   if (_allocationLimit > 0 && _bytesAllocated + sizeToAllocate > _allocationLimit)
      {
      MEMLOG(1, "allocation would exceed limit %lu!\n", _allocationLimit);
      throw std::bad_alloc();
      }

   void *p = _rawAllocator.allocate(sizeToAllocate, hint);
   TR::MemorySegment *newSegment = new (_rawAllocator) TR::MemorySegment(p, sizeToAllocate);
   try
      {
      _allocatedSegments.push_front( TR::ref(*newSegment) );
      }
   catch (...)
      {
      _rawAllocator.deallocate(p, sizeToAllocate);
      throw;
      }
   _bytesAllocated += sizeToAllocate;
   return *newSegment;
   }

void
OMR::BackingMemoryAllocator::deallocate(TR::MemorySegment & segment) throw()
   {
   MEMLOG(1, "deallocate %p with size %lu request\n", &segment, segment.size());

   bool found=false;
   for (auto it = _allocatedSegments.begin(); it != _allocatedSegments.end(); it++)
      {
      auto ref = *it;
      MEMLOG(3, "_allocatedSegments iterating segment %p\n", &(ref.get()));
      if (segment == ref.get())
         {
         _allocatedSegments.erase(it);
         found=true;
         break;
         }
      }
   TR_ASSERT_FATAL(found, "Request to deallocate segment not found in allocated segment list");

   _bytesAllocated -= segment.size();

   // deallocate actual segment memory
   _rawAllocator.deallocate(segment.base(), segment.size());

   // deallocate the memory that held the TR::MemorySegment
   _rawAllocator.deallocate(&segment, sizeof(TR::MemorySegment));;
   }

void
OMR::BackingMemoryAllocator::deallocateSegments()
   {
   while (!_allocatedSegments.empty())
      {
      TR::MemorySegment &segment = _allocatedSegments.front().get();
      deallocate(segment);
      }

   TR_ASSERT(_bytesAllocated == 0, "Unexpectedly _bytesAllocated not equal to zero after freeing all segments");
   }

void *
operator new(size_t size, TR::MemorySegment &segment) throw()
   {
   if (segment.size() < size) return NULL;
   return segment.base();
   }

void *operator new[](size_t size, TR::MemorySegment &segment) throw()
   {
   return operator new(size, segment);
   }

void operator delete(void *, TR::MemorySegment &segment) throw()
   {
   }

void operator delete[](void *ptr, TR::MemorySegment  &segment) throw()
   {
   operator delete(ptr, segment);
   }
