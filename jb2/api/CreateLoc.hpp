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
#include "util/String.hpp"

#define LOCATION const char *loc_fileName, uint32_t loc_lineNumber, const char *loc_function
#define LOC_FILE loc_fileName
#define LOC_LINE loc_lineNumber
#define LOC_FUNC loc_function
#define LOC __FILE__, __LINE__, __func__
#define PASSLOC loc_fileName, loc_lineNumber, loc_function

namespace OMR {
namespace JitBuilder {

class CreateLocation {
public:
    CreateLocation(LOCATION)
        : _fileName(LOC_FILE)
        , _lineNumber(LOC_LINE)
        , _functionName(LOC_FUNC)
    { }

    String fileName() const { return String(_fileName); }
    String lineNumber() const { return String::to_string(_lineNumber); }
    String functionName() const { return String(_functionName); }

    String to_string() const {
        return String(_functionName).append(" in ").append(_fileName).append(":L").append(String::to_string(_lineNumber));
    }

    void overrideFileName(const char *fileName) { _fileName = fileName; }
    void overrideLineNumber(uint32_t lineNumber) { _lineNumber = lineNumber; }
    void overrideFunctionName(const char *functionName) { _functionName = functionName; }

protected:
    const char *_fileName;
    uint32_t _lineNumber;
    const char *_functionName;
};

} // namespace JitBuilder
} // namespace OMR

#endif // defined(CREATELOC_INCL)

