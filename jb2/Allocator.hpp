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

#ifndef ALLOCATOR_INCL
#define ALLOCATOR_INCL

#include <cassert>
#include <cstdint>
#include "Allocatable.hpp"


namespace OMR {
namespace JB2 {

class Config;

class Allocator : public Allocatable {

private:
    static AllocationCategoryID allocCategory;
    static bool allocCategoryInitialized;

    friend class Config;

public:
    static AllocationCategoryID allocCat();

    ALL_ALLOC_ALLOWED(Allocator, const char * name="", Allocator * parent=NULL, size_t minAllocationSize=1);
    virtual ~Allocator();

    template <typename T>
    T * allocate(size_t num, AllocationCategoryID cat=NoAllocationCategory) { return static_cast<T *>(allocate(num * sizeof(T), cat)); }

    virtual void * allocate(size_t size, AllocationCategoryID cat);

    virtual void deallocate(void *ptr);

    virtual bool verify() const { return true; }

    size_t allocationAmount(size_t size) {
        if (size < _minAllocationSize)
            size = _minAllocationSize;
        return size;
    }

    bool allocatorMatches(void *ptr);
    Allocatable * initAllocation(void *p, size_t size);

protected:
    Allocator *parent() const { return _parent; }

    const char * _name;
    Allocator * _parent;
    size_t _minAllocationSize;
};

} // namespace JB2
} // namespace OMR

#endif // defined(ALLOCATOR_INCL)
