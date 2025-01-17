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

#ifndef CREATELOC_INCL
#define CREATELOC_INCL

#include <stdint.h>
#include "Allocatable.hpp"
#include "String.hpp"

#define LOCATION const char *loc_fileName, uint32_t loc_lineNumber, const char *loc_function
#define LOC_FILE loc_fileName
#define LOC_LINE loc_lineNumber
#define LOC_FUNC loc_function
#define LOC __FILE__, __LINE__, __func__
#define PASSLOC loc_fileName, loc_lineNumber, loc_function

#define MEM_LOCATION(m) Allocator *m, LOCATION
#define MEM_LOC(m) m, LOC
#define MEM_PASSLOC(m) m, PASSLOC

namespace OMR {
namespace JB2 {

class CreateLocation : public Allocatable {
    JBALLOC_NO_DESTRUCTOR_(CreateLocation)

public:
    CreateLocation(MEM_LOCATION(a))
        : Allocatable(a)
        , _fileName(LOC_FILE)
        , _lineNumber(LOC_LINE)
        , _functionName(LOC_FUNC) {

    }
    CreateLocation(LOCATION)
        : Allocatable()
        , _fileName(LOC_FILE)
        , _lineNumber(LOC_LINE)
        , _functionName(LOC_FUNC) {

    }
    virtual ~CreateLocation() { }

    String fileName(Allocator *dataAllocator) const { return String(dataAllocator, _fileName); }
    String lineNumber(Allocator *dataAllocator) const { return String::to_string(dataAllocator, _lineNumber); }
    String functionName(Allocator *dataAllocator) const { return String(dataAllocator, _functionName); }

    String to_string(Allocator *dataAllocator) const {
        return String(dataAllocator, _functionName).append(" in ").append(_fileName).append("@").append(String::to_string(dataAllocator, _lineNumber));
    }

    void overrideFileName(const char *fileName) { _fileName = fileName; }
    void overrideLineNumber(uint32_t lineNumber) { _lineNumber = lineNumber; }
    void overrideFunctionName(const char *functionName) { _functionName = functionName; }

protected:
    const char *_fileName;
    uint32_t _lineNumber;
    const char *_functionName;
};

} // namespace JB2
} // namespace OMR

#endif // defined(CREATELOC_INCL)

