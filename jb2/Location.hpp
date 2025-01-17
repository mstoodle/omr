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

#ifndef LOCATION_INCL
#define LOCATION_INCL

#include "common.hpp"
#include "ExtensibleIR.hpp"
#include "String.hpp"

namespace OMR {
namespace JB2 {

class Compilation;
class ExtensibleIR;
class IR;
class IRCloner;

class Location : public ExtensibleIR {
    JBALLOC_(Location)

    friend class IR;
    friend class IRCloner;

public:
    DYNAMIC_ALLOC_ONLY(Location, IR *ir, String fileName, String lineNumber);
    DYNAMIC_ALLOC_ONLY(Location, IR *ir, String fileName, String lineNumber, int32_t bcIndex);

    virtual size_t size()             { return sizeof(Location); }
    LocationID id() const             { return _id; }
    ByteCodeIndex bcIndex() const     { return _bcIndex; }
    const String & fileName() const   { return _fileName; }
    const String & lineNumber() const { return _lineNumber; }

protected:
    Location(Allocator *a, const Location *source, IRCloner *cloner);

    virtual ExtensibleIR *clone(Allocator *mem, IRCloner *cloner) const { return reinterpret_cast<ExtensibleIR *>(cloneLocation(mem, cloner)); } // TODO: FIX!
    virtual Location *cloneLocation(Allocator *mem, IRCloner *cloner) const;

    LocationID    _id;
    String        _fileName;
    String        _lineNumber;
    ByteCodeIndex _bcIndex;

    SUBCLASS_KINDSERVICE_DECL(Extensible,Location);
};

} // namespace JB2
} // namespace OMR

#endif // defined(LOCATION_INCL)

