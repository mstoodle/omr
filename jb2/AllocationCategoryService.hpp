/*******************************************************************************
 * Copyright (c) 2021, 2022 IBM Corp. and others
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

#ifndef ALLOCATIONCATEGORYSERVICE_INCL
#define ALLOCATIONCATEGORYSERVICE_INCL

#include <cassert>
#include <cstddef>
#include <map>

namespace OMR {
namespace JB2 {

typedef uint64_t AllocationCategoryID;
const AllocationCategoryID NoAllocationCategory=0;

class Allocator;

class AllocationCategoryService {
public:
    const static AllocationCategoryID AnyAllocationCategory=1;
    static AllocationCategoryService service;

    AllocationCategoryService(Allocator *a)
        : _mem(a)
        , _nextCategory(AnyAllocationCategory+1) {
    }
    virtual ~AllocationCategoryService() { }

    AllocationCategoryID getNextCategory(AllocationCategoryID k);

    AllocationCategoryID assignCategory(AllocationCategoryID baseCategory, const char * name);
    bool isExactMatch(AllocationCategoryID matchee, AllocationCategoryID matcher) {
        return (matchee == matcher);
    }
    bool isMatch(AllocationCategoryID matchee, AllocationCategoryID matcher) {
        return ((matchee & matcher) == matcher);
    }

protected:
    Allocator *_mem;
    AllocationCategoryID _nextCategory;
    std::map<const char *,AllocationCategoryID> _categoryFromNameMap;
    std::map<AllocationCategoryID,const char *> _nameFromCategoryMap;
};

} // namespace JB2
} // namespace OMR

#endif // defined(ALLOCATIONCATEGORYSERVICE_INCL)
