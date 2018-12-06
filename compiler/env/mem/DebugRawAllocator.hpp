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

#ifndef TR_DEBUGRAWALLOCATOR_INCL
#define TR_DEBUGRAWALLOCATOR_INCL

#pragma once

namespace OMR { class DebugRawAllocator; }
namespace TR { using OMR::DebugRawAllocator; }

#include "env/mem/RawAllocator.hpp"

namespace OMR {

class DebugRawAllocator : public TR::RawAllocator
   {
public:
   typedef void * RawSegment;

   DebugRawAllocator()
      : TR::RawAllocator()
      { }

   DebugRawAllocator(const DebugRawAllocator &other)
      : TR::RawAllocator(other)
      { }

   virtual TR::RawAllocator & clone();

   RawSegment allocate(size_t size, void * hint = 0);

   virtual void deallocate(RawSegment p) throw();
   virtual void deallocate(RawSegment p, const size_t size) throw();
   virtual void protect(RawSegment p, const size_t size) throw();
   };

}

#endif // TR_DEBUGRAWALLOCATOR_INCL
