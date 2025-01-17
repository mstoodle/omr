/*******************************************************************************
 * Copyright (c) 2022, 2022 IBM Corp. and others
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

#ifndef ALLOCATORTRACKER_INCL
#define ALLOCATORTRACKER_INCL

#include <cassert>
#include "Allocator.hpp"

namespace OMR {
namespace JB2 {

class TextLogger;

class AllocatorTracker : public Allocator {
    JBALLOC_(AllocatorTracker)

public:
    AllocatorTracker(const char * name, Allocator * trackedAllocator, TextLogger * lgr)
        : Allocator(name, trackedAllocator)
        , _lgr(lgr)
        , _totalAllocatedBytes(0)
        , _totalDeallocatedBytes(0)
        , _totalAllocations(0)
        , _totalDeallocations(0) {

        assert(trackedAllocator != NULL);
    }

    virtual void * allocate(size_t size, AllocationCategoryID cat);
    virtual void deallocate(void *ptr);

    virtual bool verify() const;

protected:
    virtual void log() const;

    TextLogger * _lgr;
    size_t _totalAllocatedBytes;
    size_t _totalDeallocatedBytes;

    size_t _totalAllocations;
    size_t _totalDeallocations;
};

} // namespace JB2
} // namespace OMR

#endif // defined(ALLOCATORTRACKER_INCL)
