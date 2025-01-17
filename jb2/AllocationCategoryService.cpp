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

#include "AllocationCategoryService.hpp"
#include "AllocatorRaw.hpp"

namespace OMR {
namespace JB2 {

AllocatorRaw raw;
AllocationCategoryService AllocationCategoryService::service(&raw);

AllocationCategoryID
AllocationCategoryService::getNextCategory(AllocationCategoryID cat) {
    if (cat == NoAllocationCategory) // 0 cannot be shifted
        return AnyAllocationCategory;
    AllocationCategoryID _nextCategory = cat << 1;
    return _nextCategory;
}

AllocationCategoryID
AllocationCategoryService::assignCategory(AllocationCategoryID baseCat, const char * name) {
    auto found = _categoryFromNameMap.find(name);
    if (found != _categoryFromNameMap.end())
        return found->second;
            
    AllocationCategoryID cat = _nextCategory;
    assert(cat != 0); // will eventually need a bit vector
    _nextCategory = getNextCategory(_nextCategory);

    AllocationCategoryID fullCat = baseCat | cat;
    _categoryFromNameMap.insert({name, fullCat});
    _nameFromCategoryMap.insert({fullCat, name});
    return fullCat;
}

} // namespace JB2
} // namespace OMR
