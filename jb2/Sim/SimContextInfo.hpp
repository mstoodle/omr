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

#ifndef FUNCTIONDEBUGINFO_INCL
#define FUNCTIONDEBUGINFO_INCL

#include <map>
#include "Base/FunctionCompilation.hpp"
#include "Debug/DebugDictionary.hpp"

namespace OMR {
namespace JB2 {

namespace Base { class Function; }

namespace Debug {

class DebuggerFrame;
class OperationDebugger;

typedef bool (OperationDebuggerFunc)(DebuggerFrame *, int64_t);

// FunctionDebugInfo holds debugger information corresponding to a Function
// (information that can be shared across multiple DebuggerFrames). Since there
// are read/log fields in this class, synchronization will be required if
// multiple threads access one of these objects. Alternatively, debuggers for all
// Operations in a Function could be generated ahead of time, at which point
// this structure would become read-only.
class FunctionDebugInfo {
public:
    FunctionDebugInfo(Base::Function *func, StrategyID strategy, Config *config)
        : _func(func)
        , _comp(func->compiler(), func, strategy, NULL, config))
        , _dbgDict(comp)
        , _valueSizeInBytes(_dbgDict._DebugValue->size()/8) {

    }

    Base::Function *func() const { return _func; }
    Base::FunctionCompilation *comp() { return &_comp; }
    DebugDictionary *dict() { return &_dbgDict; }
    size_t valueSizeInBytes() const { return _valueSizeInBytes; }

    Base::Function * _func;
    Base::FunctionCompilation _comp;
    DebugDictionary _dbgDict;
    size_t _valueSizeInBytes;
    std::map<OperationID, OperationDebugger *> _operationDebugBuilders;
    std::map<OperationID, OperationDebuggerFunc *> _operationDebuggers;
    std::map<OperationID, bool> _debugOperations;
};

} // namespace Debug
} // namespace JB2
} // namespace OMR

#endif // defined(FUNCTIONDEBUGINFO_INCL)
