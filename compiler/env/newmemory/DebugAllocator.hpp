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

#ifndef OMR_DEBUGALLOCATOR_INCL
#define OMR_DEBUGALLOCATOR_INCL

#pragma once

#include "env/newmemory/MallocAllocator.hpp"

// OMR::DebugAllocator is used when debugging use-after-free problems. When used in conjunction
// with a TR::DebugSegmentAllocator, deallocated memory will not actually be deallocated but
// rather access protected against reads (implemented inside the noLongerUsed() notification, which
// should be called by DebugSegmentAllocator at the same time deallocate() would normally have been
// called). Only when the DebugSegmentAllocator object dies will the memory segments be returned via
// deallocate(). While not memory efficient (because it holds onto allocated memory for longer) it
// can be extremely useful to catch use-after-free memory bugs.
//
// NOTE: noLongerUsed() is not really the right way to implement this "access protection" mechanism.
//       The longer term solution here moves the noLongerUsed() implementation into deallocate()
//       while adding a std::deque to remember the memory region (and NOT actually freeing the memory).
//       In the destructor, then, walk the list and actually deallocate the memory regions. This
//       approach makes the DebugRawAllocator stateful, which conflicts with an assumption in a few
//       places in the JIT that RawAllocators are, in fact, stateless (they are copied by value).
//       Once these spots have been updated, this more proper solution should be performed. At that
//       point, parts of DebugSegmentAllocator should also melt away.
//

namespace OMR
{

class DebugAllocator : public OMR::MallocAllocator
   {
public:
   DebugAllocator()
      : OMR::MallocAllocator()
      { }

   virtual TR::RawAllocator & clone()
      {
      return *(new (*this) DebugAllocator());
      }

   virtual RawMemory allocate(size_t size, const std::nothrow_t tag,void * hint = 0) const throw();
   virtual void deallocate(RawMemory p) const throw();
   virtual void deallocate(RawMemory p, const size_t size) const throw();

   // 
   virtual void noLongerUsed(RawMemory p, const size_t size) const throw();
   };

} // namespace OMR

#endif // OMR_DEBUGALLOCATOR_INCL
