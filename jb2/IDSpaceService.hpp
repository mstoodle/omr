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

#ifndef IDSPACESERVICE_INCL
#define IDSPACESERVICE_INCL

#include <cstdint>
#include "Array.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

class Allocator;

//
// IDSpaceID is a compact, sequentially assigned integer that identifies one
// file-local ID space (e.g. "fTypeID", "fBuilderID").  It is used as an
// index/key into per-serializer encoding tables so there is no string lookup
// on the hot serialization path.
//
typedef uint32_t IDSpaceID;
const IDSpaceID NoIDSpace = 0;

//
// IDSpaceService is a process-wide registry that maps tag strings to compact
// IDSpaceID integers.  It follows the same pattern as AllocationCategoryService:
// an Allocator is passed at construction and a single Array<String *> holds the
// registered names indexed by IDSpaceID-1 (IDs start at 1).
//
// idSpaceFor(tag) is idempotent: the same tag always yields the same IDSpaceID.
// New IDs are assigned in first-call order starting at 1.  The total number of
// spaces is O(10-20) across all extensions, so linear scan over the array is
// acceptable and avoids any map dependency.
//
// IRSerializer and IRDeserializer subclasses call idSpaceFor() once during
// construction for each ID space they need, storing the result in a plain
// field.  The hot-path writeID/readID virtuals then just pass that stored
// integer — no lookup, no branch.
//
class IDSpaceService {
public:
    static IDSpaceService service;

    IDSpaceService(Allocator *mem);
    ~IDSpaceService();

    // Returns the IDSpaceID for tag, registering a new one if not yet seen.
    IDSpaceID idSpaceFor(const String & tag);

    // Returns the tag String for a given IDSpaceID.
    // Returns an empty String if id is NoIDSpace or out of range.
    String tagFor(IDSpaceID id) const;

    // Number of spaces registered so far.
    uint32_t count() const { return _count; }

private:
    Allocator       *_mem;
    uint32_t         _count;
    Array<String *>  _names;  // _names[id-1] is the name for IDSpaceID id
};

} // namespace JB2
} // namespace OMR

#endif // !defined(IDSPACESERVICE_INCL)
