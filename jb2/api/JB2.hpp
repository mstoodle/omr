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

#ifndef JB2_INCL
#define JB2_INCL

#include <cassert>
#include <cstddef>

namespace OMR {
namespace JitBuilder {

class Compiler;

// eventually shift memory tracking to Compiler using an object rather than static
class JB2 {
    friend class Compiler;

public:

    template<typename T>
    static T *allocate(size_t numItems) {
        size_t bytes = numItems * sizeof(T);
        if (bytes == 0)
            return NULL;

        T *p = (T *) malloc(bytes);
        assert(p != NULL);
        _totalAllocations++;
        _totalAllocated += bytes;
        return p;
    }

    template<typename T>
    static void deallocate(T *ptr, size_t numItems) {
        assert(ptr != NULL);
        free(ptr);
        _totalDeallocations++;
        _totalDeallocated += numItems * sizeof(T);
    }

    static void report();

protected:
    JB2();

    static size_t _totalAllocated;
    static size_t _totalDeallocated;

    static size_t _totalAllocations;
    static size_t _totalDeallocations;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(JB2_INCL)
