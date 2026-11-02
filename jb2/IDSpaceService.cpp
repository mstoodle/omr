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

#include "Allocator.hpp"
#include "AllocatorRaw.hpp"
#include "IDSpaceService.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

// The global singleton is backed by a raw allocator, matching the pattern
// used by AllocationCategoryService and KindService.
static AllocatorRaw idSpaceRaw;
IDSpaceService IDSpaceService::service(&idSpaceRaw);

IDSpaceService::IDSpaceService(Allocator *mem)
    : _mem(mem)
    , _count(0)
    , _names(NULL, mem) {
}

IDSpaceService::~IDSpaceService() {
    for (uint32_t i = 0; i < _count; i++) {
        _mem->deallocate(_names[i]);
    }
}

IDSpaceID
IDSpaceService::idSpaceFor(const String & tag) {
    // Linear scan: walk existing entries looking for a match by value.
    for (uint32_t i = 0; i < _count; i++) {
        if (*_names[i] == tag)
            return static_cast<IDSpaceID>(i + 1);
    }

    // Not found — register a new entry.
    IDSpaceID id = static_cast<IDSpaceID>(++_count);
    String *name = new (_mem) String(_mem, tag);
    _names.assign(id - 1, name);
    return id;
}

String
IDSpaceService::tagFor(IDSpaceID id) const {
    if (id == NoIDSpace || id > _count)
        return String();
    return *_names[id - 1];
}

} // namespace JB2
} // namespace OMR
