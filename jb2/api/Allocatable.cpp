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

#ifndef ALLOCATABLE_INCL
#define ALLOCATABLE_INCL

#include <cassert>
#include <cstdint>
#include <new>
#include "IDs.hpp"
#include "KindService.hpp"


namespace OMR {
namespace JitBuilder {

class Allocator;
class Compiler;
class Config;

typedef uint64_t AllocationCategoryID;
const AllocationCategoryID NoAllocationCategory=0;

// To allocate objects of class C wsing an Allocator, C must be a subclass of Allocatable, which records the Allocator used and the allocation size
class Allocatable {
    friend class Allocator;
public:
    virtual size_t allocatedSize() {
        return __size;
    }

    virtual Allocator *allocator() {
        return __mem;
    }

protected:
    // For dynamic allocation: new has already initialized __mem and __size
    Allocatable(Allocator *a) {
        assert(__mem == a && __size > 0);
    }
    // for nondynamic allocation: no new has happened and __mem and __size have not initialized
    Allocatable()
        : __mem(NULL)
        , __size(0) {
        // __mem being NULL tells delete operator that new didn't allocate it
    }

    Allocator *__mem;
    size_t __size;

    static KindService categoryKind;
};

#define DYNAMIC_CONSTRUCTOR_NOARGS(C) \
    C(Allocator *a)
#define DYNAMIC_CONSTRUCTOR(C, ...) \
    C(Allocator *a, __VA_ARGS__)

#define NONDYNAMIC_CONSTRUCTOR_NOARGS(C) \
    C()
#define NONDYNAMIC_CONSTRUCTOR(C, ...) \
    C(__VA_ARGS__)

#define DYNAMIC_ALLOC_ONLY_NOARGS(C) \
    DYNAMIC_CONSTRUCTOR_NOARGS(C)

#define DYNAMIC_ALLOC_ONLY(C, ...) \
    DYNAMIC_CONSTRUCTOR(C, __VA_ARGS__)

#define ALL_ALLOC_ALLOWED_NOARGS(C) \
    DYNAMIC_CONSTRUCTOR_NOARGS(C); \
    NONDYNAMIC_CONSTRUCTOR_NOARGS(C)

#define ALL_ALLOC_ALLOWED(C, ...) \
    DYNAMIC_CONSTRUCTOR(C, __VA_ARGS__); \
    NONDYNAMIC_CONSTRUCTOR(C, __VA_ARGS__)

#define BADALLOC 0xc011AdaB // BadA11oc backwards, unaligned address

// Every jb2 class must references this macro: JBALLOC(cat) where cat is a category
#define JBALLOC(C,Category) \
    public: \
        virtual ~C(); \
        void *operator new(size_t size, Allocator *a, AllocationCategoryID cat=Category) { \
            return a->initAllocation(a->allocate(size, cat), size); \
        } \
        void *operator new[](std::size_t size, Allocator *a, AllocationCategoryID cat=Category) { \
            return a->initAllocation(a->allocate(size, cat), size); \
        } \
        void operator delete(void *ptr) { \
            if (ptr != NULL) {  \
                Allocatable *a = reinterpret_cast<C *>(ptr); \
                Allocator *mem = a->allocator(); \
                if (mem!= NULL) { \
                    mem->deallocate(a); \
                } \
            } \
        } \
        void operator delete[](void *ptr) noexcept { \
            if (ptr != NULL) {  \
                Allocatable *a = reinterpret_cast<C *>(ptr); \
                Allocator *mem = a->allocator(); \
                if (mem != NULL) { \
                    mem->deallocate(a); \
                } \
            } \
        } \

} // namespace JitBuilder
} // namespace OMR

#endif // defined(ALLOCATABLE)
