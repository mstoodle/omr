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

#include <iostream>
#include "JB2.hpp"

using namespace std;

namespace OMR {
namespace JitBuilder {

#if 0
JB2::JB2()
    : _totalAllocated(0)
    , _totalDeallocated(0)
    , _totalAllocations(0)
    , _totalDeallocations(0) {
#endif

size_t JB2::_totalAllocated = 0;
size_t JB2::_totalDeallocated = 0;

size_t JB2::_totalAllocations = 0;
size_t JB2::_totalDeallocations = 0;

void
JB2::report() {
    cerr << _totalAllocated << "\ttotal bytes allocated\n";
    cerr << _totalDeallocated << "\ttotal bytes deallocated\n\n";
    cerr << _totalAllocations << "\ttotal allocations\n";
    cerr << _totalDeallocations << "\ttotal deallocations\n";

    assert(_totalDeallocated <= _totalAllocated);
    assert(_totalDeallocations <= _totalAllocations);
    size_t unfreedMemory = _totalAllocated - _totalDeallocated;
    if (unfreedMemory > 0)
        cerr << unfreedMemory << "\tmemory not freed\n";
    else {
        if (_totalDeallocations < _totalAllocations)
            cerr << "All memory freed, but unexpected mismatch in deallocations (" << _totalDeallocations << ") versus allocations (" << _totalAllocations << ")\n";
    }
}

} // namespace JitBuilder
} // namespace OMR
