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

#include <stdint.h>
#include <string>
#include "IDs.hpp"

namespace OMR {
namespace JitBuilder {

class Compilation;

class Location {
public:
    Location(Compilation *comp, std::string fileName, std::string lineNumber);
    Location(Compilation *comp, std::string fileName, std::string lineNumber, int32_t bcIndex);

    virtual size_t size()          { return sizeof(Location); }
    LocationID id() const          { return _id; }
    ByteCodeIndex bcIndex() const  { return _bcIndex; }
    std::string fileName() const   { return _fileName; }
    std::string lineNumber() const { return _lineNumber; }

protected:
    LocationID    _id;
    Compilation * _comp;
    std::string   _fileName;
    std::string   _lineNumber;
    ByteCodeIndex _bcIndex;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(LOCATION_INCL)

