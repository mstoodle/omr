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

#ifndef OMR_PERSISTENTALLOCATOR_INCL
#define OMR_PERSISTENTALLOCATOR_INCL

#pragma once

namespace TR { class Monitor; }
namespace TR { class MonitorTable; }

#include <deque>
#include <new>
#include "env/newmemory/MemorySegment.hpp"
#include "env/newmemory/RawAllocator.hpp"
#include "env/newmemory/SegmentAllocator.hpp"
#include "env/TypedAllocator.hpp"
#include "il/DataTypes.hpp"
#include "infra/Monitor.hpp"
#include "infra/MonitorTable.hpp"
#include "infra/ReferenceWrapper.hpp"

inline size_t my_round(size_t size)
   {
#if defined(TR_HOST_64BIT) || defined(FIXUP_UNALIGNED)
   return (size+7) & (~7);
#else
   return (size+3) & (~3);
#endif
   }


namespace OMR {

template <class MyRawAllocator>
class PersistentAllocator
   {
public:
   PersistentAllocator(TR::SegmentAllocator & segmentAllocator)
      : rawAllocator()
      , _segmentAllocator(segmentAllocator)
      , _segments(SegmentDequeAllocator(rawAllocator))
      {
      for (int32_t b=0;b < PERSISTANT_BLOCK_SIZE_BUCKETS;b++)
         _reusableBlocks[b] = NULL;
      }

   virtual ~PersistentAllocator() throw()
      {
      while (!_segments.empty())
         {
         TR::MemorySegment &segment = _segments.front();
         _segments.pop_front();
         _segmentAllocator.deallocate(segment);
         }
      }

   virtual void *allocate(size_t size, const std::nothrow_t tag, void * hint = 0) throw()
      {
      TR::Monitor * memoryAllocMonitor = NULL;
      TR::MonitorTable * monitorTable = TR::MonitorTable::get();
      if (monitorTable)
         memoryAllocMonitor = monitorTable->getMemoryAllocMonitor();

      if (memoryAllocMonitor)
         memoryAllocMonitor->enter();

      void * result = allocateLocked(size);

      if (memoryAllocMonitor)
         memoryAllocMonitor->exit();

      return result;
      }

   void *allocate(size_t size, void * hint = 0)
      {
      void * alloc = allocate(size, std::nothrow, hint);
      if (!alloc) throw std::bad_alloc();
      return alloc;
      }

   virtual void deallocate(void * p, size_t sizeHint = 0) throw()
      {
      TR::Monitor * memoryAllocMonitor = NULL;
      TR::MonitorTable * monitorTable = TR::MonitorTable::get();
      if (monitorTable)
         memoryAllocMonitor = monitorTable->getMemoryAllocMonitor();

      if (memoryAllocMonitor)
         memoryAllocMonitor->enter();

      Block * block = static_cast<Block *>(p) - 1;

      // adjust the used persistent memory here and not in freePersistentMemory(block, size)
      // because that call is also used to free memory that wasn't actually committed
      //TR::AllocatedMemoryMeter::update_freed(block->_size, persistentAlloc);

      freeBlock(block);

      if (memoryAllocMonitor)
         memoryAllocMonitor->exit();
      }

protected:

   // Persistent block header
   //
   struct Block
      {
      size_t _size;
      Block * _next;

      explicit Block(size_t size, Block * next = 0) : _size(size), _next(next) {}
      Block * next() { return reinterpret_cast<Block *>( (reinterpret_cast<uintptr_t>(_next) & ~0x1)); }
      };

   static const size_t PERSISTANT_BLOCK_SIZE_BUCKETS = 12;
   static size_t computeBlockIndex(size_t const blockSize)
      {
      size_t const adjustedBlockSize = blockSize - sizeof(Block);
      size_t const candidateBucket = adjustedBlockSize / sizeof(void *);
      return candidateBucket < PERSISTANT_BLOCK_SIZE_BUCKETS ?
         candidateBucket :
         0;
      }

   void * allocateLocked(size_t const requestedSize)
      {
      TR_ASSERT( sizeof(Block) == my_round( sizeof(Block) ),"Persistent block size will prevent us from properly aligning allocations.");
      size_t const dataSize = my_round(requestedSize);
      size_t const allocSize = sizeof(Block) + dataSize;

      //TR::AllocatedMemoryMeter::update_allocated(allocSize, persistentAlloc);

      // If this is a small block try to allocate it from the appropriate
      // fixed-size-block chain.
      //
      size_t const index = computeBlockIndex(allocSize);
      Block * block = 0;
      Block * prev = 0;
      for (
         block = _reusableBlocks[index];
         block && block->_size < allocSize;
         prev = block, block = prev->next()
         )
         {
         TR_ASSERT(index == 0, "Iterating through a fixed-size block bin.");
         }

      if (block)
         {
         TR_ASSERT(
            ( index == 0 ) || ( block->_size == allocSize ),
            "block %p in chain for index %d has size %d (not %d)\n",
            block,
            index,
            block->_size,
            (index * sizeof(void *)) + sizeof(Block)
            );

         if (prev)
            prev->_next = block->next();
         else
            _reusableBlocks[index] = block->next();

         block->_next = NULL;

         size_t const excess = block->_size - allocSize;

         if (excess > sizeof(Block))
            {
            block->_size = allocSize;
            freeBlock( new (pointer_cast<uint8_t *>(block) + allocSize) Block(excess) );
            }

         //_bytesAllocated += allocSize;
         return block + 1;
         }

      // Find the first persistent segment with enough free space
      TR::MemorySegment *segment = findUsableSegment(allocSize);
      if (!segment)
         {
         segment = _segmentAllocator.allocate(allocSize, std::nothrow);
         if (!segment)
            return NULL;

         try
            {
            _segments.push_front(TR::ref(*segment));
            }
         catch(const std::exception &e)
            {
            _segmentAllocator.deallocate(*segment);
            return NULL;
            }
         }

      TR_ASSERT(segment && segment->remaining() >= allocSize, "Failed to acquire a segment");
      block = new (segment->allocate(allocSize)) Block(allocSize);
      //_bytesAllocated += allocSize;
      return block + 1;
      }

   void freeBlock(Block *block)
      {
      TR_ASSERT(block->_size > 0, "Block size is non-positive");
      TR_ASSERT(block->_next == NULL, "In-use persistent memory block @ belongs to a free block chain.", block);
      block->_next = NULL;

      // If this is a small block, add it to the appropriate fixed-size-block
      // chain. Otherwise add it to the variable-size-block chain which is in
      // ascending size order.
      //
      size_t const index = computeBlockIndex(block->_size);
      Block * blockIterator = _reusableBlocks[index];
      if (!blockIterator || !(blockIterator->_size < block->_size) )
         {
         block->_next = _reusableBlocks[index];
         _reusableBlocks[index] = block;
         }
      else
         {
         TR_ASSERT(index == 0, "Iterating through what should be fixed-size blocks.");
         while (blockIterator->next() && blockIterator->next()->_size < block->_size)
            {
            blockIterator = blockIterator->next();
            }
         block->_next = blockIterator->next();
         blockIterator->_next = block;
         }
      }

   TR::MemorySegment * findUsableSegment(size_t requiredSize)
      {
      for (auto i = _segments.begin(); i != _segments.end(); ++i)
         {
         TR::MemorySegment &candidate = *i;
         if ( candidate.remaining() >= requiredSize )
            {
            return &candidate;
            }
         }
      return NULL;
      }

   MyRawAllocator rawAllocator;

   TR::SegmentAllocator & _segmentAllocator;

   Block * _reusableBlocks[PERSISTANT_BLOCK_SIZE_BUCKETS];

   typedef TR::typed_allocator<
      TR::reference_wrapper<TR::MemorySegment>,
      MyRawAllocator
      > SegmentDequeAllocator;

   std::deque<
      TR::reference_wrapper<TR::MemorySegment>,
      SegmentDequeAllocator
      > _segments;
   };

}

// Consuming projects: if you want to override the specific MyRawAllocator class used, then
// create your own PersistentAllocator.hpp class, create your own version of the following
// block that creates the TR::PersistentAllocator class extending your preferred
// implementation, and define TR_PERSISTENTALLOCATOR so that the block below will be
// conditionally compiled out when you #include this PersistentAllocator.hpp .

#ifndef TR_PERSISTENTALLOCATOR
#define TR_PERSISTENTALLOCATOR

#include "env/newmemory/MallocAllocator.hpp"

namespace TR
{
   class PersistentAllocator : public OMR::PersistentAllocator<OMR::MallocAllocator>
      {
      public:
      PersistentAllocator(TR::SegmentAllocator & segmentAllocator)
         : OMR::PersistentAllocator<OMR::MallocAllocator>(segmentAllocator)
         { }
      virtual ~PersistentAllocator() throw()
         { }
      };

}

void *operator new(size_t size, TR::PersistentAllocator & persistentAllocator);
void *operator new[](size_t size, TR::PersistentAllocator & persistentAllocator);

void operator delete(void *ptr, TR::PersistentAllocator & persistentAllocator) throw();
void operator delete[](void * ptr, TR::PersistentAllocator & persistentAllocator) throw();

#endif // defined(TR_PERSISTENTALLOCATOR)

#endif // defined(OMR_PERSISTENTALLOCATOR_INCL)
