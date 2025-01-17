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

#include <cassert>
#include "AllocatorTracker.hpp"
#include "Compiler.hpp"
#include "Config.hpp"
#include "TextLogger.hpp"

namespace OMR {
namespace JB2 {

INIT_JBALLOC_ON(AllocatorTracker, Allocator)

AllocatorTracker::~AllocatorTracker() {
    verify();
}

void *
AllocatorTracker::allocate(size_t size, AllocationCategoryID cat) {
    size_t amount = allocationAmount(size);
    _totalAllocations++;
    _totalAllocatedBytes += amount;
    return _parent->allocate(size, cat);
}

void
AllocatorTracker::deallocate(void *ptr) {
    _totalDeallocations++;
    _totalDeallocatedBytes += reinterpret_cast<Allocatable *>(ptr)->allocatedSize();
    _parent->deallocate(ptr);
}

void
AllocatorTracker::log() const {
    TextLogger &lgr = *_lgr;
    lgr << _totalAllocations << "\t total allocations" << lgr.endl();
    lgr << _totalDeallocations << "\t total deallocations" << lgr.endl();
    lgr << _totalAllocatedBytes << "\t total allocated bytes" << lgr.endl();
    lgr << _totalDeallocatedBytes << "\t total deallocated bytes" << lgr.endl();
}

bool
AllocatorTracker::verify() const {
    bool ok =
        (_totalAllocations == _totalDeallocations) &&
        (_totalAllocatedBytes == _totalDeallocatedBytes);
    if (_lgr) {
        TextLogger & lgr = *_lgr;
        if (!ok) {
            lgr << "Allocation verification failed!" << lgr.endl();
            log();
            assert(0);
        } else {
            lgr << ("Allocation verification passed!\n");
            log();
        }
    }
    return ok;
}

} // namespace JB2
} // namespace OMR
