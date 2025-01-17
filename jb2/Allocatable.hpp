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
#include "AllocationCategoryService.hpp"


namespace OMR {
namespace JB2 {

class Allocator;
class Compiler;
class Config;

// To allocate objects of class C wsing an Allocator, C must be a subclass of Allocatable, which records the Allocator used and the allocation size
class Allocatable {
    friend class Allocator;
public:
    virtual size_t allocatedSize() const {
        return __size;
    }

    virtual Allocator *allocator() const {
        return __mem;
    }

protected:
    // For dynamic allocation: new has already initialized __mem and __size
    Allocatable(Allocator *a) {
        if (a == NULL) {
            __mem = NULL;
            __size = 0;
            return;
        }
        //assert(__mem == NULL || __mem == a && __size > 0);
        assert(__mem == a && __size > 0);
    }
    // for nondynamic allocation: no new has happened and __mem and __size have not been initialized
    Allocatable()
        : __mem(NULL)
        , __size(0) {
        // __mem being NULL tells delete operator that new didn't allocate it so should be ignored
    }

private:
    // make these private so we can change the implementation later: base class fields isn't ideal for e.g. arrays
    Allocator *__mem;
    size_t __size;
};

} // namespace JB2
} // namespace OMR

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

#include "Allocator.hpp"

// Every jb2 class must references this macro: JBALLOC(cat) where cat is a category
// For headers that must declare and define destructor
// current code base uses this so keep it around
#define JBALLOC_NO_DESTRUCTOR(C,Category) \
    public: \
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
                if (mem != NULL) { \
                    mem->deallocate(ptr); \
                } \
            } \
        } \
        void operator delete[](void *ptr) noexcept { \
            if (ptr != NULL) {  \
                Allocatable *a = reinterpret_cast<C *>(ptr); \
                Allocator *mem = a->allocator(); \
                if (mem != NULL) { \
                    mem->deallocate(ptr); \
                } \
            } \
        } \
    protected:

// current code base uses this so keep it around
#define JBALLOC(C,Category) \
    public: \
        virtual ~C(); \
        JBALLOC_NO_DESTRUCTOR(C,Category) \
    protected:

// Most likely use this one or the next one
// This macro is directly inlined in Allocator.hpp which avoids circular dependency on this file
// ANY CHANGE TO THIS MACRO MUST BE COPIED TO Allocator.hpp
#define JBALLOC_(C) \
    private: \
        static AllocationCategoryID allocCategory; \
        static bool allocCategoryInitialized; \
    public: \
        static AllocationCategoryID allocCat(); \
        JBALLOC(C,C::allocCat())

#define JBALLOC_NO_DESTRUCTOR_(C) \
    private: \
        static AllocationCategoryID allocCategory; \
        static bool allocCategoryInitialized; \
    public: \
        static AllocationCategoryID allocCat(); \
        JBALLOC_NO_DESTRUCTOR(C,C::allocCat())


// One of these goes in the cpp file for class C and takes care of initializing the allocation category appropriately
#define INIT_JBALLOC_CAT(C,cat) \
    AllocationCategoryID C::allocCategory=NoAllocationCategory; \
    bool C::allocCategoryInitialized = false; \
    AllocationCategoryID \
    C::allocCat() { \
        if (!C::allocCategoryInitialized) { \
            C::allocCategory = (cat); \
            C::allocCategoryInitialized = true; \
        } \
        return C::allocCategory; \
    }

#define INIT_JBALLOC_TEMPLATE(C,cat) \
    template<class T> \
    AllocationCategoryID C<T>::allocCategory=NoAllocationCategory; \
    template<class T> \
    bool C<T>::allocCategoryInitialized = false; \
    template<class T> \
    AllocationCategoryID \
    C<T>::allocCat() { \
        if (!C<T>::allocCategoryInitialized) { \
            C<T>::allocCategory = (cat); \
            C<T>::allocCategoryInitialized = true; \
        } \
        return C<T>::allocCategory; \
    }

// This one reuses a category from another class B
#define INIT_JBALLOC_REUSECAT(C,B) \
    INIT_JBALLOC_CAT(C, B::allocCat())

// This one assigns a new category ID on top of a base class B's category
#define INIT_JBALLOC_BASE(C,B,name) \
    INIT_JBALLOC_CAT(C, AllocationCategoryService::service.assignCategory(B::allocCat(), name))

// This one assigns a new category ID with AnyAllocationCategory as base
#define INIT_JBALLOC_NEWCAT(C,name) \
    INIT_JBALLOC_CAT(C, AllocationCategoryService::service.assignCategory(AllocationCategoryService::AnyAllocationCategory, name))

// This one assigns a new category ID on top of another category ID
#define INIT_JBALLOC_NEWCAT_BASE(C,base,name) \
    INIT_JBALLOC_CAT(C, AllocationCategoryService::service.assignCategory(base, name))

// This one assigns a new category ID on top of a base class B's category and uses C's source name as the category name
#define INIT_JBALLOC(C) \
    INIT_JBALLOC_NEWCAT(C,#C)

#define INIT_JBALLOC_ON(C,B) \
    INIT_JBALLOC_BASE(C,B,#C)


// Used to create allocation categories that aren't tied to a specific class
#define CATEGORYCLASS(C) \
class C { \
    private: \
        static AllocationCategoryID allocCategory; \
        static bool allocCategoryInitialized; \
    public: \
        static AllocationCategoryID allocCat(); \
}


#define CATEGORYCLASS_DEFS(C,base) \
    bool C::allocCategoryInitialized = false; \
    AllocationCategoryID C::allocCategory = NoAllocationCategory; \
    AllocationCategoryID \
    C::allocCat() { \
        if (!C::allocCategoryInitialized) { \
            C::allocCategory = AllocationCategoryService::service.assignCategory((base), #C); \
            C::allocCategoryInitialized = true; \
        } \
        return C::allocCategory; \
    } \

#define CATEGORYCLASS_DEFS_ON(C,B) \
    CATEGORYCLASS_DEFS(C,B::allocCat())

#define CATEGORYCLASS_DEFS_NEW(C) \
    CATEGORYCLASS_DEFS(C,AllocationCategoryService::AnyAllocationCategory)

#include "AllocationCategoryClasses.hpp"

#endif // defined(ALLOCATABLE)
